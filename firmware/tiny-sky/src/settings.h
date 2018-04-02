#include <stdint.h>

// struct ParamBase {
//     const char *name;
//     virtual void get_binary() {};
//     virtual void set_binary() {};
//     virtual void get_string() {};
//     virtual void set_string() {};
// };

// template<typename T>
// struct Parameter : ParamBase {
//     T value;

//     Parameter(T default_value, const char *name) : default_value(default_value), name(name) 
//     {
//     }

//     ParamBase *first_child;
//     ParamBase *next_sibling;
// };
// Parameter<uint8_t> g_p_t { .value = 0, .name = "gps_platform_type" };

struct EEPROMSettings {
    uint8_t     gps_platform_type;  // portable/airborne 1g/etc

    char        radio_callsign[16];
    uint8_t     radio_tx_period;    // telemetry transmit period, seconds
    int8_t      radio_tx_offset;    // telemetry transmit offset, seconds
    uint32_t    radio_frequency;    // channel frequency in Hz
    uint8_t     radio_modem_type;   // LORA vs FSK etc
    uint32_t    radio_bandwidth;    // channel bandwidth in Hz

    uint16_t    pyro_safe_time;     // seconds after arming
    uint16_t    pyro_safe_altitude; // meters above launch altitude
    uint16_t    pyro_active_time;   // time of MOSFET on state, milliseconds

    uint16_t    log_mag_interval;   // period of magnetic sensor log, milliseconds
    uint16_t    log_acc_interval;   // period of accelerometer log, milliseconds
    uint16_t    log_gyro_interval;  // period of gyro/acc sensor log, milliseconds
    uint16_t    log_baro_interval;  // period of barometric sensor log, milliseconds

    void reset() {
        gps_platform_type   = 0;
        radio_callsign[0]   = 'T';
        radio_callsign[1]   = 'S';
        radio_callsign[2]   = 'T';
        radio_callsign[3]   = '\0';
        radio_tx_period     = 5;
        radio_tx_offset     = 0;
        radio_frequency     = 433.25 * 1E6;
        radio_modem_type    = 0;
        radio_bandwidth     = 125 * 1E3;
        pyro_safe_time      = 120;
        pyro_safe_altitude  = 50;
        pyro_active_time    = 3000;
        log_mag_interval    = 1000 / 20;
        log_acc_interval    = 1000 / 100;
        log_gyro_interval   = 1000 / 100;
        log_baro_interval   = 1000 / 10;
    }
};