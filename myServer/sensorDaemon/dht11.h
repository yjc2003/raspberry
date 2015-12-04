#ifndef _DHT11
#define _DHT11
#include <wiringPi.h>
#include "WiringPi.h"

// 1wire DHT error status
typedef enum
{
	FAIL			= 0x00,
	PASS			= 0x01,
	TIME_OUT_1		= 0xE1,
	TIME_OUT_2		= 0xE2,
	TIME_OUT_3		= 0xE3, //227
	TIME_OUT_4		= 0xE4, //228
	TIME_OUT_5		= 0xE5, //229
	ERROR_CHECK_SUM = 0xC1

} DHT11_Status;

#define MAX_TIME 85


// DHT11 data -> 40bit
#define MAX_BIT 40
// [0] : 8bit integral RH data
// [1] : 8bit decimal RH data
// [2] : 8bit integral T data
// [3] : 8bit decimal T data
// [4] : 8bit check sum


class DHT11 {

private :
	
	WiringPin m_nDHT_Pin;
	double m_dHumidity;
	double m_dTemperatureC;
	double m_dTemperatureF;

public:
	unsigned char m_cDHT_Value[5];
	DHT11(int pin);
	void setDHTPin(int pin);
	int getDHTPin(void);
	bool checkPinUpKeep(int status, int upkeepdelay);
	int requestDHT11(void);
	int responseDHT11(void);
	bool InitDHTValue(void);
	double getDHTHumidity(void);
	double getDHTTemperatureC(void);
	double getDHTTemperatureF(void);
	void setDHTHumidity(double humidity);
	void setDHTTemperatureC(double temp);
	void setDHTTemperatureF(double temp);
	bool verifyDHTValue(void);
	bool calculateDHTValue(void);
};
#endif