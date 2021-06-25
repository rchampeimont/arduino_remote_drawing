#ifndef SERIAL_COM_H
#define SERIAL_COM_H

#define SERIAL_COM_LINE_OPCODE 'L'
#define SERIAL_COM_MSG_OPCODE 'M'
#define SERIAL_COM_CLEAR_OPCODE 'C'
#define SERIAL_COM_ALIVE_OPCODE 'A'

// The maximum number of characters in a status message (not including the \0)
// We can display exactly 100 characters on one line on the screen
#define MAX_STATUS_MESSAGE_LENGTH 100

// Each status message is divided in several packets
#define STATUS_MESSAGE_SIZE_IN_PACKET 10

typedef struct {
  int x0;
  int y0;
  int x1;
  int y1;
  byte color; // an index in the COLORS constant array
} Line;

typedef struct {
  byte offset;
  byte part[STATUS_MESSAGE_SIZE_IN_PACKET];
} StatusMessagePartInPacket;

// Received packets can contain status message, unlike sent packets.
typedef union {
  Line line;
  StatusMessagePartInPacket statusMessage;
} DataInReceivedPacket;

typedef union {
  Line line;
} DataInSentPacket;

typedef struct {
  byte opcode;
  DataInReceivedPacket data;
} ReceivedPacket;

typedef struct {
  byte opcode;
  DataInSentPacket data;
} SentPacket;

// Transmit a drawn line to the Wifi Arduino to send over the network
void serialTransmitLine(Line line);

// Receive a packet from the Wifi Arduino
int serialReceivePacket(ReceivedPacket *packetAddr, char returnedMessage[MAX_STATUS_MESSAGE_LENGTH + 1]);

// Tell the other Arduino that we are alive
void serialTransmitAlive();

#endif
