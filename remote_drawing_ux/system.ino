#include "system.h"

#define DECLARE_WIFI_ARDUINO_DEAD_AFTER 1000

// Number of seconds for which the Wifi Arduino has not sent us "alive" signals
volatile byte noResponseFromWifiArduinoSeconds = 0;

void aliveReceived() {
  noResponseFromWifiArduinoSeconds = 0;
}

void checkAlive() {
  if (noResponseFromWifiArduinoSeconds >= DECLARE_WIFI_ARDUINO_DEAD_AFTER) {
    printStatusFormat("Rebooting Wifi Arduino after %d seconds without signal", noResponseFromWifiArduinoSeconds);
    
    resetOther();

    // Reset to zero to give so time for the rebooted Arduino to start
    noResponseFromWifiArduinoSeconds = 0;
  }

  if (noResponseFromWifiArduinoSeconds < 255) {
    noResponseFromWifiArduinoSeconds++;
  }
}

void resetOther() {
  digitalWrite(PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT, HIGH);
  // RC constant of circuit is 22 ms so this is far enough to fill the capacitor
  delay(500);

  digitalWrite(PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT, LOW);
  // RC constant of circuit is 1 sec so we wait 2 sec for a full capacitor unload
  delay(2000);
}
