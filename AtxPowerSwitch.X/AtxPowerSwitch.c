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
 * AtxPowerSwitch uses only a PIC12F675 with no external components, and it is
 * easy to wire and connect to your PC:
 * 
 * PIC Pin          ATX Pin (color)     Description
 * 1                9 (purple)          +5V standby, power for PIC
 * 8                * (any black)       Ground
 * 5                16 (green)          ATX power-on
 * 3                ATX power switch    One of the wires from your power switch
 * 2                ATX power switch    The other wire from your power switch
 * 
 * AtxPowerSwitch is in low-power sleep mode almost all the time, so it uses
 * very little power.
 * 
 * Copyright 2025, Timothy Alicie
 *
 * ===========================================================================*/

// Device Configuration
#pragma config FOSC   = INTRCIO  // Use the internal oscillator with IO on GP4
#pragma config WDTE   = ON       // Enable the watchdog timer (used for sleep)
#pragma config PWRTE  = ON       // Enable 72 ms power on timer (power-up only)
#pragma config MCLRE  = OFF      // MCLR pin tied internally to Vdd
#pragma config BOREN  = ON       // Brown-out reset enable
#pragma config CP     = OFF      // Disable code protection
#pragma config CPD    = OFF      // Disable data memory code protection

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

// The nominal watchdog timeout value, including 2x pre-scaler, in milliseconds.
#define WDT_MS              (18*2)

// The approximate number of times per second we wake and process data.
#define TICK_RATE_HZ        (1000 / WDT_MS)

// The number of ticks (wake cycles) before powering off.
#define POWER_OFF_COUNT     (POWER_OFF_TIME_MS / TICK_RATE_HZ)

// The input used for reading the ATX power switch. RA4 is the only unused input
// with a built-in pull-up resistor, so use it.
#define SWITCH_INPUT        GPIObits.GPIO4

// Powers off the ATX power supply by setting GP2 as an input.
#define POWER_OFF           TRISIO2 = 1; poweredOn = 0;

// Powers on the ATX power supply by setting GP2 as an output, pulled low.
#define POWER_ON            TRISIO2 = 0; poweredOn = 1;

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

    // OPTION Register:
    // bit 7: GPIO pull-ups are enabled by individual port latches.
    // bit 6: Interrupt on falling edge of GP2/INT (we don't care)
    // bit 5: Internal instruction cycle clock (we don't care)
    // bit 4: Increment on low-to-high transition on GP2/T0CKI (we don't care)
    // bit 3: Pre-scaler assigned to WTD.
    // bits 0-2: Set 1:2s WDT pre-scaler (doubles watchdog timeout value).
    OPTION_REG = 0b00001001;

    // Disable analog mode on all pins so that we can use them as digital pins.
    ANSEL = 0;

    // Set all GPIO outputs to 0.
    GPIO = 0;

    // Completely disable the comparator to use the lowest power possible.
    CMCON = 0x07;
    
    // Set GP5 as an output (it is low) to serve as a ground for the switch.
    TRISIO5 = 0;

    // Enable the weak pull-up on our switch input (GP4).
    WPUbits.WPU4 = 1;    

    // Start with the supply off.
    POWER_OFF;

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