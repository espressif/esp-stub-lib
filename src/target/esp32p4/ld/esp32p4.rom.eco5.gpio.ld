/* ESP32-P4 ECO5 ROM GPIO API aliases. */
PROVIDE ( esp_rom_gpio_set_output_level      = rom_gpio_set_output_level );
PROVIDE ( esp_rom_gpio_connect_in_signal     = rom_gpio_matrix_in );
PROVIDE ( esp_rom_gpio_connect_out_signal    = rom_gpio_matrix_out );
PROVIDE ( esp_rom_gpio_output_disable        = rom_gpio_output_disable );
PROVIDE ( esp_rom_gpio_output_enable         = rom_gpio_output_enable );
PROVIDE ( esp_rom_gpio_pad_input_enable      = rom_gpio_pad_input_enable );
PROVIDE ( esp_rom_gpio_pad_pullup_only       = rom_gpio_pad_pullup );
PROVIDE ( esp_rom_gpio_pad_select_gpio       = rom_gpio_pad_select_gpio );
PROVIDE ( esp_rom_gpio_pad_set_drv           = rom_gpio_pad_set_drv );
PROVIDE ( esp_rom_gpio_pad_unhold            = rom_gpio_pad_unhold );
