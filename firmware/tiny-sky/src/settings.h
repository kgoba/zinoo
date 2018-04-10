#include <stdint.h>

#include "systick.h"
#include "ublox.h"
#include "telemetry.h"

#include "gps.h"
#include "serial.h"

#include "mag3110.h"
#include "mpl3115.h"
#include "lsm6ds33.h"
#include "rfm96.h"

extern "C" {
    #include "lfs.h"
}

// To be stored in NVM
struct AppSettings {
    // User-editable settings
    char        radio_callsign[16];
    uint8_t     radio_tx_period;  // telemetry transmit period, seconds
    uint8_t     radio_tx_start;   // telemetry transmit offset, seconds
    uint32_t    radio_frequency;    // channel frequency in MHz
    int8_t      radio_tx_power;     // transmit power in dBm
    //uint8_t     radio_modem_type;   // LORA vs FSK etc
    //uint32_t    radio_bandwidth;    // channel bandwidth in Hz

    uint16_t    pyro_safe_time;     // seconds after arming
    uint16_t    pyro_safe_altitude; // meters above launch altitude
    uint16_t    pyro_active_time;   // time of MOSFET on state, milliseconds

    uint16_t    log_mag_interval;   // period of magnetic sensor log, milliseconds
    uint16_t    log_acc_interval;   // period of accelerometer log, milliseconds
    uint16_t    log_gyro_interval;  // period of gyro/acc sensor log, milliseconds
    uint16_t    log_baro_interval;  // period of barometric sensor log, milliseconds

    uint8_t     ublox_platform_type;  // portable/airborne 1g/etc

    // Non-user-editable settings (calibration data)
    int16_t     mag_y_min;
    int16_t     mag_y_max;
    int16_t     mag_temp_offset_q4;
    int16_t     baro_temp_offset_q4;

    void reset();
    int save();
    int restore();
};

// Runtime application state that is shared between tasks
struct AppState {
    uint8_t gps_raw_mode;

    bool    baro_initialized;
    bool    mag_initialized; 
    bool    gyro_initialized;

    bool    mag_cal_enabled;
    systime_t mag_cal_start;

    uint32_t last_time_baro;
    uint32_t last_time_mag;
    uint32_t last_time_gyro;

    uint32_t last_pressure;
    
    int16_t  last_temp_baro;
    int16_t  last_temp_mag;
    int16_t  last_temp_gyro;

    int      last_mx, last_my, last_mz;
    int      last_wx, last_wy, last_wz;
    int      last_ax, last_ay, last_az;

    bool     is_armed;

    TeleMessage telemetry;

    lfs_t    lfs;

    //BufferedSerial<USART<USART1, PB_6, PB_7>, 200, 64> usart_gps;
    SerialGPS   usart_gps;

    I2C<I2C1, PB_9, PB_8> bus_i2c;

    MAG3110     mag;
    MPL3115     baro;
    LSM6DS33    gyro;

    DigitalOut<PB_12> statusLED;
    DigitalOut<PB_15> buzzerOn;

    DigitalOut<PB_0>  pyro1On;
    DigitalOut<PB_1>  pyro2On;

    SPI_T<SPI1, PB_5, PB_4, PB_3> bus_spi;
    DigitalOut<PA_9>  loraRST;
    DigitalOut<PA_10> loraSS;

    DigitalOut<PA_15> flashSS;

    GPSParserSimple gps;

    RFM96 radio;

    SPIFlash    flash;

    AppState() : mag(bus_i2c), baro(bus_i2c), gyro(bus_i2c) {}

    int init_lfs();
    void buzz_times(int times);
    int free_space();

    int init_hw();
    int init_periph();

    systime_t task_gps(systime_t due_time);
    systime_t task_console(systime_t due_time);
    systime_t task_report(systime_t due_time);
    systime_t task_led(systime_t due_time);
    systime_t task_buzz(systime_t due_time);
    systime_t task_sensors(systime_t due_time);
};

extern AppSettings gSettings;
extern AppState    gState;

int eeprom_write(uint32_t address, uint32_t *data, int length_in_words);
