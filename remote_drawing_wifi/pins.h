#ifndef PINS_H
#define PINS_H

// Pin tied to its own reset pin ("Wifi" Arduino)
#define RESET_PIN_SELF 6

// Pin tied to the other Arduino reset pin ("UX" Arduino)
#define RESET_PIN_OTHER 7

// This pin must be pulled LOW on one of the two Wifi Arduinos
#define CLIENT_ID_PIN 5

// Where to detect interrupts coming from the UX Arduino
#define WIFI_ARDUINO_INTERRUPT_PIN 2

#define DEBUG_PIN 12

extern byte myClientId;

// Reboot the entire system (both Arduinos). This function never returns.
void reboot();

#endif
