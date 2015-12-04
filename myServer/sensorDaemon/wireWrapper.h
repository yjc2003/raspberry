#ifndef _WIRINGPI
#define _WIRINGPI
#include <wiringPi.h>

class WiringPin {
private:
    int pin;
public:
    WiringPin(int pin);
    WiringPin(void);
    void setPin(int pin);
    int getPin(void);
    void showPin(void);
    void setPinMode(int mode);
    void digitalWrite(int value);   
    int digitalRead(void);
};
#endif