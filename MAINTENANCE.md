# esp-stub-lib maintenance & release policy

This document defines the maintenance rules for `esp-stub-lib` when multiple teams develop and integrate in parallel.

## 1) Public API definition

**Public API** is everything exposed by headers under:

- `include/esp-stub-lib/**`

Consumers must not rely on internal headers or internal symbols outside of this directory.

## 2) Versioning & releases (SemVer)

This project follows **Semantic Versioning**: `MAJOR.MINOR.PATCH` (e.g. `1.4.2`).

Releases are created and tagged regularly using **commitizen** (and its generated changelog/release notes).

**Integration rule for downstream projects:**
- Downstream projects should pin the submodule to **release tags** (e.g. `v1.3.0`), not to branches and not to arbitrary commits.
- Pinning to the exact commit that an annotated tag points at is equally valid. This is useful when a team has already tested and approved commit X: they can create a tag pointing at X without needing to move to a different commit and re-run their testing pipelines.

Rationale: tags are explicit integration points with a known set of changes and release notes.

## 3) Branch policy

- There is exactly **one maintained branch: `master`**.
- Feature branches may exist for development, but no other long-lived maintained branches (no `release/*` branches).

## 4) Breaking changes policy

A change is considered **breaking** if it changes **public API or behavior** in a way that can break existing consumers.

This includes (non-exhaustive):
- Changes to function signatures, types, macros, constants in `include/esp-stub-lib/**`
- Removal/renaming of any public API surface
- Behavioral changes that break existing correct usage (even if the header signature is unchanged)

### Required review for breaking changes
Any PR that introduces a breaking change MUST be reviewed by **all teams** before merge.

Operationally:
- PR must be clearly marked as breaking (e.g. title prefix `BREAKING:` and/or label `breaking-change`)
- The merge commit must follow the Conventional Commits breaking-change format so that commitizen detects it automatically:
  - Use `!` after the type/scope: `feat(flash)!: change read buffer type`, **or**
  - Include a `BREAKING CHANGE:` footer in the commit body
- Approval from all teams is required to merge

## 5) Who can release / tag

Each team is allowed to create releases/tags.

Typical flow:
- Team A merges a change to `master` that they need in their tool.
- Team A validates the commit with their tool, then performs a release directly on `master`:
  - Run `cz bump` on the up-to-date `master` branch — this commits a version bump and creates the tag locally.
  - Push both the commit and the tag: `git push && git push --tags`.
  - No separate release PR is needed. The push triggers CI and creates a draft GitHub release with auto-generated release notes. The same person who tagged may review and publish the draft.
- Team B (and any other teams) can run automation that reacts to new tags (e.g., opens a PR to bump the submodule to the new tag) and executes integration tests.
- Teams can either:
  - accept and merge the bump to keep up with the flow (reducing future catch-up cost), or
  - defer if they have a blocking issue, understanding that later upgrades will include more accumulated changes.

## 6) Expectations for release quality

Because tags are the supported integration points:
- Do not tag a release unless it is buildable and passes the releasing team's integration test(s).
- If a release is found to be bad, follow SemVer:
  - ship a new PATCH release with the fix, or
  - if necessary, ship a revert + PATCH release

(Do not move/retag an existing version tag.)
