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
// "volatile" means will stay in memory no matter what
volatile double ui32Load;
volatile double ui32PWMClock;
volatile double speedIndicator; // Indicates PWM dutycycle. Theoretical motor max RPM occurs when close to 0, min at 700.
// The motor speed is proportional to the duty cycle of the PWM's 0 logic level
volatile double tachometer; // Readings from the tachometer
volatile double ui32Period;
volatile double preLogic;// Carries the previous logic value from the tachometer
volatile double curLogic;// Carries the current logic value from the tachometer
volatile double edge;       // Shows the number of edges detected
volatile double sample = 0; // Counts the number of sample RPMs for later averaging
volatile double initialTimer;   // Stores the time when the timer is triggered
volatile double finalTimer; // Stores the time when the timer is stoped
volatile double TS;     // Time length for four poles
volatile double rpm;        // One sample reading of the actual rpm
volatile double rpmAvg = 0; // The average rpm of every four sample rpms
volatile double rpmSum = 0; // The sum of four sample rpms, will be set to 0 once a average rpm is yield
volatile int fourDigitRPM;// The actual rpm, can show up to four digits. Theoretically from 0 to 2400.
volatile double desiredDutycycle = 0;// The desired duty cycle is calculated from the reference input voltage (potentiometer)
volatile int desiredRPM = 0;    // Can be calculated from the desired duty cycle
volatile double D_out = 0;  // ADC output
volatile double Error = 0;  // Error in the closed-loop system
volatile double Kp = 0.03;// Proportional gain constant, 0.09 was determined based on testing
volatile int A;         // Separate digits from the four digits RPM
volatile int B;         // Assume the four digits RPM to be ABCD
volatile int C; // A is the thousands; B is the hundreds; C is the tens; D is the units
volatile int D;
volatile int CD;
volatile int BCD;
// Functions for the seven segment output, A;B;C;D are the iuput of these functions.
void right(int Input);
void middle(int Input);
void left(int input);

