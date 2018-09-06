#pragma once

#include <stdint.h>

void xlog_arm(uint32_t timestamp, uint8_t hour, uint8_t minute, uint8_t second);
void xlog_pos(uint32_t timestamp, float latitude, float longitude, float altitude);
void xlog_mag(uint32_t timestamp, int16_t mag_x, int16_t temp_q4);
void xlog_baro(uint32_t timestamp, uint32_t pressure_q4, int16_t temp_q4);
void xlog_gyro();
void xlog_eject(uint32_t timestamp);

int xlog_init();

int xlog_free_space();
int xlog_used_space();

int xlog_erase_log();

int eeprom_write(uint32_t address, uint32_t *data, int length_in_words);
int extflash_write(uint32_t address, const uint8_t *buffer, int size);
int extflash_read(uint32_t address, uint8_t *buffer, int size);

