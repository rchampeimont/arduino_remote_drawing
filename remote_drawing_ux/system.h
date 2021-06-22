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
#define RESET_PIN_OTHER 7

// Report that an "alive" signal was received from the other serial-connected Arduino
void aliveReceived();

// Check the Wifi Arduino is alive. To call every 1 second.
void checkAlive();

#endif