int main(void)
{
    speedIndicator = 100;   //set initial speed to overcome the sratic friction
    //System Clock 40MHZ
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
    SYSCTL_XTAL_16MHZ);
    //PWM Clock is 40MHZ/64
    SysCtlPWMClockSet(SYSCTL_PWMDIV_64);

    //GPIO Initialization **************************************************************************************************
    //Enable Peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);    //peripheral A
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);    //peripheral B
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);    //peripheral C
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);    //peripheral D
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);    //peripheral E
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);    //peripheral F

    //Specify GPIO port and pin
    GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE,
    GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 |
    GPIO_PIN_6 | GPIO_PIN_7);
    GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_3 | GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE,
    GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 |
    GPIO_PIN_7);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE,
    GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
    GPIO_PIN_4 | GPIO_PIN_5);
    //GPIO Initialization **************************************************************************************************

    //Enable PWM module 1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    //GPIOD is the output pin/var for PWM. This line just enables it though. We will assign it later.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    //Enable the 2 switches (GPIOF, Pin 0 and 4 of array F)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    //Assign pin 0 of array D as the output for PWM
    GPIOPinTypePWM(GPIO_PORTD_BASE, GPIO_PIN_0);
    GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_3);
    //Specify pin D0 to the PWM_0, module 1.
    GPIOPinConfigure(GPIO_PD0_M1PWM0);

    //Unlock the switches, because they were Locked by default.
    HWREG (GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG (GPIO_PORTF_BASE + GPIO_O_CR) |= 0x01;
    HWREG (GPIO_PORTF_BASE + GPIO_O_LOCK) = 0;

    //Set these switches as input.
    GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0, GPIO_DIR_MODE_IN);
    //Check for when the switch is pressed or hold.
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4 | GPIO_PIN_0,
    GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);

    /* The PWM clock is SYSCLK/64 (set in step 12 above). Divide the PWM clock by the desired
     frequency  to determine the count to be loaded into the Load register. Then
     subtract 1 since the counter down-counts to zero. Configure module 1 PWM generator 0
     as a down-counter and load the count value
     Set PWM clock.*/

    ui32PWMClock = SysCtlClockGet() / 64;
    ui32Load = (ui32PWMClock / PWM_FREQUENCY);
    PWMGenConfigure(PWM1_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN);
    // Assign the value to count down from to PWM generator 0, module 1 (PWM_0 == PWM generator 0)
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0, ui32Load - 1);
    // Set the pulse width for PWM generator 0
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, speedIndicator * ui32Load / 1000);
    // Set the PWM generator 0 as output (to the pin D0)
    PWMOutputState(PWM1_BASE, PWM_OUT_0_BIT, true);
    // Start PWM generator 0, module 1.
    PWMGenEnable(PWM1_BASE, PWM_GEN_0);

    // ADC0 configuration
    // Enable clocks
    SYSCTL_RCGCGPIO_R |= 0x10;  // Enable clock to GPIO port E (AIN0 is on PE3)
    SYSCTL_RCGCADC_R |= 1;  // Enable clock to ADC0

    // Initialize PE3 for AIN0 input
    GPIO_PORTE_AFSEL_R |= 8;    // Enable alternate function
    GPIO_PORTE_DEN_R &= ~8; // Disable digital function
    GPIO_PORTE_AMSEL_R |= 8;    // Enable analog function

    // Initialize ADC0
    ADC0_ACTSS_R &= ~8;     // Disable ADC0 SS3 during configuration
    ADC0_EMUX_R &= ~0xF000; // Software trigger conversion
    ADC0_SSMUX3_R = 0;      // Get input from channel 0
    ADC0_SSCTL3_R |= 6; // Take one sample at a time, set flag at the first sample
    ADC0_ACTSS_R |= 8;      // Enable ADC0 sequencer 3
    // ADC0 configuration done

    // ADC1 configuration
    // Enable clocks
    SYSCTL_RCGCGPIO_R |= 0x10;  // Enable clock to GPIO port E (AIN0 is on PE3)
    SYSCTL_RCGCADC_R |= 2;  // Enable clock to ADC0

    // Initialize PE2 for AIN1 input
    GPIO_PORTE_AFSEL_R |= 4;    // Enable alternate function
    GPIO_PORTE_DEN_R &= ~4; // Disable digital function
    GPIO_PORTE_AMSEL_R |= 4;    // Enable analog function

    // Initialize ADC1
    ADC1_ACTSS_R &= ~8;     // Disable ADC1 SS3 during configuration
    ADC1_EMUX_R &= ~0xF000; // Software trigger conversion
    ADC1_SSMUX3_R = 1;      // Get input from channel 1
    ADC1_SSCTL3_R |= 6; // Take one sample at a time, set flag at the first sample
    ADC1_ACTSS_R |= 8;      // Enable ADC1 sequencer 3
    // ADC1 configuration done

    // Read values from ADC1
    ADC1_PSSI_R |= 8;       // Start a conversion sequence 3
    while ((ADC1_RIS_R & 0x08) == 0)
        ;               // Wait for conversion complete
    D_out = ADC1_SSFIFO3_R; // Read the conversion result, assign the result to 'D_out'
    ADC1_ISC_R = 8;     // Clear completion flag

    // Timer configuration
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    // Calculate timer delay/frequency.
    ui32Period = SysCtlClockGet() / 0.5;
    // Set Timer_0A to the above frequency.
    TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period - 1);
    ADC0_PSSI_R |= 8;       // Start a conversion at sequence 3
    while ((ADC0_RIS_R & 0x08) == 0)
        ;               // Wait for conversion to complete
    tachometer = ADC0_SSFIFO3_R;// Read the conversion result, assign the result to 'tachometer'
    ADC0_ISC_R = 8;     // Clear completion flag

    // Read tachometer value for the first time, and store the reading in 'preLogic'
    // Readings from the tachometer are either logic high or logic low
    // Thanks to ADC0 SS3, the logic values are converted to values theoretically equal to either 0 or 4095

    if (tachometer > 2500)// Set tolerance for this reading, all readings greater than 2500 are logic high
    {
        preLogic = 1;
    }
    else if (tachometer < 1500) // Set tolerance for this reading, all readings smaller than 1500 are logic low
    {
        preLogic = 0;
    }

    while (1)
    {

        // ********************Reading from ADC0*******************************
        // Analog signal from Tachometer
        ADC0_PSSI_R |= 8;       // Start a conversion at sequence 3
        while ((ADC0_RIS_R & 0x08) == 0)
            ;           // Wait for conversion to complete
        tachometer = ADC0_SSFIFO3_R;// Read conversion result, and assign the result to 'tachometer'
        ADC0_ISC_R = 8;     // Clear completion flag
        // ********************************************************************

        // ********************Reading from ADC1*******************************
        // Voltage From Potentiometer
        ADC1_PSSI_R |= 8;       // Start a conversion at sequence 3
        while ((ADC1_RIS_R & 0x08) == 0)
            ;           // Wait for conversion complete
        D_out = ADC1_SSFIFO3_R; // Read conversion result, and assign the result to 'D_out'
        ADC1_ISC_R = 8;     // Clear completion flag
        // ********************************************************************

        //************ Map digitized values(0-4095) of input voltage(0-3.3V) from the potentiometer ************************

        desiredDutycycle = D_out / 4095;// Get ratio of ADC value out of 4095 (12 bit ADC)
        // Input voltage/3.3 = ADC value/4095 = desiredRPM / [MAX RPM (in this case 2400)]
        //In case the desired duty cycle is in between 0% and 100%
        if (desiredDutycycle > 1)
            desiredDutycycle = 1;
        if (desiredDutycycle < 0)
            desiredDutycycle = 0;

        desiredRPM = desiredDutycycle * 2400;//Multiply 2400 by the desired duty cycle to get the desired RPM (max 2400 rpm)
        //Adjusting knob on potentiometer -> adjust desiredRPM;
        //***********************************************************************************************************************

        if (tachometer > 2500)  // Read the current tachometer value
        {
            curLogic = 1;
        }
        else if (tachometer < 1500)
        {
            curLogic = 0;
        }

        // Edge Detecting ************************************************************************************
        if (curLogic != preLogic)// Edge detected when the current logic is different from the previous logic
        {
            if (edge == 0)
            {
                TimerEnable(TIMER0_BASE, TIMER_A);  // Enable timer
                initialTimer = TimerValueGet(TIMER0_BASE, TIMER_A); // Store the initial timer value
            }
            edge++;
            if (edge == 5)  // Four poles = Five edges
            {
                finalTimer = TimerValueGet(TIMER0_BASE, TIMER_A);// Store the final timer value
                edge = 0;       // Reset edge counter
                TS = (initialTimer - finalTimer);   // Yield TS
                rpm = round(60 / (TS / SysCtlClockGet()));// From data sheet: TS = 60/RPM (sec)

                // To get a more accurate reading from the tachometer, take average for every three sample RPMs.
                sample++;       // Count the sample numbers
                rpmSum = rpmSum + rpm;  // Increment the samples
                if (sample == 3)    // Take average
                {
                    rpmAvg = rpmSum / 3;
                    sample = 0;
                    fourDigitRPM = rpmAvg;  // Send averge RPM to 'fourDigitRPM'
                    rpmSum = 0; // Reset the increment carrier 'rpmSum'
                }

                //************PROPORTIONAL SPEED CONTROL********************************************
                Error = ((desiredRPM - rpmAvg) / 2400) * 1000; /* Find the error of the closed-loop system, and project the error
                 to PWM control indicator 'speedIndicator' */
                Error = Error * Kp; // Tuning the Error by the Proportional Gain parameter 'Kp'
                speedIndicator = speedIndicator - Error;// Update the fixed value for the PWM output
                if (speedIndicator < 32)// Based on testing, maximum speed occurs at PWM output of 32 out of 1000
                {
                    speedIndicator = 32;
                }

                if (speedIndicator > 770)// Based on testing, minimum speed occurs at PWM output of 770 out of 1000
                {
                    speedIndicator = 770;
                }
                if (desiredRPM == 2400) // Speed to maximum when the desired speed is 2400 rpm
                {
                    speedIndicator = 32;
                }

                PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0,
                                 speedIndicator * ui32Load / 1000);

                TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period - 1); // Reset Timer for next loop

            }
        }

        if (curLogic == preLogic && speedIndicator == 770)// Show 0 RPM if the fan is not turning
        {
            rpm = 0;
            fourDigitRPM = 0;
            rpmAvg = 0;
        }

        if (tachometer > 2500)// Reset the tachometer by storing a new initial reading in 'preLogic'
        {
            preLogic = 1;
        }
        else if (tachometer < 1500)
        {
            preLogic = 0;
        }

        if (desiredRPM == 0)// Show 0 RPM if the desired rpm is 0. If this shows 0 but the fan is turing, something goes wrong.
        {
            rpmAvg = 0;
            fourDigitRPM = 0;
        }

