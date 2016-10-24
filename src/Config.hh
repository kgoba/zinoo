

/******** PINOUT ********

Digital extension bus
   PD2     RADIO_EN
   PD3     FSK_CMOS
   PD4
   PD5     GPS_TXO
   PD6
   PD7
Digital/Analog extension bus
   ADC0/PC0
   ADC1/PC1
   ADC2/PC2
   ADC3/PC3
Analog internal
   ADC6    ADC_UNREG
   ADC7    ADC_NTC
Load switches
   PB0     RELEASE
   PB1     BUZZER (OC1A)
MicroSD Card
   PB2     SS_SD
   PB3     MOSI
   PB4     MISO
   PB5     SCK
I2C Bus
   PC4     SDA
   PC5     SCL

************************/


/********** I2C devices ************
  0x1D	  Linear acc. sensor LSM303D
  0x38    UV sensor VEML6070
  0x39    UV sensor VEML6070
  0x40    Humidity sensor HTU21D
  0x77    Barometer MS5607-02BA03

 *************************/

const int kFSKBaudrate = 300;     // FSK baudrate
const int kMeasureInterval = 4;   // seconds between sensor updates
const int kGPSLockTimeout = 60;
