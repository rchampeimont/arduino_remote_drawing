// This file contains the code to communicate with the "UX" Arduino

#include <stdarg.h>
#include "serial_com.h"
#include "system.h"

void serialInit() {
  Serial1.begin(115200, SERIAL_8E1);

  // Used to trigger serial reception
  pinMode(WIFI_ARDUINO_INTERRUPT_PIN, INPUT);

  // Leave some time for the UX Arduino to start up before sending serial data
  delay(5000);

  sendStatusMessageFormat("Hello received from Wifi Arduino. I am client %d.", myClientId);
  delay(1000);

  // Clear any remaining displayed drawing from previous run.
  serialTransmitClear();
}

void sendStatusMessage(const char *msg) {
  // Print on serial console for debug (if connected by USB)
  Serial.print("MSG: ");
  Serial.println(msg);

  // Send to other Arduino for display in status bar
  serialTransmitStatusMessage(msg);
}

void sendStatusMessageFormat(const char *format, ...) {
  char buf[MAX_STATUS_MESSAGE_BUFFER_SIZE];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, MAX_STATUS_MESSAGE_BUFFER_SIZE, format, args);
  sendStatusMessage(buf);
  va_end(args);
}

// Report fatal error on status bar and reboot. This function never returns.
void fatalError(const char *format, ...) {
  char buf[MAX_STATUS_MESSAGE_BUFFER_SIZE];
  char bufFinal[MAX_STATUS_MESSAGE_BUFFER_SIZE];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, MAX_STATUS_MESSAGE_BUFFER_SIZE, format, args);
  for (int i = 60; i > 0; i--) {
    snprintf(bufFinal, MAX_STATUS_MESSAGE_BUFFER_SIZE, "%s - Rebooting in %d seconds...", buf, i);
    sendStatusMessage(bufFinal);
    delay(1000);
  }
  reboot();
  va_end(args);
}

void initPacket(Packet *packet) {
  // Zero unused fields to allow for simpler debug
  // when looking at the serial data with an oscilloscope.
  memset(packet, 0, sizeof(Packet));
}

void serialTransmitPacket(Packet packet) {
  Serial1.write((byte*) &packet, sizeof(Packet));
}

int serialReceivePacket(Packet *packetAddr) {
  if (Serial1.readBytes((byte *) packetAddr, sizeof(Packet)) == sizeof(Packet)) {
    return 1;
  } else {
    return 0;
  }
}

void serialTransmitStatusMessage(const char *msg) {
  Packet packet;
  initPacket(&packet);
  packet.opcode = SERIAL_COM_MSG_OPCODE;
  strncpy(packet.data.statusMessage, msg, MAX_STATUS_MESSAGE_BUFFER_SIZE);
  packet.data.statusMessage[MAX_STATUS_MESSAGE_BUFFER_SIZE - 1] = '\0';
  serialTransmitPacket(packet);
}

void serialTransmitLine(Line line) {
  Packet packet;
  initPacket(&packet);
  packet.opcode = SERIAL_COM_LINE_OPCODE;
  packet.data.line = line;
  serialTransmitPacket(packet);
}

void serialTransmitClear() {
  Packet packet;
  initPacket(&packet);
  packet.opcode = SERIAL_COM_CLEAR_OPCODE;
  serialTransmitPacket(packet);
}

void serialTransmitAlive() {
  Packet packet;
  initPacket(&packet);
  packet.opcode = SERIAL_COM_ALIVE_OPCODE;
  serialTransmitPacket(packet);
}
