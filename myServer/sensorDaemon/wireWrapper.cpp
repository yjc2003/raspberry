#include "wireWrapper.h"
#include <stdio.h>

WiringPin::WiringPin(int pin)
{
    this->pin = pin;
}
WiringPin::WiringPin(void)
{
    this->pin = -1;
}
void WiringPin::setPin(int pin)
{
    this->pin = pin;
}
int WiringPin::getPin(void)
{
    return this->pin;
}
void WiringPin::showPin(void)
{
    printf("Pin : %d\n", this->pin);
}
void WiringPin::setPinMode(int mode)
{
    if(getPin() == -1) 
    {
        printf("Pin Error\n");
        return;
    }
    switch(mode)
    {
        case OUTPUT:
        case INPUT:
        case PWM_OUTPUT:
            pinMode(getPin(), mode);
            break;
        default:
            printf("Mode %d Is not Defined\n", mode);
            break;
    }
}
void WiringPin::digitalWrite(int value)
{
    if(getPin() == -1) 
    {
        printf("Pin Error\n");
        return;
    }
    switch(value)
    {
        case 1:
        case 0:
            ::digitalWrite (getPin(), value);
            break;
        default:
            printf("value %d Is not Defined\n", value);
            break;
    }
}


int WiringPin::digitalRead(void)
{
    if(getPin() == -1) 
    {
        printf("Pin Error\n");
        return -1;
    }
    return ::digitalRead(getPin());
}



