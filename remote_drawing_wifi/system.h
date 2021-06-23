#ifndef SYSTEM_H
#define SYSTEM_H

// Pin tied to the other ("UX") Arduino reset circuit
#define PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT 7

// This pin must be pulled LOW on one of the two Wifi Arduinos
#define CLIENT_ID_PIN 5

// Where to detect interrupts coming from the UX Arduino
#define WIFI_ARDUINO_INTERRUPT_PIN 2

#define DEBUG_PIN 12

// To distiguish this Wifi Arduino from the other Wifi Arduino
extern byte myClientId;

// Reset the other Arduino ("UX" Arduino), which will then reset us (cf. its setup() code)
void resetOther();

// Report that an "alive" signal was received from the other serial-connected Arduino
// Can be called from ISR.
void aliveReceived();

// Check the UX Arduino is alive. To call every 1 second.
void checkAlive();

// Report a fatal error and stop the system
void fatalError(const char *format, ...);

#endif
