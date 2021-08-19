#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "driver/i2c.h"
#include "bootloader_random.h"
#include "vfd_driver.h"



void app_main(void) {
    bootloader_random_enable();
    ESP_ERROR_CHECK(i2c_master_init());
    init_io_expander();
    uint8_t mode = init_display();
    while (1) {
        mode = matrix_write("Hello world!!!", mode, 2, 25000);
        usleep(3000000);
        mode = clear_display(mode);
        mode = matrix_write("Hello from ESP32!!!", mode, 5, 10000);
        usleep(3000000);
        mode = clear_display(mode);
        mode = matrix_write("This is a long message! See how long it is? Wow!!! How cool!", mode, 5, 10000);
        usleep(3000000);
        mode = clear_display(mode);
        mode = set_cursor_position(0x28, mode);
        mode = write_str("This should start on the second line!", mode);
        usleep(3000000);
        mode = clear_display(mode);
        mode = fill(mode);
        usleep(10000000);
    }
}
