#ifndef SERIAL_COM_H
#define SERIAL_COM_H

#define SERIAL_COM_LINE_OPCODE 'L'
#define SERIAL_COM_MSG_OPCODE 'M'
#define SERIAL_COM_CLEAR_OPCODE 'C'

#define MAX_STATUS_MESSAGE_BUFFER_SIZE 60

typedef struct {
  int x0;
  int y0;
  int x1;
  int y1;
  byte color; // an index in the COLORS constant array
} Line;

typedef union {
  Line line;
  char statusMessage[MAX_STATUS_MESSAGE_BUFFER_SIZE];
} DataInPacket;

typedef struct {
  byte opcode;
  DataInPacket data;
} Packet;

void serialInit();

void serialTransmitLine(Line line);

int serialReceivePacket(Packet *packetAddr);

#endif
