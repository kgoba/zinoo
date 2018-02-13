#include <stdint.h>

// Class to keep track of our last known status (GPS data)
struct Status {
    uint16_t msg_id;

    bool     fixValid;
    float    lat, lng;  // Last valid coordinates
    uint16_t alt;       // Last valid altitude
    uint8_t  n_sats;    // Current satellites
    int8_t   temperature_ext;   // External temperature, Celsius
    
    uint8_t  switch_state;

    uint16_t msg_recv;
    int8_t   rssi_last;

    float    pyro_voltage;

    void restore();
    void save();

    bool build_string(char *buf, uint8_t buf_len);
};
