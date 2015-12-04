#include "dht11.h"
#include <stdio.h>
#include <string.h>


DHT11::DHT11(int pin)
{
    this->m_nDHT_Pin.setPin(pin);
}
bool DHT11::InitDHTValue(void)
{
    memset(this->m_cDHT_Value, 0, sizeof(this->m_cDHT_Value));
	return true;
}

void DHT11::setDHTPin(int pin)
{
	this->m_nDHT_Pin.setPin(pin);
}

int DHT11::getDHTPin(void)
{
	return this->m_nDHT_Pin.getPin();
}

double DHT11::getDHTHumidity(void)
{
	return this->m_dHumidity;
}

double DHT11::getDHTTemperatureC(void)
{
	return this->m_dTemperatureC;
}

double DHT11::getDHTTemperatureF(void)
{
	return this->m_dTemperatureF;
}

void DHT11::setDHTHumidity(double humidity)
{
	this->m_dHumidity = humidity;
}

void DHT11::setDHTTemperatureC(double temp)
{
	this->m_dTemperatureC = temp;
}

void DHT11::setDHTTemperatureF(double temp)
{
	this->m_dTemperatureF = temp;
}

bool DHT11::checkPinUpKeep(int status, int upkeepdelay)
{
	int delay = 0;
	while(this->m_nDHT_Pin.digitalRead() == status)
	{
		delayMicroseconds(1);
		if (++delay >= upkeepdelay) return true;
	}

	return false;
}

// MCU Sends out Start Signal to DHT
int DHT11::requestDHT11(void)
{
	this->m_nDHT_Pin.setPinMode(OUTPUT);
    
	// 1wire init- to HIGH
	this->m_nDHT_Pin.digitalWrite(HIGH);
	delay(1); 

	// MCU sends out start siganal and pull down voltage for at least 18ms to let DHT11 detect the signal
	this->m_nDHT_Pin.digitalWrite(LOW);
	delay(18);

	// MCU pulls up voltage and waits for DHT response(20~40us)
	this->m_nDHT_Pin.digitalWrite(HIGH);
	this->m_nDHT_Pin.setPinMode(INPUT);

	// check : high -> low
	if(checkPinUpKeep(HIGH, 255) == true) return TIME_OUT_1;

	// DHT sends out response signal & keeps it for 80us
	// check : low -> high
	if(checkPinUpKeep(LOW, 255) == true) return TIME_OUT_2;

	// DHT pulls up voltage & keeps it for 80us
	// check : high -> low
	if(checkPinUpKeep(HIGH, 255) == true) return TIME_OUT_3;

	return PASS;
}



int DHT11::responseDHT11(void)
{
	InitDHTValue();

	if(checkPinUpKeep(LOW, 255) == true) return TIME_OUT_4;
	
	// Start data transmission
	for(int bit = 0 ; bit < MAX_BIT ; bit++)  
    {  
    	m_cDHT_Value[bit / 8] <<= 1;
        if(checkPinUpKeep(HIGH, 16) == true) 
		{
			m_cDHT_Value[bit / 8] |= HIGH;
			checkPinUpKeep(HIGH, 255);
        }
		checkPinUpKeep(LOW, 255);
    }  

	return PASS;
}

bool DHT11::verifyDHTValue(void)
{
	int checksum = 0;

	for(int i = 0; i < 4 ; i++)
	{
		checksum += m_cDHT_Value[i];
	}

	if(checksum == 0 && m_cDHT_Value[4] == 0) 	return false;
	if(checksum != m_cDHT_Value[4])				return false;

	return true;
}

bool DHT11::calculateDHTValue(void)
{
	if(verifyDHTValue() == false) return false;

	setDHTHumidity((double)m_cDHT_Value[0]);
	setDHTTemperatureC((double)m_cDHT_Value[2]);
	setDHTTemperatureF((double)(getDHTTemperatureC() * 9. /5. + 32));
	return true;	
}

