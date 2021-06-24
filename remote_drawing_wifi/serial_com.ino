// This file contains the code to communicate with the "UX" Arduino

#include <stdarg.h>
#include "serial_com.h"
#include "system.h"

void serialInit() {
  Serial.print("UX to Wifi Arduino packet size is ");
  Serial.print(sizeof(ReceivedPacket));
  Serial.println(" bytes.");

  Serial.print("Wifi to UX Arduino packet size is ");
  Serial.print(sizeof(SentPacket));
  Serial.println(" bytes.");

  sendStatusMessageFormat("Hello received from Wifi Arduino. I am client %d.", myClientId);

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



void initPacket(SentPacket *packet) {
  // Zero unused fields to allow for simpler debug
  // when looking at the serial data with an oscilloscope.
  memset(packet, 0, sizeof(SentPacket));
}

void serialTransmitPacket(SentPacket packet) {
  Serial1.write((byte*) &packet, sizeof(SentPacket));
}

int serialReceivePacket(ReceivedPacket *packetAddr) {
  if (Serial1.readBytes((byte *) packetAddr, sizeof(ReceivedPacket)) == sizeof(ReceivedPacket)) {
    return 1;
  } else {
    return 0;
  }
}

void serialTransmitStatusMessage(const char *msg) {
  SentPacket packet;
  initPacket(&packet);
  packet.opcode = SERIAL_COM_MSG_OPCODE;
  strncpy(packet.data.statusMessage, msg, MAX_STATUS_MESSAGE_BUFFER_SIZE);
  packet.data.statusMessage[MAX_STATUS_MESSAGE_BUFFER_SIZE - 1] = '\0';
  serialTransmitPacket(packet);
}

void serialTransmitLine(Line line) {
  SentPacket packet;
  initPacket(&packet);
  packet.opcode = SERIAL_COM_LINE_OPCODE;
  packet.data.line = line;
  serialTransmitPacket(packet);
  // Leave some time for the UX Arduino to render the line
  delay(10);
}

void serialTransmitClear() {
  SentPacket packet;
  initPacket(&packet);
  packet.opcode = SERIAL_COM_CLEAR_OPCODE;
  serialTransmitPacket(packet);
}

void serialTransmitAlive() {
  SentPacket packet;
  initPacket(&packet);
  packet.opcode = SERIAL_COM_ALIVE_OPCODE;
  serialTransmitPacket(packet);
}