//        The PWM output can be manully adjusted by pressing the buttons on Tive for testing.
//        The following codes are commented out, since a Proportional Control method is now applied.
//        In the following code 0x00 means pressing.
//        if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0x00)
//        {
//            speedIndicator --;
//            if (speedIndicator < 32)
//            {
//                speedIndicator = 32;
//            }
//            //Set the new pulse width
//            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, speedIndicator * ui32Load / 1000);
//        }
//        if (GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0) == 0x00)
//        {
//            speedIndicator ++;
//            if (speedIndicator > 770)
//            {
//                speedIndicator = 770;
//            }
//            PWMPulseWidthSet(PWM1_BASE, PWM_OUT_0, speedIndicator * ui32Load / 1000);
//        }

        // Separate the four-digit RPM to single digits by finding remainders.
        D = fourDigitRPM % 10;  // Divide by 10, remainder is D
        CD = fourDigitRPM % 100;    // Divide by 100, remainder is CD
        BCD = fourDigitRPM % 1000;  // Divide by 1000, remainder is BCD
        C = (CD - D) / 10;  // Find tens
        B = (BCD - CD) / 100;   // Find hundreds
        A = (fourDigitRPM - BCD) / 1000;    // Find thousands

        // Show digits on three seven segments.
        if (A > 0)// If the RPM has four digits, drop the one on units (show ABC)
        {
            left(A);
            middle(B);
            right(C);

        }
        else if (A == 0)// If the RPM only has three digits, drop the one on thousands (show BCD)
        {
            left(B);
            middle(C);
            right(D);
        }

    }
}

