#include <stdint.h>
#include "tm4c123gh6pm.h"
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
void sevenSeg(int Input);

//*****************************************************************************

// GPIO Write - HIGH= GPIO_PIN_X , LOW= 0x00
//
//*****************************************************************************
int main(void)
{

    while (1)
    {
        // GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_0,GPIO_PIN_0); //write HIGH
        // GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_1,0x00);       //write LOW
        // GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_2,0x00);
        // GPIOPinWrite(GPIO_PORTD_BASE,GPIO_PIN_3,GPIO_PIN_3);
        //GPIOPinWrite(GPIO_PORTB_BASE,GPIO_PIN_0,GPIO_PIN_0);
        //GPIOPinWrite(GPIO_PORTE_BASE,GPIO_PIN_2,GPIO_PIN_2);
        sevenSeg(1);
       // GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
        //GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
        //GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
        //GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
    }

}

void sevenSeg(int Input)
{
    switch (Input)
    {
    case 0:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
        break;
    case 1:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
        break;
    case 2:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, GPIO_PIN_0);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
        break;
    case 3:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, GPIO_PIN_0);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
        break;
    case 4:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_PIN_7);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
        break;
    case 5:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_PIN_7);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
        break;
    case 6:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, GPIO_PIN_0);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_PIN_7);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
        break;
    case 7:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, GPIO_PIN_0);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_PIN_7);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
        break;
    case 8:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, GPIO_PIN_6);
        break;
    case 9:
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, GPIO_PIN_6);
        break;

    }

}
