#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/debug.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "inc/hw_gpio.h"
#include "driverlib/rom.h"
#include "driverlib/adc.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#define PWM_FREQUENCY 20000
volatile uint32_t ui32Load;
volatile uint32_t ui32PWMClock;
volatile uint32_t speedCrl;
volatile double result;
volatile double ui32Period;
volatile double preLogic;
volatile double i;
volatile double curLogic;
volatile double count;
volatile double initial_timer_value;
volatile double final_timer_value;
volatile double period;
volatile double rpm;
volatile int rounded;
volatile int A;
volatile int CD;
volatile int BCD;
volatile int B;
volatile int C;
volatile int D;
//void sevenSeg(int Input);
int main(void)
{
    //PWM variables
    // "volatile" means will stay in memory no matter what
    speedCrl = 50;
    //System Clock 40MHZ
    SysCtlClockSet(
    SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);  // Enabling for the PWM output
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD); // on PD0 (Port D pin 0)

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // enable the buttons PF0(SW2) and PF4(SW1)

    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0); //Configure Port D, Pin 0
    GPIOPinConfigure(GPIO_PD0_M1PWM0);        //Make PWM output pin for module 1

    // Configure the buttons on Port F pin 0 and pin 4
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;
    GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0, GPIO_DIR_MODE_IN);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0,
    GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);

    ui32PWMClock = SysCtlClockGet() / 64;  //PWM clock at 625kHz
    ui32Load = (ui32PWMClock / PWM_FREQUENCY) - 1;
    PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, ui32Load);

    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, speedCrl * ui32Load / 1000); //set PWM
    PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true);
    PWMGenEnable(PWM1_BASE, PWM_GEN_0); //PWM module 1, generator 0 needs to be enabled as output to run

    /* enable clocks */
    SYSCTL_RCGCGPIO_R |= 0x10; /* enable clock to GPIOE (AIN0 is on PE3) */
    SYSCTL_RCGCADC_R |= 1; /* enable clock to ADC0 */

    /* initialize PE3 for AIN0 input  */
    GPIO_PORTE_AFSEL_R |= 8; /* enable alternate function */
    GPIO_PORTE_DEN_R &= ~8; /* disable digital function */
    GPIO_PORTE_AMSEL_R |= 8; /* enable analog function */

    /* initialize ADC0 */
    ADC0_ACTSS_R &= ~8; /* disable SS3 during configuration */
    ADC0_EMUX_R &= ~0xF000; /* software trigger conversion */
    ADC0_SSMUX3_R = 0; /* get input from channel 0 */
    ADC0_SSCTL3_R |= 6; /* take one sample at a time, set flag at 1st sample */
    ADC0_ACTSS_R |= 8; /* enable ADC0 sequencer 3 */

    //Timer Config. Periodic mode.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    //Calculate timer delay/frequency. SysCtlClockGet() = 1 second
    ui32Period = SysCtlClockGet() * 2;
    //Set Timer_0A to the above frequency. Minus 1 because it count to 0.
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period - 1);

    ADC0_PSSI_R |= 8; /* start a conversion sequence 3 */
    while ((ADC0_RIS_R & 0x08) == 0)
        ; /* wait for conversion complete */
    result = ADC0_SSFIFO3_R; /* read conversion result */
    ADC0_ISC_R = 8; /* clear completion flag */

    //This if-else is here to initialize our period calculation
    if (result > 3000)
    {
        preLogic = 0;
    }
    else if (result < 1000)
    {
        preLogic = 1;
    }
    //GPIO
//    SysCtlClockSet(
//                SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ
//                        | SYSCTL_OSC_MAIN); // set up the clock
//        //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
//        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
//        //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
//        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
//        //SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
//
//        //
//        // Enable the GPIO port that is used for the on-board LED.
//        //
//
//        GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7);
//        //GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
//        GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_0);
//        //GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);

    while (1)
    {

        //Read value from the fan's sensor and save it to 'result'
        ADC0_PSSI_R |= 8; /* start a conversion sequence 3 */
        while ((ADC0_RIS_R & 0x08) == 0)
            ; /* wait for conversion complete */
        result = ADC0_SSFIFO3_R; /* read conversion result */
        ADC0_ISC_R = 8; /* clear completion flag */

        //Convert result to 0 and 1.
        if (result > 3000)
        {
            curLogic = 0;
        }
        else if (result < 1000)
        {
            curLogic = 1;
        }

        //Check if the signal change compare to previous loop.
        if (curLogic != preLogic)
        {
            //if it does. Check the edge count.
            if (count == 0)
            {
                //if it is the first edge. Enable timer and record the timer value
                TimerEnable(TIMER0_BASE, TIMER_A);
                initial_timer_value = TimerValueGet(TIMER0_BASE, TIMER_A);
            }
            //Increase count by 1
            count++;
            if (count == 5)
            {
                //if it is the final edge of a period. Record the timer value.
                final_timer_value = TimerValueGet(TIMER0_BASE, TIMER_A);
                //Reset the count variable.
                count = 0;
                //calculate period in clock cycle and rpm.
                period = (initial_timer_value - final_timer_value);
                rpm = round(60 / (period / SysCtlClockGet()));
                //this var is not use for anything. Will be deleted later.
                rounded = rpm;
                //reset timer
                TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period - 1);
            }

        }
        if (curLogic == preLogic && speedCrl == 750)
        {
            rounded = 0;
        }
        //Reset the sensor result for next loop.
        if (result > 3000)
        {
            preLogic = 0;
        }
        else if (result < 1000)
        {
            preLogic = 1;
        }
        D = rounded % 10;
        CD = rounded % 100;
        BCD = rounded % 1000;
        C = (CD - D) / 10;
        B = (BCD - CD) / 100;
        A = (rounded - BCD) / 1000;
       // sevenSeg(D);

//BCD

        if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00) //press SW1 DECREASE PW - min 1.0 mS
        {
            speedCrl--;
            if (speedCrl < 45)
            {
                speedCrl = 45;
            }
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, speedCrl * ui32Load / 1000);
        }
        if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00) //press SW2 INCREASE PW - max 2.0 mS
        {
            speedCrl++;
            if (speedCrl > 750)
            {
                speedCrl = 750;
            }
            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, speedCrl * ui32Load / 1000);
        }
        SysCtlDelay(9000);
        //Break the rpm value into digits.
        //Delay for the loop to account for button bouncing. However it will affect the accuracy of our calculation. Therefore we use a very low delay and very high step number.

    }
}

//void sevenSeg(int Input)
//{
//    switch (Input)
//    {
//    case 0:
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0x00);
//        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
//        break;
//    case 1:
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
//        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
//        break;
//    case 2:
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0x00);
//        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, GPIO_PIN_0);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
//        break;
//    case 3:
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
//        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, GPIO_PIN_0);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
//        break;
//    case 4:
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0x00);
//        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_PIN_7);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
//        break;
//    case 5:
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
//        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_PIN_7);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
//        break;
//    case 6:
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0x00);
//        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, GPIO_PIN_0);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_PIN_7);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
//        break;
//    case 7:
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
//        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, GPIO_PIN_0);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, GPIO_PIN_7);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, 0x00);
//        break;
//    case 8:
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0x00);
//        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, GPIO_PIN_6);
//        break;
//    case 9:
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2);
//        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_7, 0x00);
//        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_6, GPIO_PIN_6);
//        break;
//
//    }
//
//}
//
