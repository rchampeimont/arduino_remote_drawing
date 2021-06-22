#ifndef SYSTEM_H
#define SYSTEM_H

// Pin tied to its own reset pin ("Wifi" Arduino)
#define RESET_PIN_SELF 6

// Pin tied to the other Arduino reset pin ("UX" Arduino)
#define RESET_PIN_OTHER 7

// This pin must be pulled LOW on one of the two Wifi Arduinos
#define CLIENT_ID_PIN 5

// Where to detect interrupts coming from the UX Arduino
#define WIFI_ARDUINO_INTERRUPT_PIN 2

#define DEBUG_PIN 12

// To distiguish this Wifi Arduino from the other Wifi Arduino
extern byte myClientId;

// Reboot the entire system (both Arduinos). This function never returns.
void reboot();

// Report that an "alive" signal was received from the other serial-connected Arduino
// Can be called from ISR.
void aliveReceived();

// Check the UX Arduino is alive. To call every 1 second.
void checkAlive();

#endif
