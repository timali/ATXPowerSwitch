/* =============================================================================
 * ATX power switch utility for AT-style motherboards.
 * 
 * Suppose you are building a retro-PC with an AT motherboard, but you want to
 * put it into an ATX case with an ATX power supply. You can buy an adapter that
 * will adapt the ATX power connector to an AT power connector, but it expects
 * you to use an AT-style power switch, which is a toggle style switch. But ATX
 * power switches are momentary, so they will not work as-is.
 * 
 * Some workarounds are to wire the power switch so that it is always on, and
 * then control the power to the PC using the switch on the back of the power
 * supply, or simply plug/unplug the power cord. But who wants to do that when
 * you can use AtxPowerSwitch?!?
 * 
 * AtxPowerSwitch allows you to use an ATX-style switch with an AT motherboard
 * with ease! Simply press the power switch to power on your PC, and then press
 * and hold the power switch for a brief period to power it down. Voila!
 * 
 * AtxPowerSwitch uses only a PIC16F527 (it's what I had handy), and it is easy
 * to wire and connect to your PC:
 * 
 * PIC Pin          ATX Pin             Description
 * 1                9 (purple)          +5V standby, power for PIC
 * 20               any black           Ground
 * 9                16 (green)          ATX power-on
 * 3                ATX power switch    One of the wires from your power switch
 * 20               ATX power switch    The other wire from your power switch
 * 
 * To be clear, PIC pin 3 is connected to one of the wires of your ATX power
 * switch (it doesn't matter which), and the other wire of our ATX power switch
 * is grounded, wherever is easiest for you to ground it.
 * 
 * AtxPowerSwitch is in low-power sleep mode almost all the time, so it uses
 * very little power.
 * 
 * Copyright 2025, Timothy Alicie
 *
 * ===========================================================================*/

// Device Configuration
#pragma config FOSC = INTRC_IO  // Use the internal oscillator
#pragma config WDTE = ON        // Enable the watchdog timer (used for sleep)
#pragma config CP = OFF         // Disable code protection
#pragma config MCLRE = ON       // MCLR pin functions as MCLR
#pragma config IOSCFS = 4MHz    // 4 MHz clock speed
#pragma config CPSW = OFF       // Self-writable memory code protection off
#pragma config BOREN = ON       // Brown-out reset enable
#pragma config DRTEN = OFF      // Device reset timer disabled

//=============================================================================
// Includes
//=============================================================================
#include <xc.h>

//=============================================================================
// User-Setting Defines
//=============================================================================

// How long, in milliseconds, the power button must be held before powering off.
#define POWER_OFF_TIME_MS   (500)

//=============================================================================
// Utility Defines
//=============================================================================

// The nominal watchdog timeout value, including pre-scaler, in milliseconds.
#define WDT_MS              (18*2)

// The approximate number of times per second we wake and process data.
#define TICK_RATE_HZ        (1000 / WDT_MS)

// The number of ticks (wake cycles) before powering off.
#define POWER_OFF_COUNT     (POWER_OFF_TIME_MS / TICK_RATE_HZ)

// The input used for reading the ATX power switch. RA4 is the only unused input
// with a built-in pull-up resistor, so use it.
#define SWITCH_INPUT        PORTAbits.RA4

// Powers off the ATX power supply by setting RC7 as an input.
#define POWER_OFF           TRISC = 0b10000000; poweredOn = 0;

// Powers on the ATX power supply by setting RC7 as an output, pulled low.
#define POWER_ON            TRISC = 0b00000000; poweredOn = 1;

//=============================================================================
// Variables
//=============================================================================

// Whether the ATX power supply is currently powered on (1) or off (0).
unsigned char poweredOn;

/* =============================================================================
 * Called when the ATX power button is pressed, and also when it is held.
 * 
 * param[in] holdCount How long (in ticks) the button has been held. 0 indicates
 *      the button has just been pressed.
 * ===========================================================================*/
void OnButtonPressed(int holdCount)
{
    static unsigned char powerOffArmed = 0;
    
    if (poweredOn)
    {
        // If the button has just been pressed while power on, arm the power
        // off sequence.
        if (holdCount == 0)
        {
            powerOffArmed = 1;
        }
        
        // See if the button has been held long enough to power off the supply.
        if (powerOffArmed && (holdCount >= POWER_OFF_COUNT))
        {
            POWER_OFF;
            powerOffArmed = 0;
        }
    }
    else
    {
        // If the button has just been pressed while the supply was off, then
        // power on the supply.
        if (holdCount == 0)
        {
            POWER_ON;
        }
    }
}

/* =============================================================================
 * Main entry point.
 * ===========================================================================*/
void main(void)
{
    unsigned char lastButtonState = 0;
    unsigned int holdCount = 0;
    
    // Disable analog mode on all pins so that we can use them as digital pins.
    ANSEL = 0;

    // Set all outputs to low by default.
    PORTA = PORTB = PORTC = 0x00;  
    
    // Ensure all unused pins are set to outputs and driven low to save power.
    TRISA = 0b00010000; // Set only RA4 as input
    TRISB = 0b00000000; // Set all bits as outputs.
    
    // Start with the supply off.
    POWER_OFF;
    
    // OPTION Register:
    // bit 7: Disable PORTA interrupt on change.
    // bit 6: Enable PORTA weak pull-ups.
    // bit 5: Timer0 clock source internal clock.
    // bit 4: Timer0 source edge on falling edge.
    // bit 3: Pre-scaler assigned to WTD.
    // bits 0-2: Set 1:2 WDT pre-scaler (doubles watchdog timeout value).
    OPTION = 0b10011001;
    
    // Loop forever.
    while (1)
    {
        // Clear the watchdog timer, giving us plenty of time to what we need to.
        CLRWDT();

        // Poll the input pin. The logic is inverted (high means not pressed).
        if (SWITCH_INPUT == 1)
        {
            lastButtonState = 0;
        }
        else
        {
            // If the switch has just been pressed, reset the hold count.
            if (!lastButtonState)
            {
                holdCount = 0;
            }

            // Indicate that the button is pressed.
            OnButtonPressed(holdCount++);
            
            lastButtonState = 1;
        }
       
        // Go to sleep. We'll wake up when the watchdog fires.
        SLEEP();

        // Some sources say a NOP is recommended after resuming from sleep. Not
        // sure if this is needed or not, but it doesn't hurt anything.
        NOP();        
    }
}