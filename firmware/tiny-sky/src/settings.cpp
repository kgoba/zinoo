#include "settings.h"

#include <libopencm3/stm32/flash.h>

#include <cstring>

#include "systick.h"

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

    mag_y_min           = 0;
    mag_y_max           = 0;
}

int eeprom_write(uint32_t address, uint32_t *data, int length_in_words)
{
    // Wait until FLASH is not busy
    uint32_t time_start;

    time_start = millis();
    while (FLASH_SR & FLASH_SR_BSY) {
        if (millis() - time_start > 100) return -1;
    }

	if ((FLASH_PECR & FLASH_PECR_PELOCK) != 0) {
        flash_unlock_pecr();	
        if (FLASH_PECR & FLASH_PECR_PELOCK) return -2;
    }

	/* erase only if needed */
	//FLASH_PECR &= ~FLASH_PECR_FTDW;
	for (int i = 0; i < length_in_words; i++) {
		MMIO32(address + (i * sizeof(uint32_t))) = data[i];
        
        time_start = millis();
		while (FLASH_SR & FLASH_SR_BSY) {
            if (millis() - time_start > 100) return -1;
        }
	}

    //delay(100);
	flash_lock_pecr();
    return 0;
}

void AppSettings::save() {
    // address must point to EEPROM space (begins at 0x0808 0000)
    // void eeprom_program_word(uint32_t address, uint32_t data);
    // void eeprom_program_words(uint32_t address, uint32_t *data, int length_in_words);

    uint32_t addr_eeprom = 0x08080000 + 0;
    const uint32_t n_words = (3 + sizeof(AppSettings)) / 4;

    eeprom_write(addr_eeprom, (uint32_t *)(this), n_words);
}

void AppSettings::restore() {
    // address must point to EEPROM space (begins at 0x0808 0000)
    // void eeprom_program_word(uint32_t address, uint32_t data);
    // void eeprom_program_words(uint32_t address, uint32_t *data, int length_in_words);

    //eeprom_program_words(0x08080000 + 0, (uint32_t *)(this), (3 + sizeof(AppSettings)) / 4);
    uint32_t addr_eeprom = 0x08080000 + 0;
    const uint32_t n_bytes = sizeof(AppSettings);

    memcpy((void *)(this), (const void *)addr_eeprom, n_bytes);
}
