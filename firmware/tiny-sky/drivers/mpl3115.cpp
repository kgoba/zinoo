#include "mpl3115.h"

#include "systick.h"

/////////////////////////////////////////
// MPL3115 Magnetometer Registers      //
/////////////////////////////////////////
#define REG_STATUS  			0x00
#define REG_OUT_P_MSB			0x01
#define REG_OUT_P_CSB			0x02
#define REG_OUT_P_LSB			0x03
#define REG_OUT_T_MSB			0x04
#define REG_OUT_T_LSB			0x05
#define REG_DR_STATUS			0x06
#define REG_OUT_PD_MSB			0x07
#define REG_OUT_PD_CSB			0x08
#define REG_OUT_PD_LSB			0x09
#define REG_OUT_TD_MSB			0x0A
#define REG_OUT_TD_LSB			0x0B
#define REG_WHO_AM_I			0x0C
#define REG_F_STATUS			0x0D
#define REG_F_DATA  			0x0E
#define REG_F_SETUP  			0x0F
#define REG_TIME_DLY  			0x10
#define REG_SYSMOD   			0x11
#define REG_INT_SOURCE 			0x12
#define REG_PT_DATA_CFG 		0x13
#define REG_BAR_IN_MSB			0x14
#define REG_BAR_IN_LSB			0x15
#define REG_P_TGT_MSB			0x16
#define REG_P_TGT_LSB			0x17
#define REG_T_TGT    			0x18
#define REG_P_WND_MSB			0x19
#define REG_P_WND_LSB			0x1A
#define REG_T_WND    			0x1B
#define REG_P_MIN_MSB			0x1C
#define REG_P_MIN_CSB			0x1D
#define REG_P_MIN_LSB			0x1E
#define REG_T_MIN_MSB			0x1F
#define REG_T_MIN_LSB			0x20
#define REG_P_MAX_MSB			0x21
#define REG_P_MAX_CSB			0x22
#define REG_P_MAX_LSB			0x23
#define REG_T_MAX_MSB			0x24
#define REG_T_MAX_LSB			0x25
#define REG_CTRL_REG1			0x26
#define REG_CTRL_REG2			0x27
#define REG_CTRL_REG3			0x28
#define REG_CTRL_REG4			0x29
#define REG_CTRL_REG5			0x2A
#define REG_OFF_P    			0x2B
#define REG_OFF_T    			0x2C
#define REG_OFF_H    			0x2D

////////////////////////////////
// MPL3115 WHO_AM_I Response  //
////////////////////////////////
#define VALUE_WHO_AM_I_RSP		0xC4

/////////////////////////////////////////
// MPL3115 Register values             //
/////////////////////////////////////////

//DR_STATUS settings
#define VALUE_PDR                 0x04
#define VALUE_TDR                 0x02

//PT_DATA_CFG settings
#define VALUE_TDEFE                 0x01
#define VALUE_PDEFE                 0x02
#define VALUE_DREM                  0x04

//SYSMOD settings
#define VALUE_SYSMOD_ACTIVE       0x01

//CTRL_REG1 Settings
#define VALUE_SBYB                0x01
#define VALUE_OST                 0x02
#define VALUE_RST                 0x04
#define VALUE_OS_SHIFT            3
#define VALUE_OS_MASK             0x07
#define VALUE_RAW                 0x40
#define VALUE_ALT                 0x80

//CTRL_REG2 Settings
#define VALUE_ST_SHIFT	    	0
#define VALUE_ST_MASK	    	    0x0F
#define VALUE_ALARM_SEL			0x10
#define VALUE_LOAD_OUTPUT			0x20


bool MPL3115::initialize() {
    uint8_t who_am_i;
    if (!readRegister(REG_WHO_AM_I, who_am_i)) return false;
    return who_am_i == VALUE_WHO_AM_I_RSP;
}

void MPL3115::reset(osr_t osr) {
    uint8_t ctrl_reg1 = ((osr & VALUE_OS_MASK) << VALUE_OS_SHIFT); 
    //ctrl_reg1 |= VALUE_RST; // Trigger software reset
	writeRegister(REG_CTRL_REG1, ctrl_reg1); //Barometer mode, standby
    delay(10);

    // Enable data ready event flags
    writeRegister(REG_PT_DATA_CFG, VALUE_PDEFE | VALUE_TDEFE);
	//enterStandby();
}

void MPL3115::enterStandby(){
    uint8_t current;
    readRegister(REG_CTRL_REG1, current);
	writeRegister(REG_CTRL_REG1, current & ~(VALUE_SBYB));
    delay(10);
}

void MPL3115::exitStandby(){
    uint8_t current;
    readRegister(REG_CTRL_REG1, current);
	writeRegister(REG_CTRL_REG1, current | (VALUE_SBYB));
    delay(10);
}

void MPL3115::start() {
	
}

void MPL3115::trigger(){	
	uint8_t current;
    readRegister(REG_CTRL_REG1, current);
	writeRegister(REG_CTRL_REG1, (current | VALUE_OST));
}

bool MPL3115::dataReady() {
    uint8_t status;
    readRegister(REG_DR_STATUS, status);
	return (status & (VALUE_PDR | VALUE_TDR)) == (VALUE_PDR | VALUE_TDR);
    //return (status & (VALUE_PDR));
}

void MPL3115::readPressure_u28q4(uint32_t &pressure) {
    uint8_t p_bytes[3];
    readRegister(REG_OUT_P_MSB, p_bytes, 3);
    pressure = ((p_bytes[0] << 16) | (p_bytes[1] << 8) | (p_bytes[2] & 0xF0)) >> 2;
}

void MPL3115::readTemperature_12q4(int16_t &temperature) {
    uint8_t t_bytes[3];
    readRegister(REG_OUT_T_MSB, t_bytes, 2);
    temperature = (int16_t)((t_bytes[0] << 8) | (t_bytes[1] & 0xF0)) >> 4;
}

void MPL3115::setOSR(osr_t osr) {
	uint8_t current;
    readRegister(REG_CTRL_REG1, current);
	writeRegister(REG_CTRL_REG1, current | ((osr & VALUE_OS_MASK) << VALUE_OS_SHIFT));
}
