#include <stdint.h>

#include "systick.h"
#include "ublox.h"
#include "telemetry.h"

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

    void reset();
    void save();
    void restore();

    //static AppSettings object;
};

// Runtime application state that is shared between tasks
struct AppState {
    bool    gps_raw_mode;

    bool    baro_initialized;
    bool    mag_initialized; 
    bool    gyro_initialized;

    bool    mag_cal_enabled;
    systime_t mag_cal_start;

    uint32_t last_baro_time;
    uint32_t last_mag_time;
    uint32_t last_gyro_time;

    uint32_t last_pressure;
    int16_t  last_temp_baro;
    int      last_temp_mag;
    int      last_temp_gyro;
    int      last_mx, last_my, last_mz;
    int      last_wx, last_wy, last_wz;
    int      last_ax, last_ay, last_az;

    bool     is_armed;

    TeleMessage telemetry;
};

int eeprom_write(uint32_t address, uint32_t *data, int length_in_words);
