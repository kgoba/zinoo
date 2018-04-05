#include "settings.h"

#include <libopencm3/stm32/flash.h>

void AppSettings::reset() {
    ublox_platform_type = (uint8_t)eDYN_AIRBORNE_1G;
    radio_callsign[0]   = 'T';
    radio_callsign[1]   = 'S';
    radio_callsign[2]   = 'T';
    radio_callsign[3]   = '\0';
    radio_tx_period   = 5;
    radio_tx_start    = 0;
    radio_frequency   = 434.25f * 1000000;
    radio_tx_power    = 5;
    // radio_modem_type    = 0;
    // radio_bandwidth     = 125 * 1E3;
    pyro_safe_time      = 120;
    pyro_safe_altitude  = 50;
    pyro_active_time    = 3000;
    log_mag_interval    = 1000 / 20;
    log_acc_interval    = 1000 / 100;
    log_gyro_interval   = 1000 / 100;
    log_baro_interval   = 1000 / 10;

    mag_y_offset        = 0;
}

void AppSettings::save() {
    // address must point to EEPROM space (begins at 0x0808 0000)
    // void eeprom_program_word(uint32_t address, uint32_t data);
    // void eeprom_program_words(uint32_t address, uint32_t *data, int length_in_words);

    eeprom_program_words(0x08080000 + 0, (uint32_t *)(this), (3 + sizeof(AppSettings)) / 4);
}

void AppSettings::restore() {
    // address must point to EEPROM space (begins at 0x0808 0000)
    // void eeprom_program_word(uint32_t address, uint32_t data);
    // void eeprom_program_words(uint32_t address, uint32_t *data, int length_in_words);

    //eeprom_program_words(0x08080000 + 0, (uint32_t *)(this), (3 + sizeof(AppSettings)) / 4);
}
