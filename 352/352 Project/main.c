#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"
#define PWM_FREQUENCY 20000

//variables with "volatile" so compiler doesn't eliminate them.
volatile uint32_t ui32Load;
volatile uint32_t ui32PWMClock;
volatile uint32_t speedCrl; //variable to hold different PWM signals

int main(void)
{
    speedCrl = 30;             // 83 is the center position to create a 1.5mS pulse from PWM

    /*
    How they got 83:
    Since PWM_FREQUENCY = 55HZ and T(Period) = 18.2mS
    Pulse Resolution = 18.2mS / 1000 = 1.82 uS.
    Pulse Resolution * 83 = 1.51mS PULSE WIDTH

    Other Resolutions are acceptable provided that they produce a 1.5mS pulse-width
    Important: Make sure your numbers will fit within the 16-bit registers.
    */

    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN| SYSCTL_XTAL_16MHZ);
    SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);  // Enabling for the PWM output
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // on PD0 (Port D pin 0)

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // enable the buttons PF0(SW2) and PF4(SW1)

    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0); //Configure Port D, Pin 0
    GPIOPinConfigure(GPIO_PD0_M1PWM0);           //Make PWM output pin for module 1

    // Configure the buttons on Port F pin 0 and pin 4
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
    GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0,GPIO_DIR_MODE_IN);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0,GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    ui32PWMClock = SysCtlClockGet() / 64;  //PWM clock at 625kHz
    ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
    PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, ui32Load);

    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, speedCrl * ui32Load / 1000);  //set PWM
    PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true);
    PWMGenEnable(PWM1_BASE, PWM_GEN_0);   //PWM module 1, generator 0 needs to be enabled as output to run

    while (1)
    {
        if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00)  //press SW1 DECREASE PW - min 1.0 mS
        {
            speedCrl--;
            if (speedCrl < 30)
            {
                speedCrl = 30;
            }
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, speedCrl * ui32Load / 1000);
        }
        if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00)  //press SW2 INCREASE PW - max 2.0 mS
        {
            speedCrl++;
            if (speedCrl > 1000)
            {
                speedCrl = 1000;
            }
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, speedCrl * ui32Load / 1000);
        }
        SysCtlDelay(80000);   //speed of the loop (in microseconds?)
    }
}
