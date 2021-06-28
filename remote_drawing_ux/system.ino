#include "system.h"

// Reset the other Arduino if it does not report being alive for this number of seconds
#define DECLARE_WIFI_ARDUINO_DEAD_AFTER 60

unsigned long lastAliveReceived = millis();

void aliveReceived() {
  lastAliveReceived = millis();
}

void checkAlive() {
  unsigned long now = millis();
  int noSignalSice = (int) ((now - lastAliveReceived) / 1000);
  if (noSignalSice >= DECLARE_WIFI_ARDUINO_DEAD_AFTER) {
    printStatusFormat("Rebooting Wifi Arduino after %d seconds without signal", noSignalSice);
    delay(1000);

    resetOther();

    // Reset the time without alive to give the other Arduino a new chance
    lastAliveReceived = now;
  }
}

void resetOther() {
  Serial.end();

  // Make sure the capacitor is unloaded
  digitalWrite(PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT, LOW);
  // RC constant of circuit is 1 sec
  delay(2000);

  digitalWrite(PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT, HIGH);
  // RC constant of circuit is 22 ms so 500 ms this is far enough to fill the capacitor
  delay(500);
  digitalWrite(PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT, LOW);

  Serial.begin(115200, SERIAL_8E1);
}

void error(const char *format, ...) {
  char buf[MAX_STATUS_MESSAGE_LENGTH + 1];
  char bufFinal[MAX_STATUS_MESSAGE_LENGTH + 1];
  va_list args;
  
  va_start(args, format);
  vsnprintf(buf, MAX_STATUS_MESSAGE_LENGTH + 1, format, args);
  snprintf(bufFinal, MAX_STATUS_MESSAGE_LENGTH + 1, "ERROR: %s", buf);
  printStatus(bufFinal);
  va_end(args);

  digitalWrite(DEBUG_CRASH_PIN, HIGH);
  delay(30000);
  resetOther();
}
