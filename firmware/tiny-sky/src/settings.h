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

// struct ParamBase {
//     virtual int setStrValue(const char *str);
//     virtual int getStrValue(char *str, int &length);

//     virtual int getBinaryLength();
//     virtual int getBinaryValue(uint8_t *buf, int &length);
//     virtual int setBinaryValue(const uint8_t *buf);
// };

// template<int n_bytes>
// struct IntParam : public ParamBase {
//     IntParam();
//     //int value;
// };

// template<int n_bytes>
// struct UIntParam : public ParamBase {
//     UIntParam();
//     //int value;
// };

// template<int n_chars>
// struct StrParam : public ParamBase {
//     StrParam(const char *default_value);
//     //char value[n_chars];
// };

// struct Test {
//     struct param_descriptor_t {
//         const char  *name;
//         ParamBase   &value;
//     };
//     StrParam<16> radio_callsign;
//     UIntParam<4> radio_frequency;
//     UIntParam<4> radio_cw_frequency;
//     IntParam<1>  radio_tx_power;
//     UIntParam<2> radio_tx_period;
//     UIntParam<2> radio_tx_start;

//     UIntParam<2> pyro_safe_time;
//     UIntParam<2> pyro_safe_altitude;
//     UIntParam<2> pyro_active_time;
//     UIntParam<2> pyro_min_voltage;

//     UIntParam<2> log_mag_interval;   // period of magnetic sensor log, milliseconds
//     UIntParam<2> log_acc_interval;   // period of accelerometer log, milliseconds
//     UIntParam<2> log_gyro_interval;  // period of gyro/acc sensor log, milliseconds
//     UIntParam<2> log_baro_interval;  // period of barometric sensor log, milliseconds

//     UIntParam<1> ublox_platform_type;  // portable/airborne 1g/etc

//     param_descriptor_t params[2];

//     Test();

//     int findByName(const char *name);
//     int setValue(int id, const char *str);
//     int getValue(int id, char *str, int &length);

//     void reset();
// };


// To be stored in NVM
struct AppSettings {
    enum param_type_t {
        PARAM_INT,
        PARAM_STR
    };

    struct param_descriptor_t {
        const char  *name;
        param_type_t type;
        int8_t       size;
        void        *value;
    };

    param_descriptor_t params[8];

    // User-editable settings
    char        radio_callsign[16];
    uint32_t    radio_frequency;    // channel center frequency in MHz
    uint32_t    radio_cw_frequency; // CW frequency in MHz
    int8_t      radio_tx_power;     // transmit power in dBm
    uint8_t     radio_tx_period;    // telemetry transmit period, seconds
    uint8_t     radio_tx_start;     // telemetry transmit offset, seconds
    //uint8_t     radio_modem_type;   // LORA vs FSK etc
    //uint32_t    radio_bandwidth;    // channel bandwidth in Hz

    uint16_t    pyro_safe_time;     // seconds after arming
    uint16_t    pyro_safe_altitude; // meters above launch altitude
    uint16_t    pyro_hold_time;     // time of MOSFET on state, milliseconds
    uint16_t    pyro_min_voltage;   // minimum pyro supply voltage, millivolts

    uint16_t    log_mag_interval;   // period of magnetic sensor log, milliseconds
    uint16_t    log_acc_interval;   // period of accelerometer log, milliseconds
    uint16_t    log_gyro_interval;  // period of gyro/acc sensor log, milliseconds
    uint16_t    log_baro_interval;  // period of barometric sensor log, milliseconds

    uint8_t     ublox_platform_type;  // portable/airborne 1g/etc

    // Sensor calibration data
    int16_t     gyro_temp_offset_q4;

    char        must_be_zero;       // will read 0xFF from uninitialized NVM

    AppSettings();

    void reset();
    int save();
    int restore();

    int getCount();
    int findByName(const char *name);
    int setValue(int id, const char *str);
    int getValue(int id, char *str, int &length);
};

struct AppCalibration {
    // Non-user-editable settings (calibration data)
    int16_t     mag_x_offset;
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

    int16_t  last_mx, last_my, last_mz;
    int16_t  last_wx, last_wy, last_wz;
    int16_t  last_ax, last_ay, last_az;

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