// Following switch functions generates the corresponding output configuration of the GPIO for the current RPM
// From case 0 to case 9, the outputs are 0000,0001,0010,0011,0100,0101,0110,0111,1000
// Due to the same structure for left, middle, and right, only 'left' is annotated.
void left(int Input)    // GPIO used for this seven segment are PD2,PB3,PE4,PE5
{
    switch (Input)
    {
    case 0:
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, 0x00);// Least significant bit
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0x00);// 0x00 means the output is 0, while GPIO_PIN_X maens the output is 1
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0x00);// Most significant bit
        break;
    case 1:
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0x00);
        break;
    case 2:
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0x00);
        break;
    case 3:
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0x00);
        break;
    case 4:
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0x00);
        break;
    case 5:
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0x00);
        break;
    case 6:
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0x00);
        break;
    case 7:
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, 0x00);
        break;
    case 8:
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_PIN_5);
        break;
    case 9:
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_2, GPIO_PIN_2);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_5, GPIO_PIN_5);
        break;
    }
}

void middle(int Input)      //PE1, PC4, PB4, PA5
{
    switch (Input)
    {
    case 0:
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0x00);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, 0x00);
        break;
    case 1:
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_PIN_1);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, 0x00);
        break;
    case 2:
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0x00);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, 0x00);
        break;
    case 3:
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_PIN_1);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, 0x00);
        break;
    case 4:
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0x00);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, 0x00);
        break;
    case 5:
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_PIN_1);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, 0x00);
        break;
    case 6:
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0x00);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, 0x00);
        break;
    case 7:
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_PIN_1);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, 0x00);
        break;
    case 8:
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0x00);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_PIN_5);
        break;
    case 9:
        GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_PIN_1);
        GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_PIN_5);
        break;

    }

}

void right(int Input)       //PA4, PA3, PD6, PD7
{
    switch (Input)
    {
    case 0:
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_7, 0x00);
        break;
    case 1:
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_7, 0x00);
        break;
    case 2:
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_7, 0x00);
        break;
    case 3:
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_7, 0x00);
        break;
    case 4:
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, GPIO_PIN_6);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_7, 0x00);
        break;
    case 5:
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, GPIO_PIN_6);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_7, 0x00);
        break;
    case 6:
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, GPIO_PIN_6);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_7, 0x00);
        break;
    case 7:
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, GPIO_PIN_6);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_7, 0x00);
        break;
    case 8:
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, 0x00);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_7, GPIO_PIN_7);
        break;
    case 9:
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_4, GPIO_PIN_4);
        GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, 0x00);
        GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_7, GPIO_PIN_7);
        break;

    }

}
