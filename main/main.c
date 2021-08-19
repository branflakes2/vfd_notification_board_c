#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "driver/i2c.h"
#include "esp_random.h"
#include "bootloader_random.h"

#define I2C_SCL_GPIO 13
#define I2C_SDA_GPIO 14
#define I2C_NUM 0
#define I2C_FREQ_HZ 400000
#define I2C_NUM_TICKS_TIMEOUT 400000 // I have no idea what a tick references. guessing it's one cycle of the i2c clock.

#define VFD_ADDR 0x20
#define IODIRA_REG 0x00
#define IODIRB_REG 0x01
#define IOCON_REG 0x0A
#define GPIOA 0x12
#define GPIOB 0x13

#define CONTROL_MODE 0x00 //B0 is RS B1 is E
#define DATA_MODE 0x01

static esp_err_t i2c_master_init(void) {
    int i2c_master_port = I2C_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

static esp_err_t write_vfd(uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(I2C_NUM, VFD_ADDR, write_buf, 2, I2C_NUM_TICKS_TIMEOUT);
}

static esp_err_t write_a(uint8_t data) {
    return write_vfd(GPIOA, data);
}

static esp_err_t write_b(uint8_t data) {
    return write_vfd(GPIOB, data);
}

static void set_mode(uint8_t mode) {
    ESP_ERROR_CHECK(write_b(mode));
}

static void write_data_bus(uint8_t data, uint8_t mode) {
    ESP_ERROR_CHECK(write_b(mode | 0x02)); // set enable pin
    ESP_ERROR_CHECK(write_a(data)); // write the data
    ESP_ERROR_CHECK(write_b(mode & 0x01)); // clear enable pin
}

static uint8_t clear_display(uint8_t current_mode) {
    if (current_mode != CONTROL_MODE) {
        set_mode(CONTROL_MODE);
    }
    write_data_bus(0x01, CONTROL_MODE);
    usleep(2000);
    return CONTROL_MODE;
}

static uint8_t set_cursor_position(uint8_t position, uint8_t current_mode) {
    if (current_mode != CONTROL_MODE) {
        set_mode(CONTROL_MODE);
    }
    if (position > 0x27) {
        position += (0x40 - 0x28);
    }
    write_data_bus(0x80 | position, CONTROL_MODE);
    return CONTROL_MODE;
}

static uint8_t init_display() {
    write_a(0x00); // ensure everything is cleared
    write_b(0x00);
    set_mode(CONTROL_MODE); 
    write_data_bus(0x30, CONTROL_MODE);
    usleep(10000); 
    write_data_bus(0x30, CONTROL_MODE);
    usleep(10000);
    write_data_bus(0x30, CONTROL_MODE);
    usleep(10000);
    write_data_bus(0x38, CONTROL_MODE); // set interface length to 8
    write_data_bus(0x10, CONTROL_MODE);
    write_data_bus(0x01, CONTROL_MODE);
    usleep(2000);
    write_data_bus(0x06, CONTROL_MODE);
    write_data_bus(0x0C, CONTROL_MODE);
    return CONTROL_MODE;
}

static void init_io_expander() {
    ESP_ERROR_CHECK(write_vfd(IODIRA_REG, 0x00));
    ESP_ERROR_CHECK(write_vfd(IODIRB_REG, 0x00));
}

static uint8_t write_str(char string[80], uint8_t current_mode) {
    if (current_mode != DATA_MODE) {
        set_mode(DATA_MODE);
    }
    for (uint8_t i = 0; i < 80; i++) {
        char c = string[i];
        if (c == '\0') {
            break;
        }
        write_data_bus(c, DATA_MODE);
    }
    return DATA_MODE;
}

static uint8_t fill(uint8_t current_mode) {
    if (current_mode != DATA_MODE) {
        set_mode(DATA_MODE);
    }
    for (uint8_t i = 0; i < 80; i++) {
        write_data_bus(0xFF, DATA_MODE);
    }
    return DATA_MODE;
}

static uint8_t all_written(uint8_t* written_chars, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        if (written_chars[i] == 0) {
            return 0;
        }
    }
    return 1;
}

uint8_t select_next_char(uint8_t* written_chars, uint8_t length) {
    uint8_t next_char = esp_random() % length;
    uint8_t first_char = next_char;
    while (written_chars[next_char]) {
        next_char = (next_char + 1) % length;
        if (next_char == first_char) {
            //*current_mode = clear_display(*current_mode);
            //write_str("All chars taken!!!", *current_mode);
            //usleep(3000000);
            return length;
        }
    }
    return next_char;
}

static uint8_t n_space(uint8_t n, uint8_t current_mode) {
    if (current_mode != DATA_MODE) {
        set_mode(DATA_MODE);
    }
    for (uint8_t i = 0; i < n; i++) {
        write_data_bus(' ', DATA_MODE);
    }
    return DATA_MODE;
}
        
static uint8_t matrix_write(char string[80], uint8_t current_mode, uint8_t speed, uint32_t delay) {
    char buffer[80];
    uint8_t length = strlen(string);
    uint8_t n_clear = 80 - length;
    uint8_t* written_chars = (uint8_t*)malloc(length);
    uint8_t* cleared_chars = (uint8_t*)malloc(n_clear);
    uint32_t n = 1;
    float increment = (float)n_clear / length;
    for (uint8_t i = 0; i < length; i++) {
        written_chars[i] = 0;
        cleared_chars[i] = 0;
    }
    for (uint8_t i = length; i < 80 - length; i++) {
        cleared_chars[i] = 0;
    }

    // until all the characters have been written, write another character surrounded by noise
    while (!all_written(written_chars, length)) {
        uint8_t do_update = n % speed;
        esp_fill_random(buffer, 80);
        if (do_update == 0) {
            uint8_t next_char = select_next_char(written_chars, length);
            written_chars[next_char] = 1;
            for (uint8_t i = 0; i < increment; i++) {
                uint8_t clear_position = select_next_char(cleared_chars, n_clear);
                if (clear_position < n_clear) {
                    cleared_chars[clear_position] = 1;
                } else {
                    break;
                }
            }
        }
        for (uint8_t i = 0; i < length; i++) {
            if (written_chars[i]) {
                buffer[i] = string[i];
            }
        }
        for (uint8_t i = 0; i < n_clear; i++) {
            if (cleared_chars[i]) {
                buffer[i + length] = ' ';
            }
        }
        usleep(delay);
        current_mode = set_cursor_position(0, current_mode);
        current_mode = write_str(buffer, current_mode);
        n += 1;
    }
    usleep(delay);
    current_mode = set_cursor_position(length, current_mode);
    current_mode = n_space(n_clear, current_mode);
    free(written_chars);
    free(cleared_chars);
    return current_mode;
}

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
