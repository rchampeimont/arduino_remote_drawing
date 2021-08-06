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

  sendStatusMessageFormat("Hello received from Wifi Arduino: Client %d / Version " VERSION, myClientId);

  // Clear any remaining displayed drawing from previous run.
  serialTransmitClear();

  delay(2000);
}

void sendStatusMessage(const char *msg) {
  // Print on serial console for debug (if connected by USB)
  Serial.print("MSG: ");
  Serial.println(msg);

  // Send to other Arduino for display in status bar
  serialTransmitStatusMessage(msg);
}

void sendStatusMessageFormat(const char *format, ...) {
  char buf[MAX_STATUS_MESSAGE_LENGTH + 1];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, MAX_STATUS_MESSAGE_LENGTH + 1, format, args);
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

  // The packets from Wifi to UX Arduino are really big,
  // so only one can fit in the 64 bytes buffer. So we
  // need to wait for the UX Arduino to process the packet
  // before we send another one.
  delayMicroseconds(10000);
}

int serialReceivePacket(ReceivedPacket *packetAddr) {
  if (Serial1.readBytes((byte *) packetAddr, sizeof(ReceivedPacket)) == sizeof(ReceivedPacket)) {
    return 1;
  } else {
    return 0;
  }
}

// In this function we split the message to send in several smaller packets
// that will be concatenated together by the receiver.
void serialTransmitStatusMessage(const char *msg) {
  SentPacket packet;
  byte messageBuffer[MAX_STATUS_MESSAGE_LENGTH];

  // strncpy() does not put a \0 if there is no space for it,
  // but this is exactly the behavior we want here.
  strncpy((char *) messageBuffer, msg, MAX_STATUS_MESSAGE_LENGTH);

  for (byte offset = 0; offset < MAX_STATUS_MESSAGE_LENGTH; offset += STATUS_MESSAGE_SIZE_IN_PACKET) {
    initPacket(&packet);
    packet.opcode = SERIAL_COM_MSG_OPCODE;
    packet.data.statusMessage.offset = offset;
    memcpy(packet.data.statusMessage.part, messageBuffer + offset, STATUS_MESSAGE_SIZE_IN_PACKET);
    serialTransmitPacket(packet);
  }
}

void serialTransmitLine(Line line) {
  SentPacket packet;
  initPacket(&packet);
  packet.opcode = SERIAL_COM_LINE_OPCODE;
  packet.data.line = line;
  serialTransmitPacket(packet);
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
