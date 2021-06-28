#include "system.h"
#include "serial_com.h"

// Reset the other Arduino if it does not report being alive for this number of seconds
#define DECLARE_UX_ARDUINO_DEAD_AFTER 60

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
  // Wait 500 ms but without using delay() because it cannot be called from an ISR,
  // and the function we are in might be called from an ISR.
  // RC constant of circuit is 22 ms so 400 ms is far enough to fill the capacitor.
  for (int i = 0; i < 50; i++) {
    delayMicroseconds(10000); // 10 ms
  }
  digitalWrite(PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT, LOW);
}

// Report fatal error on status bar and reboot. This function never returns.
// Can be called from an ISR.
void fatalError(const char *format, ...) {
  // Stop ISR from running
  detachInterrupt(digitalPinToInterrupt(WIFI_ARDUINO_INTERRUPT_PIN));

  digitalWrite(DEBUG_CRASH_PIN, HIGH);
  digitalWrite(READY_TO_DRAW_PIN, LOW);

  char buf[MAX_STATUS_MESSAGE_LENGTH + 1];
  char bufFinal[MAX_STATUS_MESSAGE_LENGTH + 1];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, MAX_STATUS_MESSAGE_LENGTH + 1, format, args);
  snprintf(bufFinal, MAX_STATUS_MESSAGE_LENGTH + 1, "FATAL: %s", buf);
  sendStatusMessage(bufFinal);
  va_end(args);

  // Wait some time to let the user see the message.
  // Wait 30 sec but without using delay() because it cannot be called from an ISR,
  // and the function we are in might be called from an ISR.
  for (int i = 0; i < 3000; i++) {
    delayMicroseconds(10000); // 10 ms
  }

  // Reset the other Arduino, which is going to reset us after.
  resetOther();
  delay(1000);
  while (1);
}
