## ATX Power Switch Utility for AT-Style Motherboards.

Suppose you are building a retro-PC with an AT motherboard, but you want to put it into an ATX case with an ATX power supply. You can buy an adapter that will adapt the ATX power connector to an AT power connector, but it expects you to use an AT-style power switch, which is a toggle style switch. But ATX power switches are momentary, so they will not work as-is.

Some workarounds are to wire the power switch so that it is always on, and then control the power to the PC using the switch on the back of the power supply, or simply plug/unplug the power cord. But who wants to do that when you can use AtxPowerSwitch?!?

AtxPowerSwitch allows you to use an ATX-style switch with an AT motherboard with ease! Simply press the power switch to power on your PC, and then press and hold the power switch for a brief period to power it down. Voila!

AtxPowerSwitch uses only a PIC12F675 with no external components, and it is easy to wire and connect to your PC:

| PIC Pin | ATX Pin (color)  | Description
|---------|------------------|--------------------------------
| 1       | 9 (purple)       | +5V standby, power for PIC
| 2       | ATX power switch | The other wire from your power switch
| 3       | ATX power switch | One of the wires from your power switch
| 5       | 16 (green)       | ATX power-on
| 8       | * (any black)    | Ground

AtxPowerSwitch is in low-power sleep mode almost all the time, so it uses very little power.

*Copyright 2025, Timothy Alicie*
