#ifndef PINS_H
#define PINS_H

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

#endif
