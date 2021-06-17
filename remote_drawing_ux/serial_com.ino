// This file contains the code to communicate with the "Wifi" Arduino

#include "pins.h"
#include "serial_com.h"

void serialInit() {
  Serial.begin(115200, SERIAL_8E1);
}

int serialReceiveStatusMessage(char msg[MAX_STATUS_MESSAGE_BUFFER_SIZE]) {
  size_t readChars = Serial.readBytesUntil(0, msg, MAX_STATUS_MESSAGE_BUFFER_SIZE-1);
  if (readChars >= MAX_STATUS_MESSAGE_BUFFER_SIZE-1) {
    // The message is too big, so we need to discard it until we reach NUL
    while (Serial.read() != 0);
    msg[MAX_STATUS_MESSAGE_BUFFER_SIZE - 1] = 0;
    digitalWrite(WARNING_LED_PIN, HIGH);
    // Return 1 because we can still display the truncated string
    return 1;
  } else if (readChars > 0) {
    msg[readChars] = '\0';
    return 1;
  } else {
    return 0;
  }
}

void serialTransmitLine(int x0, int y0, int x1, int y1) {
  int a[4] = { x0, y0, x1, y1 };
  Serial.write(SERIAL_COM_LINE_OPCODE);
  Serial.write((byte*) a, sizeof(a));
}

int serialReceiveLine(int *x0, int *y0, int *x1, int *y1) {
  int a[4];
  if (Serial.readBytes((byte*) a, sizeof(a)) == sizeof(a)) {
    *x0 = a[0];
    *y0 = a[1];
    *x1 = a[2];
    *y1 = a[3];
    return 1;
  } else {
    return 0;
  }
}

int serialReceiveOpCode() {
  int opcode = Serial.read();
  return opcode;
}
