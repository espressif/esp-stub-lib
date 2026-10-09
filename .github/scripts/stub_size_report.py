#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
"""Compare OpenOCD stub ELF sizes of two build artifacts and write a Markdown report."""

import argparse
import json
import re
import sys
from pathlib import Path

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

SECTIONS = ('.text', '.data', '.bss')
ELF_NAME_RE = re.compile(r'^stub_[a-z0-9]+_(?P<command>.+)\.elf$')
LOG_TAIL_LINES = 50
MAX_FAILED_LOGS = 3


class Build:
    def __init__(self, root, targets):
        self.root = Path(root)
        self.exists = self.root.is_dir()
        self.openocd_commit = self._read('openocd_commit')
        self.stub_lib_commit = self._read('stub_lib_commit')
        self.ok_targets = [t for t in targets if (self.root / 'status' / f'{t}.ok').is_file()]
        self.failed_targets = [t for t in targets if t not in self.ok_targets]
        self.sizes = self._collect(self.ok_targets)

    def _read(self, name):
        path = self.root / name
        return path.read_text().strip() if path.is_file() else None

    def _collect(self, targets):
        sizes = {}
        for target in targets:
            for elf_path in sorted((self.root / 'build' / target).glob('stub_*.elf')):
                match = ELF_NAME_RE.match(elf_path.name)
                if match:
                    sizes[(target, match.group('command'))] = read_elf(elf_path)
        return sizes

    def log_tail(self, target):
        log = self.root / 'logs' / f'{target}.log'
        if not log.is_file():
            return None
        return '\n'.join(log.read_text(errors='replace').splitlines()[-LOG_TAIL_LINES:])


def read_elf(path):
    with open(path, 'rb') as f:
        elf = ELFFile(f)
        info = {name: 0 for name in SECTIONS}
        for section in elf.iter_sections():
            if section.name in info:
                info[section.name] = section['sh_size']
        info['iram_len'] = 0
        info['dram_len'] = 0
        symtab = elf.get_section_by_name('.symtab')
        if isinstance(symtab, SymbolTableSection):
            for sym in symtab.iter_symbols():
                name = sym.name.lstrip('.')
                if name in ('iram_len', 'dram_len'):
                    info[name] = sym['st_value']
    return info


def total(info):
    return sum(info[name] for name in SECTIONS)


def percent(used, length):
    return f'{100.0 * used / length:.1f}%' if length else '-'


def signed(value):
    return f'+{value}' if value > 0 else str(value)


def short(commit):
    return f'`{commit[:10]}`' if commit else 'unknown'


def size_cell(head, base, name):
    if base is None or head[name] == base[name]:
        return str(head[name])
    return f'{head[name]} ({signed(head[name] - base[name])})'


def row(key, head, base, has_baseline=True):
    target, command = key
    if head is None:
        return f'| {target} | {command} | removed | | | {signed(-total(base))} | | |'
    if not has_baseline:
        delta = '-'
    elif base is None:
        delta = 'new'
    else:
        delta = signed(total(head) - total(base))
    cells = [size_cell(head, base, name) for name in SECTIONS]
    iram = percent(head['.text'], head['iram_len'])
    dram = percent(head['.data'] + head['.bss'], head['dram_len'])
    return f'| {target} | {command} | {" | ".join(cells)} | {delta} | {iram} | {dram} |'


TABLE_HEADER = [
    '| Target | Command | .text | .data | .bss | Total delta | IRAM | DRAM |',
    '|---|---|--:|--:|--:|--:|--:|--:|',
]


def table(rows):
    return '\n'.join(TABLE_HEADER + rows)


def details(summary, body):
    return f'<details>\n<summary>{summary}</summary>\n\n{body}\n\n</details>'


def finish(lines):
    return '\n'.join(lines) + '\n'


def build_report(head, base, targets):
    lines = ['## OpenOCD stub size report', '']
    lines.append(
        f'openocd-esp32 {short(head.openocd_commit or base.openocd_commit)}, '
        f'esp-stub-lib base {short(base.stub_lib_commit)}, head {short(head.stub_lib_commit)}'
    )
    lines.append('')

    if not head.exists:
        lines.append(':x: **No head build artifacts found.** See the `Build` jobs.')
        return finish(lines)

    if head.failed_targets:
        failed = ', '.join(head.failed_targets)
        lines.append(f':x: **Head build failed for {failed}.** See the matching `Build (<target>)` jobs.')
        for target in head.failed_targets[:MAX_FAILED_LOGS]:
            tail = head.log_tail(target)
            if tail:
                lines += ['', details(f'{target}: last {LOG_TAIL_LINES} lines of the build log', f'```\n{tail}\n```')]
        return finish(lines)

    if not head.sizes:
        lines.append(':x: **Head build produced no stub ELF files.** Check the build output paths and artifact upload.')
        return finish(lines)

    compared = set(base.ok_targets)
    if not compared:
        keys = sorted(head.sizes)
        lines.append(':warning: **No baseline.** The base build failed or produced no artifacts.')
        lines += ['', details('Head sizes', table([row(k, head.sizes[k], None, has_baseline=False) for k in keys]))]
        return finish(lines)

    all_keys = sorted(set(head.sizes) | {k for k in base.sizes if k[0] in compared})
    changed = []
    grown = []
    shrunk = 0
    added = 0
    removed = 0
    for key in all_keys:
        if key[0] not in compared:
            continue
        h = head.sizes.get(key)
        b = base.sizes.get(key)
        if h is not None and b is not None and all(h[n] == b[n] for n in SECTIONS):
            continue
        changed.append(row(key, h, b))
        if b is None:
            added += 1
            continue
        if h is None:
            removed += 1
            continue
        delta = total(h) - total(b)
        if delta > 0:
            grown.append((delta, key))
        elif delta < 0:
            shrunk += 1

    if not changed:
        lines.append(':white_check_mark: No size changes.')
    else:
        parts = []
        if grown:
            delta, (target, command) = max(grown)
            parts.append(f'{len(grown)} stub(s) grew (largest: +{delta} B on {target}/{command})')
        if shrunk:
            parts.append(f'{shrunk} stub(s) shrank')
        if added:
            parts.append(f'{added} stub(s) added')
        if removed:
            parts.append(f'{removed} stub(s) removed')
        if not parts:
            parts.append('Section sizes changed with no change in total')
        lines.append(':bar_chart: ' + '. '.join(parts) + '.')

    no_baseline = [t for t in targets if t not in compared]
    if no_baseline:
        lines += ['', f':warning: No baseline for {", ".join(no_baseline)}. The base build failed for these targets.']

    if changed:
        lines += ['', table(changed)]

    full = table([row(k, head.sizes.get(k), base.sizes.get(k), has_baseline=k[0] in compared) for k in all_keys])
    lines += ['', details('All stubs', full)]
    return finish(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--head', required=True, help='Head build artifact directory')
    parser.add_argument('--base', required=True, help='Base build artifact directory')
    parser.add_argument('--targets', required=True, help='JSON list of expected build targets')
    parser.add_argument('--out', required=True, help='Output Markdown file')
    args = parser.parse_args()

    targets = json.loads(args.targets)
    report = build_report(Build(args.head, targets), Build(args.base, targets), targets)
    Path(args.out).write_text(report)
    sys.stdout.write(report)


if __name__ == '__main__':
    main()
