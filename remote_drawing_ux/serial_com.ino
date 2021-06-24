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


int serialReceivePacket(ReceivedPacket *packetAddr) {
  if (Serial.readBytes((byte *) packetAddr, sizeof(ReceivedPacket)) == sizeof(ReceivedPacket)) {
    return 1;
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
