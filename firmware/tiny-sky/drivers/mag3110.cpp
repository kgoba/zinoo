#include "mag3110.h"

#include "systick.h"

/////////////////////////////////////////
// MAG3110 Magnetometer Registers      //
/////////////////////////////////////////
#define MAG3110_DR_STATUS			0x00
#define MAG3110_OUT_X_MSB			0x01
#define MAG3110_OUT_X_LSB			0x02
#define MAG3110_OUT_Y_MSB			0x03
#define MAG3110_OUT_Y_LSB			0x04
#define MAG3110_OUT_Z_MSB			0x05
#define MAG3110_OUT_Z_LSB			0x06
#define MAG3110_WHO_AM_I			0x07
#define MAG3110_SYSMOD				0x08
#define MAG3110_OFF_X_MSB			0x09
#define MAG3110_OFF_X_LSB			0x0A
#define MAG3110_OFF_Y_MSB			0x0B
#define MAG3110_OFF_Y_LSB			0x0C
#define MAG3110_OFF_Z_MSB			0x0D
#define MAG3110_OFF_Z_LSB			0x0E
#define MAG3110_DIE_TEMP			0x0F
#define MAG3110_CTRL_REG1			0x10
#define MAG3110_CTRL_REG2			0x11

////////////////////////////////
// MAG3110 WHO_AM_I Response  //
////////////////////////////////
#define MAG3110_WHO_AM_I_RSP		0xC4

/////////////////////////////////////////
// MAG3110 Commands and Settings       //
/////////////////////////////////////////

//CTRL_REG1 Settings
//Output Data Rate/Oversample Settings
//DR_OS_80_16 -> Output Data Rate = 80Hz, Oversampling Ratio = 16

#define MAG3110_DR_OS_80_16 		0x00
#define MAG3110_DR_OS_40_32 		0x08
#define MAG3110_DR_OS_20_64 		0x10
#define MAG3110_DR_OS_10_128		0x18
#define MAG3110_DR_OS_40_16			0x20
#define MAG3110_DR_OS_20_32			0x28
#define MAG3110_DR_OS_10_64			0x30
#define MAG3110_DR_OS_5_128			0x38
#define MAG3110_DR_OS_20_16			0x40
#define MAG3110_DR_OS_10_32			0x48
#define MAG3110_DR_OS_5_64			0x50
#define MAG3110_DR_OS_2_5_128		0x58
#define MAG3110_DR_OS_10_16			0x60
#define MAG3110_DR_OS_5_32			0x68
#define MAG3110_DR_OS_2_5_64		0x70
#define MAG3110_DR_OS_1_25_128		0x78
#define MAG3110_DR_OS_5_16			0x80
#define MAG3110_DR_OS_2_5_32		0x88
#define	MAG3110_DR_OS_1_25_64		0x90
#define MAG3110_DR_OS_0_63_128		0x98
#define MAG3110_DR_OS_2_5_16		0xA0
#define MAG3110_DR_OS_1_25_32		0xA8
#define MAG3110_DR_OS_0_63_64		0xB0
#define MAG3110_DR_OS_0_31_128		0xB8
#define MAG3110_DR_OS_1_25_16		0xC0
#define MAG3110_DR_OS_0_63_32		0xC8
#define MAG3110_DR_OS_0_31_64		0xD0
#define MAG3110_DR_OS_0_16_128		0xD8
#define MAG3110_DR_OS_0_63_16		0xE0
#define MAG3110_DR_OS_0_31_32		0xE8
#define MAG3110_DR_OS_0_16_64		0xF0
#define MAG3110_DR_OS_0_08_128		0xF8

//Other CTRL_REG1 Settings
#define MAG3110_FAST_READ 			0x04
#define MAG3110_TRIGGER_MEASUREMENT	0x02
#define MAG3110_ACTIVE_MODE			0x01
#define MAG3110_STANDBY_MODE		0x00

//CTRL_REG2 Settings
#define MAG3110_AUTO_MRST_EN		0x80
#define MAG3110_RAW_MODE			0x20
#define MAG3110_NORMAL_MODE			0x00
#define MAG3110_MAG_RST				0x10

//SYSMOD Readings
#define MAG3110_SYSMOD_STANDBY		0x00
#define MAG3110_SYSMOD_ACTIVE_RAW	0x01
#define	MAG3110_SYSMOD_ACTIVE		0x02


bool MAG3110::initialize() {
    uint8_t who_am_i;
    if (!readRegister(MAG3110_WHO_AM_I, who_am_i)) return false;
    return who_am_i == MAG3110_WHO_AM_I_RSP;
}

