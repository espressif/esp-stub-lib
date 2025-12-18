/** ROM APIs
 */

PROVIDE ( esp_rom_crc32_le = crc32_le );
PROVIDE ( esp_rom_crc16_le = crc16_le );
PROVIDE ( esp_rom_crc8_le  = crc8_le );
PROVIDE ( esp_rom_crc32_be = crc32_be );
PROVIDE ( esp_rom_crc16_be = crc16_be );
PROVIDE ( esp_rom_crc8_be  = crc8_be );

PROVIDE ( esp_rom_cdc_acm_fifo_fill = cdc_acm_fifo_fill );
PROVIDE ( esp_rom_cdc_acm_fifo_read = cdc_acm_fifo_read );
PROVIDE ( esp_rom_cdc_acm_rx_fifo_cnt = cdc_acm_rx_fifo_cnt );
PROVIDE ( esp_rom_cdc_acm_line_ctrl_get = cdc_acm_line_ctrl_get );
PROVIDE ( esp_rom_cdc_acm_irq_rx_enable = cdc_acm_irq_rx_enable );
PROVIDE ( esp_rom_cdc_acm_irq_rx_disable = cdc_acm_irq_rx_disable );
PROVIDE ( esp_rom_cdc_acm_irq_callback_set = cdc_acm_irq_callback_set );
PROVIDE ( esp_rom_cdc_acm_irq_state_enable = cdc_acm_irq_state_enable );

PROVIDE ( esp_rom_usb_dc_check_poll_for_interrupts = usb_dc_check_poll_for_interrupts );
PROVIDE ( esp_rom_usb_dc_prepare_persist = usb_dc_prepare_persist );
PROVIDE ( esp_rom_chip_usb_set_persist_flags = chip_usb_set_persist_flags );
PROVIDE ( esp_rom_usb_dw_isr_handler = usb_dw_isr_handler );

PROVIDE ( esp_rom_gpio_pad_select_gpio    = gpio_pad_select_gpio );
PROVIDE ( esp_rom_gpio_pad_pullup_only    = gpio_pad_pullup );
PROVIDE ( esp_rom_gpio_pad_set_drv        = gpio_pad_set_drv );
PROVIDE ( esp_rom_gpio_pad_unhold         = gpio_pad_unhold );
PROVIDE ( esp_rom_gpio_connect_in_signal  = gpio_matrix_in );
PROVIDE ( esp_rom_gpio_connect_out_signal = gpio_matrix_out );

PROVIDE ( esp_rom_efuse_mac_address_crc8       = esp_crc8 );
PROVIDE ( esp_rom_efuse_is_secure_boot_enabled = ets_efuse_secure_boot_enabled );

PROVIDE ( esp_rom_uart_flush_tx       = uart_tx_flush );
PROVIDE ( esp_rom_uart_tx_one_char    = uart_tx_one_char2 );
PROVIDE ( esp_rom_uart_tx_wait_idle   = uart_tx_wait_idle );
PROVIDE ( esp_rom_uart_rx_one_char    = uart_rx_one_char );
PROVIDE ( esp_rom_uart_rx_string      = UartRxString );
PROVIDE ( esp_rom_uart_set_as_console = uart_tx_switch );
PROVIDE ( esp_rom_uart_putc           = ets_write_char_uart );
PROVIDE ( esp_rom_uart_attach         = uartAttach );
PROVIDE ( esp_rom_uart_init           = Uart_Init );
PROVIDE ( esp_rom_uart_div_modify     = uart_div_modify );
PROVIDE ( esp_rom_get_apb_freq        = ets_get_apb_freq );
PROVIDE ( esp_rom_get_cpu_freq        = ets_clk_get_cpu_freq );
PROVIDE ( esp_rom_get_uart_device     = GetUartDevice );

PROVIDE ( esp_rom_usb_serial_putc     = uart_tx_one_char3 );

PROVIDE ( esp_rom_output_flush_tx       = uart_tx_flush );
PROVIDE ( esp_rom_output_tx_one_char    = uart_tx_one_char );
PROVIDE ( esp_rom_output_tx_wait_idle   = uart_tx_wait_idle );
PROVIDE ( esp_rom_output_rx_one_char    = uart_rx_one_char );
PROVIDE ( esp_rom_output_rx_string      = UartRxString );
PROVIDE ( esp_rom_output_set_as_console = uart_tx_switch );
PROVIDE ( esp_rom_output_putc           = ets_write_char_uart );

PROVIDE ( esp_rom_md5_init   = MD5Init );
PROVIDE ( esp_rom_md5_update = MD5Update );
PROVIDE ( esp_rom_md5_final  = MD5Final );

PROVIDE ( esp_rom_software_reset_system = software_reset );
PROVIDE ( esp_rom_software_reset_cpu    = software_reset_cpu );

PROVIDE ( esp_rom_printf   = ets_printf );
PROVIDE ( esp_rom_install_uart_printf = ets_install_uart_printf );
PROVIDE ( esp_rom_delay_us = ets_delay_us );
PROVIDE ( esp_rom_get_reset_reason = rtc_get_reset_reason );
PROVIDE ( esp_rom_route_intr_matrix = intr_matrix_set );
PROVIDE ( esp_rom_esprv_intc_int_set_priority = esprv_intc_int_set_priority );
PROVIDE ( esp_rom_isr_attach = ets_isr_attach );
PROVIDE ( esp_rom_isr_unmask = ets_isr_unmask );
PROVIDE ( esp_rom_isr_mask = ets_isr_mask );
PROVIDE ( esp_rom_get_cpu_ticks_per_us = ets_get_cpu_frequency );
PROVIDE ( esp_rom_set_cpu_ticks_per_us = ets_update_cpu_frequency );

PROVIDE ( esp_rom_spiflash_attach = spi_flash_attach );
PROVIDE ( esp_rom_spiflash_clear_bp = esp_rom_spiflash_unlock );
PROVIDE ( esp_rom_spiflash_write_enable = SPI_write_enable );
PROVIDE ( esp_rom_spiflash_erase_area = SPIEraseArea );

PROVIDE ( esp_rom_spiflash_fix_dummylen = spi_dummy_len_fix );
PROVIDE ( esp_rom_spiflash_set_drvs = SetSpiDrvs);
PROVIDE ( esp_rom_spiflash_select_padsfunc = SelectSpiFunction );
PROVIDE ( esp_rom_spiflash_common_cmd = SPI_Common_Command );
