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

// Inits the serial communication with the UX Arduino
void serialInit();

// Sends a message to the UX Arduino, meant to be displayed on status bar
void sendStatusMessage(const char *msg);

// Same as sendStatusMessage() but arguments as printf()
void sendStatusMessageFormat(const char *format, ...);

// Like sendStatusMessageFormat(), but reboots after.
void fatalError(const char *format, ...);

// Transmit line to the UX Arduino to display it
void serialTransmitLine(Line line);

// Tell the UX Arduino to clear the screen
void serialTransmitClear();

// Receive a packet from the UX Arduino
int serialReceivePacket(Packet *packetAddr);

// Clear drawing
void serialTransmitClear();

#endif
