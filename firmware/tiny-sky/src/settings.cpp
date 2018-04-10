#include "settings.h"

#include <libopencm3/stm32/flash.h>

#include <cstring>

#include "systick.h"
#include "console.h"

extern "C" {
#include "cdcacm.h"
}


// Global shared variables
AppSettings gSettings;
AppState    gState;


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
    mag_temp_offset_q4  = -12.0f * 16;
    baro_temp_offset_q4 =   0.5f * 16;
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
	FLASH_PECR &= ~FLASH_PECR_FTDW;
	for (int i = 0; i < length_in_words; i++) {
		MMIO32(address + (i * sizeof(uint32_t))) = data[i];
        
        time_start = millis();
		while (FLASH_SR & FLASH_SR_BSY) {
            if (millis() - time_start > 100) return -1;
        }
	}

    delay(100);
	flash_lock_pecr();
    return 0;
}

int AppSettings::save() {
    // address must point to EEPROM space (begins at 0x0808 0000)
    // void eeprom_program_word(uint32_t address, uint32_t data);
    // void eeprom_program_words(uint32_t address, uint32_t *data, int length_in_words);

    // uint32_t addr_eeprom = 0x08080000 + 0;
    // const uint32_t n_words = (3 + sizeof(AppSettings)) / 4;

    // eeprom_write(addr_eeprom, (uint32_t *)(this), n_words);

    lfs_file_t  file;
    int         err;

    // Attempt to open settings file
    // TODO: do we need the LFS_O_TRUNC?
    err = lfs_file_open(&gState.lfs, &file, "settings", LFS_O_WRONLY | LFS_O_TRUNC | LFS_O_CREAT);
    if (err) {
        return -1;
    }

    err = lfs_file_write(&gState.lfs, &file, (const void *)this, sizeof(AppSettings));
    lfs_file_close(&gState.lfs, &file);

    // // TODO: check err and do something about it (retry?)
    if (err < 0) return -2;
    return 0;
}

int AppSettings::restore() {
    reset();

    lfs_file_t  file;
    int         err;

    // Attempt to open settings file
    err = lfs_file_open(&gState.lfs, &file, "settings", LFS_O_RDONLY);  // LFS_O_CREAT
    if (err) {
        reset();
        // File not open, so just return
        return -1;
    }

    err = lfs_file_read(&gState.lfs, &file, (void *)this, sizeof(AppSettings));
    lfs_file_close(&gState.lfs, &file);

    if (err < 0) {
        // Reading failed 
        reset();
        return -2;
    }

    // if (err < sizeof(AppSettings))   // or we read too few bytes

    return 0;

    // // address must point to EEPROM space (begins at 0x0808 0000)
    // // void eeprom_program_word(uint32_t address, uint32_t data);
    // // void eeprom_program_words(uint32_t address, uint32_t *data, int length_in_words);

    // //eeprom_program_words(0x08080000 + 0, (uint32_t *)(this), (3 + sizeof(AppSettings)) / 4);
    // uint32_t addr_eeprom = 0x08080000 + 0;
    // const uint32_t n_bytes = sizeof(AppSettings);

    // memcpy((void *)(this), (const void *)addr_eeprom, n_bytes);
}


extern "C" {
    static int spi_flash_lfs_read(const struct lfs_config *c, lfs_block_t block, 
        lfs_off_t off, void *buffer, lfs_size_t size);

    static int spi_flash_lfs_prog(const struct lfs_config *c, lfs_block_t block, 
        lfs_off_t off, const void *buffer, lfs_size_t size);

    static int spi_flash_lfs_erase(const struct lfs_config *c, lfs_block_t block);

    static int spi_flash_lfs_sync(const struct lfs_config *c);
}


#define LFS_BLOCK_SIZE      4096
#define LFS_BLOCK_COUNT     512         // 16 Mbit

#define LFS_READ_SIZE       16
#define LFS_PROG_SIZE       LFS_READ_SIZE

uint8_t lfs_read_buffer[LFS_READ_SIZE];
uint8_t lfs_prog_buffer[LFS_PROG_SIZE];
uint8_t lfs_file_buffer[LFS_PROG_SIZE];
uint8_t lfs_lookahead_buffer[16];

