#include "lsm6ds33.h"

//#include "systick.h"

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

// STATUS
#define VALUE_XLDA              (1 << 0)
#define VALUE_GDA               (1 << 1)
#define VALUE_TDA               (1 << 2)

// CTRL1_XL
#define VALUE_ODR_XL_SHIFT      4
#define VALUE_FS_XL_SHIFT       2
#define VALUE_BW_XL_SHIFT       0
#define VALUE_ODR_XL_MASK       0xF0
#define VALUE_FS_XL_MASK        0x0C
#define VALUE_BW_XL_MASK        0x03

// CTRL2_G
#define VALUE_ODR_G_SHIFT       4
#define VALUE_FS_G_SHIFT        1
#define VALUE_ODR_G_MASK        0xF0
#define VALUE_FS_G_MASK         0x0E

// CTRL3_C
#define VALUE_BOOT              0x80
#define VALUE_BDU               0x40
#define VALUE_H_LACTIVE         0x20
#define VALUE_PP_OD             0x10
#define VALUE_SIM               0x08
#define VALUE_IF_INC            0x04
#define VALUE_BLE               0x02
#define VALUE_SW_RESET          0x01

// CTRL4_C
#define VALUE_XL_BW_SCAL_ODR    0x80
#define VALUE_SLEEP_G           0x40
#define VALUE_DRDY_MASK         0x08

// CTRL7_G
#define VALUE_HP_G_EN           0x40


bool LSM6DS33::initialize() {
    uint8_t who_am_i;
    if (!readRegister(REG_WHO_AM_I, who_am_i)) return false;
    if (who_am_i != VALUE_WHO_AM_I) return false;

    uint8_t ctrl_reg3;
    readRegister(REG_CTRL3_C, ctrl_reg3);
    ctrl_reg3 |= VALUE_BDU | VALUE_IF_INC;
    writeRegister(REG_CTRL3_C, ctrl_reg3);
}

void LSM6DS33::reset() {
}

void LSM6DS33::trigger() {
}

bool LSM6DS33::dataReady(bool gyro, bool accel) {
    uint8_t status;
    readRegister(REG_STATUS, status);
    if (gyro && !(status & VALUE_GDA)) return false;
    if (accel && !(status & VALUE_XLDA)) return false;
    return true;
}

void LSM6DS33::readMeasurement(int16_t &gx, int16_t &gy, int16_t &gz, int16_t &ax, int16_t &ay, int16_t &az) {
    uint8_t data[12];
    readRegister(REG_OUTX_G_L, data, sizeof(data));
    gx = data[0] | (data[1] << 8);
    gy = data[2] | (data[3] << 8);
    gz = data[4] | (data[5] << 8);
    ax = data[6] | (data[7] << 8);
    ay = data[8] | (data[9] << 8);
    az = data[10] | (data[11] << 8);
}

void LSM6DS33::readTemperature_12q4(int16_t &temperature) {
    uint8_t data[2];
    readRegister(REG_OUT_TEMP_L, data, sizeof(data));
    temperature = (int16_t)(data[0] | (data[1] << 8));
}

// void LSM6DS33::setODR(odr_xl_t odr_xl, odr_g_t odr_g) {
//     setGyroODR(odr_g);
//     setAccelODR(odr_xl);
// }

void LSM6DS33::setupGyro(odr_g_t odr_g, fs_g_t fs_g) {
	uint8_t reg = 0;
    //readRegister(REG_CTRL2_G, reg);
    //reg &= ~VALUE_ODR_G_MASK;
    reg |= (odr_g << VALUE_ODR_G_SHIFT) & VALUE_ODR_G_MASK;
    reg |= (fs_g << VALUE_FS_G_SHIFT) & VALUE_FS_G_MASK;
	writeRegister(REG_CTRL2_G, reg);
}

void LSM6DS33::setupAccel(odr_xl_t odr_xl, fs_xl_t fs_xl) {
    uint8_t reg = 0;
    readRegister(REG_CTRL1_XL, reg);
    reg &= VALUE_BW_XL_MASK;
    reg |= (odr_xl << VALUE_ODR_XL_SHIFT) & VALUE_ODR_XL_MASK;
    reg |= (fs_xl << VALUE_FS_XL_SHIFT) & VALUE_FS_XL_MASK;
	writeRegister(REG_CTRL1_XL, reg);
}
