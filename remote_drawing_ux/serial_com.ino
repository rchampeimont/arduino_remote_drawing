// This file contains the code to communicate with the "Wifi" Arduino

#include "pins.h"
#include "serial_com.h"

void serialInit() {
  Serial.begin(115200, SERIAL_8E1);

  // Used to trigger serial reception on the other side
  pinMode(WIFI_ARDUINO_INTERRUPT_PIN, OUTPUT);
}

int serialReceiveStatusMessage(char msg[MAX_STATUS_MESSAGE_BUFFER_SIZE]) {
  size_t readChars = Serial.readBytesUntil(0, msg, MAX_STATUS_MESSAGE_BUFFER_SIZE-1);
  if (readChars >= MAX_STATUS_MESSAGE_BUFFER_SIZE-1) {
    // The message is too big, so we need to discard it until we reach NUL
    while (Serial.read() != 0);
    msg[MAX_STATUS_MESSAGE_BUFFER_SIZE - 1] = 0;
    // Return 1 because we can still display the truncated string
    return 1;
  } else if (readChars > 0) {
    msg[readChars] = '\0';
    return 1;
  } else {
    return 0;
  }
}

void serialTransmitLine(Line line) {
  digitalWrite(WIFI_ARDUINO_INTERRUPT_PIN, HIGH);
  
  Serial.write(SERIAL_COM_LINE_OPCODE);
  Serial.write((byte*) &line, sizeof(Line));
  Serial.flush();

  // Tell the receiver to read the serial data now
  digitalWrite(WIFI_ARDUINO_INTERRUPT_PIN, LOW);
}

int serialReceiveLine(Line *line) {
  if (Serial.readBytes((byte*) line, sizeof(Line)) == sizeof(Line)) {
    return 1;
  } else {
    return 0;
  }
}

int serialReceiveOpCode() {
  int opcode = Serial.read();
  return opcode;
}