// configuration of the filesystem is provided by this struct
const lfs_config lfs_cfg = {
    .context = 0,

    // block device operations
    .read  = spi_flash_lfs_read,
    .prog  = spi_flash_lfs_prog,
    .erase = spi_flash_lfs_erase,
    .sync  = spi_flash_lfs_sync,

    // block device configuration
    .read_size = LFS_READ_SIZE,    // Minimum size of a block read
    .prog_size = LFS_PROG_SIZE,    // Minimum size of a block program
    .block_size = LFS_BLOCK_SIZE,  // Size of an erasable block, N*prog_size
    .block_count = LFS_BLOCK_COUNT,// Number of erasable blocks on the device
    .lookahead = 128, // Number of blocks to lookahead during block allocation, N*32

    .read_buffer = lfs_read_buffer,
    .prog_buffer = lfs_prog_buffer,
    .lookahead_buffer = lfs_lookahead_buffer,
    .file_buffer = lfs_file_buffer
};

int AppState::init_lfs() {
    if (!flash.initialize()) return -1;
    delay(10);

    // mount the filesystem
    int err = lfs_mount(&lfs, &lfs_cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(&lfs, &lfs_cfg);
        err = lfs_mount(&lfs, &lfs_cfg);
        if (err) return -2;
    }

    return 0;
}

int AppState::init_hw() {
    // Initialize internal peripherals
    usb_setup();
    usb_enable_interrupts();

    usart_gps.begin();

    bus_i2c.begin();
    //bus_i2c.enableIRQ();
    //bus_i2c.enableSTOPInterrupt();
    //bus_i2c.enableNACKInterrupt();

    bus_spi.begin();
    bus_spi.format(8, 0);
    bus_spi.frequency(4000000);

    statusLED.begin();
    buzzerOn.begin();

    loraRST.begin(1);
    loraSS.begin(1);

    flashSS.begin(1);

    pyro1On.begin();    // TODO: leave uninitialized/floating for safety
    pyro2On.begin();

    return 0;    
}

int AppState::init_periph() {
    // Restore settings
    if (0 == init_lfs()) {
        gSettings.restore();
    } else {
        gSettings.reset();
    }

    // Configure PPS to flash at 5Hz w/o lock, 1Hz w/ lock
    ublox_cfg_tp5(5, 1, 1 << 31, 1 << 29, true, false);

    radio.init();
    radio.setupLoRa(RFM96::ModemSettings()
        .setSF(RFM96::eSF_10)
        .setCR(RFM96::eCR_4_8)
    );
    // radio.setupLoRa(RFM96::ModemSettings()
    //     .setSF(RFM96::eSF_9)
    //     .setBW(RFM96::eBW_31k25)
    //     .setCR(RFM96::eCR_4_8)
    // );
    radio.setFrequencyHz(gSettings.radio_frequency);
    radio.setTXPower(gSettings.radio_tx_power);

    // Make a beep to indicate startup
    statusLED = buzzerOn = 1;
    delay(500);
    statusLED = buzzerOn = 0;
}

void AppState::buzz_times(int times) {
    for (int i = 0; i < times; i++) {
        gState.buzzerOn = 1;
        delay(100);
        gState.buzzerOn = 0;
        delay(100);
    }
}

systime_t AppState::task_gps(systime_t due_time) {
    uint8_t ch;
    while (usart_gps.getc(ch)) {
        if (millis() - due_time > 10) break;
        if (gps_raw_mode == 1) {
            print("%02X ", ch);
        }
        else {
            if (gps_raw_mode == 2) {
                print(ch);
            }   
            gps.decode(ch);
        }
    }

    static bool platform_updated;
    if (!platform_updated && millis() > 5000) {
        // Set GNSS platform type (e.g. airborne 1g)
        ublox_set_dyn_mode((ublox_dyn_t)gSettings.ublox_platform_type);

        platform_updated = true;
    }

    // static bool fix_3d;
    // static bool fix_2d;
    // static uint32_t time_lost;

    // if (gps.fixType().atLeast2D()) {
    //     if (gps.fixType().is3D()) {
    //         if (!fix_3d) {
    //             print("Time to 3D fix: %ld seconds\n", (millis() - time_lost) / 1000);
    //             fix_3d = true;
    //             fix_2d = false;
    //             buzz_times(3);
    //         }
    //     } 
    //     else {
    //         if (!fix_2d) {
    //             print("Time to 2D fix: %ld seconds\n", (millis() - time_lost) / 1000);
    //             fix_2d = true;
    //             fix_3d = false;
    //             buzz_times(2);
    //         }
    //     }
    // }
    // else {
    //     if (fix_2d || fix_3d) {
    //         time_lost = millis();
    //         fix_2d = false;
    //         fix_3d = false;
    //         buzz_times(1);
    //     }
    // }

    return due_time + 100;
}

