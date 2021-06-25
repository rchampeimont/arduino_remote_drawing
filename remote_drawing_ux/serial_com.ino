// This file contains the code to communicate with the "Wifi" Arduino

#include "system.h"
#include "serial_com.h"

void initPacket(SentPacket *packet) {
  // Zero unused fields to allow for simpler debug
  // when looking at the serial data with an oscilloscope.
  memset(packet, 0, sizeof(SentPacket));
}

void serialTransmitPacket(SentPacket packet) {
  Serial.write((byte*) &packet, sizeof(SentPacket));
  Serial.flush();

  // Tell the receiver to read the serial data now.
  // The 1ms delay is necessary, otherwise the receiver
  // is sometimes interrupted before the data is completly received.
  digitalWrite(WIFI_ARDUINO_INTERRUPT_PIN, HIGH);
  delay(1);
  digitalWrite(WIFI_ARDUINO_INTERRUPT_PIN, LOW);
}


int serialReceivePacket(ReceivedPacket *packetAddr, char returnedMessage[MAX_STATUS_MESSAGE_LENGTH + 1]) {
  // Since each status message is divided in several package,
  // we concatenate them together in this variable.
  // + 1 because we always have a \0 at the end
  static char concatenatedStatusMessage[MAX_STATUS_MESSAGE_LENGTH + 1];

  if (Serial.readBytes((byte *) packetAddr, sizeof(ReceivedPacket)) == sizeof(ReceivedPacket)) {
    if (packetAddr->opcode == SERIAL_COM_MSG_OPCODE) {
      if (packetAddr->data.statusMessage.offset == 0) {
        // This is a new message, so clear the previous one
        memset(concatenatedStatusMessage, 0, sizeof(concatenatedStatusMessage));
      }
      if (packetAddr->data.statusMessage.offset + STATUS_MESSAGE_SIZE_IN_PACKET > MAX_STATUS_MESSAGE_LENGTH) {
        error("Too high offset for status message: %d", packetAddr->data.statusMessage.offset);
        return 0;
      } else {
        memcpy(
          concatenatedStatusMessage + packetAddr->data.statusMessage.offset,
          packetAddr->data.statusMessage.part,
          STATUS_MESSAGE_SIZE_IN_PACKET);
        if (packetAddr->data.statusMessage.offset + STATUS_MESSAGE_SIZE_IN_PACKET == MAX_STATUS_MESSAGE_LENGTH) {
          // We have received the end of a status message, so report to the caller that we have something
          memcpy(returnedMessage, concatenatedStatusMessage, MAX_STATUS_MESSAGE_LENGTH + 1);
          return 1;
        } else {
          // The status message is not complete yet, so tell the caller than we have nothing
          return 0;
        }
      }
    } else {
      // Packet is not a status message
      return 1;
    }
  } else {
    return 0;
  }
}

void serialTransmitLine(Line line) {
  SentPacket packet;
  initPacket(&packet);
  packet.opcode = SERIAL_COM_LINE_OPCODE;
  packet.data.line = line;
  serialTransmitPacket(packet);
}

void serialTransmitAlive() {
  SentPacket packet;
  initPacket(&packet);
  packet.opcode = SERIAL_COM_ALIVE_OPCODE;
  serialTransmitPacket(packet);
}
