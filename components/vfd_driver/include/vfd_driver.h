#ifndef VFD_DRIVER
#define VFD_DRIVER

#include <unistd.h>
#include <stdio.h>
#include "driver/i2c.h"

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

// init i2c device to talk to the IO expander
esp_err_t i2c_master_init(void);

// write a command to the IO expander
esp_err_t write_vfd(uint8_t reg_addr, uint8_t data);

// write to port a of the io expander (data bus)
esp_err_t write_a(uint8_t data);

// write to port b of the io expander (enable, rs pins)
esp_err_t write_b(uint8_t data);

// or unset the rs pin
void set_mode(uint8_t mode);

// write to the data bus of the vfd using the io expander
void write_data_bus(uint8_t data, uint8_t mode);

// send the command to clear the display. this requires waiting 2000us
uint8_t clear_display(uint8_t current_mode);

// set the cursor position of the vfd
uint8_t set_cursor_position(uint8_t position, uint8_t current_mode);

// run initialization commands. this takes ~50ms
uint8_t init_display();

// set the directions of the GPIO ports of the io expander
void init_io_expander();

// write a string to the vfd
uint8_t write_str(char string[80], uint8_t current_mode);

// turn every pixel on. this is for testing worst case power draw
uint8_t fill(uint8_t current_mode);

// helper method for matrix_write
uint8_t all_written(uint8_t* written_chars, uint8_t length);

// write n spaces to the display from the current cursor position. helper method for matrix write
uint8_t n_space(uint8_t n, uint8_t current_mode);

// helper method for matrix write
uint8_t select_next_char(uint8_t* written_chars, uint8_t length);

// cool "matrix-like" writing animation. random characters are written as the message is
uint8_t matrix_write(char string[80], uint8_t current_mode, uint8_t speed, uint32_t delay);

#endif
