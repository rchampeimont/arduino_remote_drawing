#include "system.h"

// Reset the other Arduino if it does not report being alive for this number of seconds
#define DECLARE_WIFI_ARDUINO_DEAD_AFTER 60

// Number of seconds for which the Wifi Arduino has not sent us "alive" signals
volatile byte noResponseFromWifiArduinoSeconds = 0;

void aliveReceived() {
  noResponseFromWifiArduinoSeconds = 0;
}

void checkAlive() {
  if (noResponseFromWifiArduinoSeconds >= DECLARE_WIFI_ARDUINO_DEAD_AFTER) {
    printStatusFormat("Rebooting Wifi Arduino after %d seconds without signal", noResponseFromWifiArduinoSeconds);
    delay(1000);

    resetOther();

    // Reset to zero to give so time for the rebooted Arduino to start
    noResponseFromWifiArduinoSeconds = 0;
  }

  if (noResponseFromWifiArduinoSeconds < 255) {
    noResponseFromWifiArduinoSeconds++;
  }
}

void resetOther() {
  Serial.end();
  
  digitalWrite(PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT, HIGH);
  // RC constant of circuit is 22 ms so this is far enough to fill the capacitor
  delay(500);
  digitalWrite(PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT, LOW);
  
  Serial.begin(115200, SERIAL_8E1);
}