systime_t AppState::task_console(systime_t due_time) {
    static char rx_line[80];
    static int  rx_len;
    uint8_t ch;

    while (usb_cdc_read((uint8_t *)&ch, 1)) {
        if (millis() - due_time > 5) break;
        print(ch);
        if (rx_len < 79 && ch != 0x0A && ch != 0x0D) {
            rx_line[rx_len] = ch;
            rx_len++;
        }
        if (ch == 0x0A) {
            rx_line[rx_len] = '\0';
            if (rx_len > 0) {
                console_parse(rx_line);
                print("\r\n");
            } else {
                print_report();
            }
            print("> ");
            rx_len = 0;
        }
    }

    return due_time + 50;
}

systime_t AppState::task_report(systime_t due_time) {
    if (gps.fixTime().valid()) {
        uint8_t packet_data[64];
        int     packet_length = 64;
        
        memcpy(telemetry.callsign, gSettings.radio_callsign, 16);
        telemetry.msg_id++;
        if (telemetry.fixValid = gps.fixType().is3D()) {
            telemetry.lat = gps.latitude().degreesSignedFloat();
            telemetry.lng = gps.longitude().degreesSignedFloat();
            telemetry.alt = gps.altitude().meters();
        }
        telemetry.n_sats = gps.tracked().value();
        int temp_int = 0;
        int temp_cnt = 0;
        if (mag_initialized) {
            temp_int += last_temp_mag;
            temp_cnt++;
        }
        if (baro_initialized) {
            temp_int += last_temp_baro;
            temp_cnt++;
        }
        if (temp_cnt) temp_int /= temp_cnt;

        telemetry.temperature_int = (temp_int + 8) / 16;
        telemetry.hour = gps.fixTime().hour();
        telemetry.minute = gps.fixTime().minute();
        telemetry.second = gps.fixTime().second();

        if (telemetry.build_string((char *)packet_data, packet_length)) {
            print("Telemetry: [%s]\n", (const char *)packet_data);
            radio.writeFIFO(packet_data, packet_length);
            radio.startTX();
        }            
    }
    return due_time + 5000;
}

systime_t AppState::task_led(systime_t due_time) {
    static bool led_on;

    if (!led_on) {
        led_on = true;
        statusLED = 1;
        if (gps.fixType().atLeast2D()) {
            return due_time + 200;
        } else {
            return due_time + 100;
        }
    }
    else {
        led_on = false;
        statusLED = 0;
        if (gps.fixType().atLeast2D()) {
            return due_time + (gps.fixType().is3D() ? 800 : 300);
        } else {
            return due_time + 100;
        }
    }
}

systime_t AppState::task_buzz(systime_t due_time) {
    static bool buzzer_on;

    if (!buzzer_on) {
        buzzer_on = true;
        buzzerOn = 1;
        return due_time + 200;
    }
    else {
        buzzer_on = false;
        buzzerOn = 0;
        return due_time + 800;
    }
}

