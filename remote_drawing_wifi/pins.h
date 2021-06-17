#ifndef PINS_H
#define PINS_H

// Pin tied to its own reset pin ("Wifi" Arduino)
#define RESET_PIN_SELF 6

// Pin tied to the other Arduino reset pin ("UX" Arduino)
#define RESET_PIN_OTHER 7

// Reboot the entire system (both Arduinos). This function never returns.
void reboot();

#endif
