#define DECLARE_WIFI_ARDUINO_DEAD_AFTER 10

// Number of seconds for which the Wifi Arduino has not sent us "alive" signals
volatile byte noResponseFromWifiArduinoSeconds = 0;

void aliveReceived() {
  noResponseFromWifiArduinoSeconds = 0;
}

void checkAlive() {
  if (noResponseFromWifiArduinoSeconds < 255) {
    noResponseFromWifiArduinoSeconds++;
  }
  if (noResponseFromWifiArduinoSeconds > DECLARE_WIFI_ARDUINO_DEAD_AFTER) {
    printStatusFormat("Wifi Arduino has not reported being alive for %d seconds", noResponseFromWifiArduinoSeconds);
  }
}