systime_t AppState::task_sensors(systime_t due_time) {
    static int      cal_my_min, cal_my_max;
    static int      cal_count;

    if (!baro_initialized) {
        if (baro.initialize()) {
            // Approx 50 ms sampling time
            baro.reset(MPL3115::eOSR_16);
            // temp offset -1.5 C
            baro_initialized = true;
            baro.trigger();
        }
    }
    else if (baro.dataReady()) {
        last_time_baro = millis();
        baro.readPressure_u28q4(last_pressure);
        int16_t temp;
        baro.readTemperature_12q4(temp);
        last_temp_baro = temp - gSettings.baro_temp_offset_q4;

        // trigger a new conversion
        baro.trigger();
    }

    if (!mag_initialized) {
        if (mag.initialize()) {
            // Approx 25 ms sampling time
            mag.reset(MAG3110::eRATE_1280, MAG3110::eOSR_32);      
            
            mag.setOffset(MAG3110::eY_AXIS, (gSettings.mag_y_min + gSettings.mag_y_max) / 2);
            mag.rawData(false); 
            
            mag_initialized = true;
            mag.trigger();
        }
    }
    else if (mag.dataReady()) {
        last_time_mag = millis();
        mag.readMag(last_mx, last_my, last_mz);
        int temp;
        mag.readTemperature(temp);
        last_temp_mag = (temp << 4) - gSettings.mag_temp_offset_q4;

        pyro1On = (last_my < 0);
        pyro2On = (last_my >= 0);

        if (mag_cal_enabled) {
            cal_count++;

            if (cal_count > 3) {
                if (last_my > cal_my_max) cal_my_max = last_my;
                if (last_my < cal_my_min) cal_my_min = last_my;
            }
            else {
                cal_my_max = cal_my_min = last_my;
            }

            if (cal_count % 8 == 0) {
                print("Y = %d\n", last_my);
            }
            if (millis() - mag_cal_start >= 10000) {
                gSettings.mag_y_min = cal_my_min;
                gSettings.mag_y_max = cal_my_max;
                print("Mag Y = [%d,%d]\n", gSettings.mag_y_min, gSettings.mag_y_max);
                //gSettings.save();

                mag.setOffset(MAG3110::eY_AXIS, (cal_my_min + cal_my_max) / 2);
                mag.rawData(false);

                mag_cal_enabled = false;
                cal_count = 0;

                buzz_times(4);
            }
        }

        // trigger a new conversion
        mag.trigger();
    }

    if (!gyro_initialized) {
        if (gyro.initialize()) {
            // Approx 5 ms sampling time?
            //gyro.reset(MPL3115::eOSR_16);
            gyro_initialized = true;
            //baro.trigger();
        }
    }

    return due_time + 100;
}


static int spi_flash_lfs_read(const struct lfs_config *c, lfs_block_t block, 
    lfs_off_t off, void *buffer, lfs_size_t size) 
{
    uint32_t address = (block * LFS_BLOCK_SIZE) | off;

    while (gState.flash.busy()) {
        // idle wait
    }
    gState.flash.read(address, (uint8_t *)buffer, size);
    return 0;
}

static int spi_flash_lfs_prog(const struct lfs_config *c, lfs_block_t block, 
    lfs_off_t off, const void *buffer, lfs_size_t size) 
{
    uint32_t address = (block * LFS_BLOCK_SIZE) | off;
    const uint8_t *wr_buf = (const uint8_t *)buffer;

    uint32_t page_remaining = 256 - (address & 0xFF);
    while (size > 0) {
        int wr_len = (size < page_remaining) ? size : page_remaining;
        while (gState.flash.busy()) {
            // idle wait
        }
        gState.flash.programPage(address, wr_buf, wr_len);
        address += wr_len;
        wr_buf += wr_len;
        size -= wr_len;
        page_remaining = 256;
    }
    return 0;
}

static int spi_flash_lfs_erase(const struct lfs_config *c, lfs_block_t block) 
{
    uint32_t address = (block * LFS_BLOCK_SIZE);

    while (gState.flash.busy()) {
        // idle wait
    }
    gState.flash.eraseSector(address);
    return 0;
}

static int spi_flash_lfs_sync(const struct lfs_config *c) {
    while (gState.flash.busy()) {
        // idle wait
    }
    // Nothing to sync here, indicate success
    return 0;
}

static int _traverse_df_cb(void *p, lfs_block_t block){
	uint32_t *nb = (uint32_t *)p;
	*nb += 1;
	return 0;
}

int AppState::free_space(){
	uint32_t n_allocated = 0;
	int err = lfs_traverse(&lfs, _traverse_df_cb, &n_allocated);
	if (err < 0) {
		return err;
	}

	uint32_t available = (lfs.cfg->block_count - n_allocated) * lfs.cfg->block_size;
	return available;
}