void MAG3110::reset(rate_t rate, osr_t osr) {
	enterStandby();

    uint8_t DROS = ((uint8_t)rate << 5) | ((uint8_t)osr << 3);
	writeRegister(MAG3110_CTRL_REG1, DROS); //Write DROS, everything else 0
	writeRegister(MAG3110_CTRL_REG2, 0x80); //Enable Auto Mag Reset, non-raw mode

	setOffset(eX_AXIS, 0);
	setOffset(eY_AXIS, 0);
	setOffset(eZ_AXIS, 0);
}

void MAG3110::enterStandby(){
	uint8_t current;
    readRegister(MAG3110_CTRL_REG1, current);
	//Clear bits to enter low power standby mode
    current &= ~(MAG3110_ACTIVE_MODE | MAG3110_TRIGGER_MEASUREMENT);
	writeRegister(MAG3110_CTRL_REG1, current);
    delay(10);
}

void MAG3110::exitStandby(){
	uint8_t current;
    readRegister(MAG3110_CTRL_REG1, current);
    current |= MAG3110_ACTIVE_MODE;
	writeRegister(MAG3110_CTRL_REG1, current);
    delay(10);
}

void MAG3110::start() {
	exitStandby();
}

//Note that AUTO_MRST_EN will always read back as 0
//Therefore we must explicitly set this bit every time we modify CTRL_REG2
void MAG3110::rawData(bool raw){
	if(raw) //Turn on raw (non-user corrected) mode
	{
		writeRegister(MAG3110_CTRL_REG2, MAG3110_AUTO_MRST_EN | (0x01 << 5) );
	}
	else //Turn off raw mode
	{
		writeRegister(MAG3110_CTRL_REG2, MAG3110_AUTO_MRST_EN & ~(0x01 << 5));
	}
}

void MAG3110::trigger(){	
	uint8_t current;
    readRegister(MAG3110_CTRL_REG1, current);
	writeRegister(MAG3110_CTRL_REG1, (current | 0x02));
}

bool MAG3110::dataReady() {
    uint8_t status;
    readRegister(MAG3110_DR_STATUS, status);
	return ((status & 0x8) >> 3);
}

//This is private because you must read each axis for the data ready bit to be cleared
//It may be confusing for casual users
int16_t MAG3110::readAxis(uint8_t axis){
	uint8_t lsbAddress, msbAddress;
	uint8_t lsb, msb;
	
	msbAddress = axis;
	lsbAddress = axis+1;
	
	if (!readRegister(msbAddress, msb)) return 0;
	
	//delayMicroseconds(2); //needs at least 1.3us free time between start and stop
	
	if (!readRegister(lsbAddress, lsb)) return 0;
	
	int16_t out = (lsb | (msb << 8)); //concatenate the MSB and LSB
	return out;
}

bool MAG3110::readMag(int16_t &x, int16_t &y, int16_t &z) {
	//Read each axis
	x = readAxis(eX_AXIS);
	y = readAxis(eY_AXIS);
	z = readAxis(eZ_AXIS);
	return true; // FIXME
}

bool MAG3110::readMicroTeslas(float &x, float &y, float &z) {
	//Read each axis and scale to Teslas
	x = readAxis(eX_AXIS) * 0.1f;
	y = readAxis(eY_AXIS) * 0.1f;
	z = readAxis(eZ_AXIS) * 0.1f;
	return true; // FIXME
}

void MAG3110::setRate(rate_t rate, osr_t osr) {
    //Get the current control register
    uint8_t DROS = ((uint8_t)rate << 5) | ((uint8_t)osr << 3);
	uint8_t current;
    readRegister(MAG3110_CTRL_REG1, current);
    current = (current & 0x07) | DROS; //And chop off the 5 MSB  with new DR_OS set
	writeRegister(MAG3110_CTRL_REG1, current); //Write back the register
}

//If you look at the datasheet, the offset registers are kind of strange
//The offset is stored in the most significant 15 bits.
//Bit 0 of the LSB register is always 0 for some reason...
//So we have to left shift the values by 1
//Ask me how confused I was...
void MAG3110::setOffset(axis_t axis, int16_t offset){		
	offset = offset << 1;
	// uint8_t data[] = {
	// 	(uint8_t)((int)axis + 8),
	// 	(uint8_t)(offset >> 8),
	// 	(uint8_t)(offset >> 0)
	// };

	// write(data, 3);

	writeRegister((int)axis + 8, offset >> 8);
	writeRegister((int)axis + 9, offset >> 0);
}

//See above
int16_t MAG3110::readOffset(axis_t axis){
	return (readAxis((uint8_t)axis+8)) >> 1;
}

bool MAG3110::readTemperature(int &temp) {
	int8_t tmp;
    if (readRegister(MAG3110_DIE_TEMP, (uint8_t &)tmp)) {
		temp = tmp;
		return true;
	}
	return false;
}
