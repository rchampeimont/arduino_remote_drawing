#ifndef SYSTEM_H
#define SYSTEM_H

// Adafruit RA8875 connections:
// Connect SCLK to ICSP SCK
// Connect MISO to ICSP MISO
// Connect MOSI to ICSP MOSI
#define RA8875_CS_PIN 10
#define RA8875_RESET_PIN 9

// Tie this pin to ground to force recalibration at start time
#define FORCE_CALIBRATE_PIN 5

// Pin which is connected to the pin declared as WIFI_ARDUINO_INTERRUPT_PIN on the Wifi Arduino
#define WIFI_ARDUINO_INTERRUPT_PIN 2

// Pin tied to the other ("Wifi") Arduino reset circuit
#define PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT 7

// High if fatal error occurs
#define DEBUG_CRASH_PIN A0

// Alternates HIGH/LOW on each loop run
#define DEBUG_LOOP_RUN_TIME_PIN A1

#define DEBUG_SPECIFIC_PIN A2

#define PHOTOTRANSISTOR_PIN A5

// Record that the Wifi Arduino is alive
void aliveReceived();

// Check the Wifi Arduino is alive. To call every 1 second.
void checkAlive();

// Reset the other Arduino ("Wifi" Arduino)
void resetOther();

// Report an error on the status bar and reset the other Arduino
void error(const char *format, ...);

#endif
