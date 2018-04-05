#include "lsm6ds33.h"

#include "systick.h"

/////////////////////////////////////////
// Register addresses                  //
/////////////////////////////////////////

#define REG_FUNC_CFG_ACCESS     0x01
#define REG_FIFO_CTRL1          0x06
#define REG_FIFO_CTRL2          0x07
#define REG_FIFO_CTRL3          0x08
#define REG_FIFO_CTRL4          0x09
#define REG_FIFO_CTRL5          0x0A
#define REG_WHO_AM_I			0x0F
#define REG_CTRL1_XL            0x10
#define REG_CTRL2_G             0x11
#define REG_CTRL3_C             0x12
#define REG_CTRL4_C             0x13
#define REG_CTRL5_C             0x14
#define REG_CTRL6_C             0x15
#define REG_CTRL7_G             0x16
#define REG_CTRL8_XL            0x17
#define REG_CTRL9_XL            0x18
#define REG_CTRL10_C            0x19
#define REG_STATUS  			0x1E
#define REG_OUT_TEMP_L          0x20
#define REG_OUT_TEMP_H          0x21
#define REG_OUTX_G_L            0x22
#define REG_OUTX_G_H            0x23
#define REG_OUTY_G_L            0x24
#define REG_OUTY_G_H            0x25
#define REG_OUTZ_G_L            0x26
#define REG_OUTZ_G_H            0x27
#define REG_OUTX_XL_L           0x28
#define REG_OUTX_XL_H           0x29
#define REG_OUTY_XL_L           0x2A
#define REG_OUTY_XL_H           0x2B
#define REG_OUTZ_XL_L           0x2C
#define REG_OUTZ_XL_H           0x2D
#define REG_FIFO_STATUS1        0x3A
#define REG_FIFO_STATUS2        0x3B
#define REG_FIFO_STATUS3        0x3C
#define REG_FIFO_STATUS4        0x3D
#define REG_FIFO_DATA_OUT_L     0x3E
#define REG_FIFO_DATA_OUT_H     0x3F


/////////////////////////////////////////
// Register values                     //
/////////////////////////////////////////

#define VALUE_WHO_AM_I		    0x69

// CTRL1_XL
#define VALUE_ODR_XL_SHIFT       4
#define VALUE_FS_XL_SHIFT        2
#define VALUE_BW_XL_SHIFT        0

// CTRL2_G
#define VALUE_ODR_G_SHIFT       4
#define VALUE_FS_G_SHIFT        1

// CTRL3_C
#define VALUE_BOOT              0x80
#define VALUE_BDU               0x40
#define VALUE_H_LACTIVE         0x20
#define VALUE_PP_OD             0x10
#define VALUE_SIM               0x08
#define VALUE_IF_INC            0x04
#define VALUE_BLE               0x02
#define VALUE_SW_RESET          0x01



bool LSM6DS33::initialize() {
    uint8_t who_am_i;
    if (!readRegister(REG_WHO_AM_I, who_am_i)) return false;
    return who_am_i == VALUE_WHO_AM_I;
}
/*
void LSM6DS33::reset(osr_t osr) {
    uint8_t ctrl_reg1 = ((osr & VALUE_OS_MASK) << VALUE_OS_SHIFT); 
    //ctrl_reg1 |= VALUE_RST; // Trigger software reset
	writeRegister(REG_CTRL_REG1, ctrl_reg1); //Barometer mode, standby
    delay(10);

    // Enable data ready event flags
    writeRegister(REG_PT_DATA_CFG, VALUE_PDEFE | VALUE_TDEFE);
	//enterStandby();
}

void LSM6DS33::enterStandby(){
    uint8_t current;
    readRegister(REG_CTRL_REG1, current);
	writeRegister(REG_CTRL_REG1, current & ~(VALUE_SBYB));
    delay(10);
}

void LSM6DS33::exitStandby(){
    uint8_t current;
    readRegister(REG_CTRL_REG1, current);
	writeRegister(REG_CTRL_REG1, current | (VALUE_SBYB));
    delay(10);
}

void LSM6DS33::start() {
	
}

void LSM6DS33::trigger(){	
	uint8_t current;
    readRegister(REG_CTRL_REG1, current);
	writeRegister(REG_CTRL_REG1, (current | VALUE_OST));
}

bool LSM6DS33::dataReady() {
    uint8_t status;
    readRegister(REG_DR_STATUS, status);
	return (status & (VALUE_PDR | VALUE_TDR)) == (VALUE_PDR | VALUE_TDR);
    //return (status & (VALUE_PDR));
}

void LSM6DS33::readPressure_28q4(uint32_t &pressure) {
    uint8_t p_bytes[3];
    readRegister(REG_OUT_P_MSB, p_bytes, 3);
    pressure = ((p_bytes[0] << 16) | (p_bytes[1] << 8) | (p_bytes[2] & 0xF0)) >> 2;
}

void LSM6DS33::readTemperature_12q4(int16_t &temperature) {
    uint8_t t_bytes[3];
    readRegister(REG_OUT_T_MSB, t_bytes, 2);
    temperature = (int16_t)((t_bytes[0] << 8) | (t_bytes[1] & 0xF0)) >> 4;
}

void LSM6DS33::setOSR(osr_t osr) {
	uint8_t current;
    readRegister(REG_CTRL_REG1, current);
	writeRegister(REG_CTRL_REG1, current | ((osr & VALUE_OS_MASK) << VALUE_OS_SHIFT));
}
*/