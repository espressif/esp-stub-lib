/* ESP32-P4 rev1 ROM GPIO API aliases. */
PROVIDE ( esp_rom_gpio_set_output_level      = gpio_set_output_level );
PROVIDE ( esp_rom_gpio_connect_in_signal     = gpio_matrix_in );
PROVIDE ( esp_rom_gpio_connect_out_signal    = gpio_matrix_out );
PROVIDE ( esp_rom_gpio_output_disable        = gpio_output_disable );
PROVIDE ( esp_rom_gpio_output_enable         = gpio_output_enable );
PROVIDE ( esp_rom_gpio_pad_input_enable      = gpio_pad_input_enable );
PROVIDE ( esp_rom_gpio_pad_pullup_only       = gpio_pad_pullup );
PROVIDE ( esp_rom_gpio_pad_select_gpio       = gpio_pad_select_gpio );
PROVIDE ( esp_rom_gpio_pad_set_drv           = gpio_pad_set_drv );
PROVIDE ( esp_rom_gpio_pad_unhold            = gpio_pad_unhold );
