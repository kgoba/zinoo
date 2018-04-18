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
    uint16_t    pyro_min_voltage;   // minimum pyro supply voltage, millivolts

    uint16_t    log_mag_interval;   // period of magnetic sensor log, milliseconds
    uint16_t    log_acc_interval;   // period of accelerometer log, milliseconds
    uint16_t    log_gyro_interval;  // period of gyro/acc sensor log, milliseconds
    uint16_t    log_baro_interval;  // period of barometric sensor log, milliseconds

    uint8_t     ublox_platform_type;  // portable/airborne 1g/etc

    char        must_be_zero;

    void reset();
    int save();
    int restore();
};

struct AppCalibration {
    // Non-user-editable settings (calibration data)
    int16_t     mag_y_min;
    int16_t     mag_y_max;
    int16_t     mag_temp_offset_q4;
    int16_t     baro_temp_offset_q4;

    char        must_be_zero;

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
    
    int16_t  last_temp_baro;    // q4 format (x16)
    int16_t  last_temp_mag;     // q4 format (x16)
    int16_t  last_temp_gyro;    // q4 format (x16)

    int      last_mx, last_my, last_mz;
    int      last_wx, last_wy, last_wz;
    int      last_ax, last_ay, last_az;

    int16_t  last_v_batt;       // millivolts
    int16_t  last_v_pyro;       // millivolts
    int16_t  last_vdd;          // millivolts
    int16_t  last_pyro_sense1;  // millivolts
    int16_t  last_pyro_sense2;  // millivolts

    bool     is_pyro1_on;

    uint8_t  buzz_errors;
    uint8_t  buzz_mode;

    TeleMessage telemetry;

    enum State {
        eSAFE,
        eARMED,
        eFLIGHT,
        eRECOVERY
    };

    State   state;

    //lfs_t               lfs;    
    //lfs_file_t          log_file;
    bool                log_file_ok;
    uint32_t            log_size;

    GPSParserSimple     gps;

    //BufferedSerial<USART<USART1, PB_6, PB_7>, 200, 64> usart_gps;
    SerialGPS                   usart_gps;

    SPI<SPI1, PB_5, PB_4, PB_3> bus_spi;
    I2C<I2C1, PB_9, PB_8>       bus_i2c;

    ADC<ADC1>           adc;

    DigitalOut<PB_12>   statusLED;
    DigitalOut<PB_15>   buzzerOn;

    DigitalOut<PB_0>    pyro1On;
    DigitalOut<PB_1>    pyro2On;

    AnalogIn<PA_0>      adc_v_batt;
    AnalogIn<PA_1>      adc_v_pyro;
    AnalogIn<PA_4>      adc_pyro_sense1;
    AnalogIn<PA_5>      adc_pyro_sense2;

    DigitalIn<PA_7>     arm_sense;

    DigitalOut<PC_13>   loraDIO2;
    DigitalOut<PA_9>    loraRST;
    DigitalOut<PA_10>   loraSS;

    DigitalOut<PA_15>   flashSS;

    RFM96               radio;
    SPIFlash            flash;

    MAG3110             mag;
    MPL3115             baro;
    LSM6DS33            gyro;

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
    systime_t task_control(systime_t due_time);
};

extern AppCalibration   gCalibration;
extern AppSettings      gSettings;
extern AppState         gState;

int eeprom_write(uint32_t address, uint32_t *data, int length_in_words);
