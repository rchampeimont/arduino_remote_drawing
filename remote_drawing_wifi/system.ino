#include "system.h"
#include "serial_com.h"

#define DECLARE_UX_ARDUINO_DEAD_AFTER 1000

byte myClientId = -1;

// Number of seconds for which the UX Arduino has not sent us "alive" signals
// volatile because reset to 0 from ISR
volatile byte noResponseFromUxArduinoSeconds = 0;

void initClientId() {
  pinMode(CLIENT_ID_PIN, INPUT_PULLUP);
  if (digitalRead(CLIENT_ID_PIN) == LOW) {
    myClientId = 0;
  } else {
    myClientId = 1;
  }
}

void aliveReceived() {
  noResponseFromUxArduinoSeconds = 0;
}

void checkAlive() {
  if (noResponseFromUxArduinoSeconds >= DECLARE_UX_ARDUINO_DEAD_AFTER) {
    sendStatusMessageFormat("Rebooting UX Arduino after %d seconds without signal", noResponseFromUxArduinoSeconds);

    resetOther();

    // Reset to zero to give so time for the rebooted Arduino to start
    noResponseFromUxArduinoSeconds = 0;
  }

  if (noResponseFromUxArduinoSeconds < 255) {
    noResponseFromUxArduinoSeconds++;
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

// Report fatal error on status bar and reboot. This function never returns.
void fatalError(const char *format, ...) {
  // Stop ISR from running
  detachInterrupt(digitalPinToInterrupt(WIFI_ARDUINO_INTERRUPT_PIN));

  char buf[MAX_STATUS_MESSAGE_BUFFER_SIZE];
  char bufFinal[MAX_STATUS_MESSAGE_BUFFER_SIZE];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, MAX_STATUS_MESSAGE_BUFFER_SIZE, format, args);
  snprintf(bufFinal, MAX_STATUS_MESSAGE_BUFFER_SIZE, "%s - Rebooting...", buf);
  sendStatusMessage(bufFinal);
  va_end(args);

  // Stop everything. The other Arduino is going to reset us after detecting we are dead.
  delay(1000);
  while (1);
}
