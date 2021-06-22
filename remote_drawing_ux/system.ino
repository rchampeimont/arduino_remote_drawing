#define DECLARE_WIFI_ARDUINO_DEAD_AFTER 30

// Number of seconds for which the Wifi Arduino has not sent us "alive" signals
volatile byte noResponseFromWifiArduinoSeconds = 0;

void aliveReceived() {
  noResponseFromWifiArduinoSeconds = 0;
}

void checkAlive() {
  if (noResponseFromWifiArduinoSeconds >= DECLARE_WIFI_ARDUINO_DEAD_AFTER) {
    printStatusFormat("Rebooting Wifi Arduino after %d seconds without signal", noResponseFromWifiArduinoSeconds);
    digitalWrite(RESET_CIRCUIT_OTHER, HIGH);
    // RC constant of circuit is 22 ms so this is far enough to fill the capacitor
    delay(500);

    digitalWrite(RESET_CIRCUIT_OTHER, LOW);
    // RC constant of circuit is 1 sec so we wait 2 sec for a full capacitor unload
    delay(2000);

    // Reset to zero to give so time for the rebooted Arduino to start
    noResponseFromWifiArduinoSeconds = 0;
  }

  if (noResponseFromWifiArduinoSeconds < 255) {
    noResponseFromWifiArduinoSeconds++;
  }
}
