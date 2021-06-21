#ifndef SERIAL_COM_H
#define SERIAL_COM_H

#define SERIAL_COM_LINE_OPCODE 'L'
#define SERIAL_COM_MSG_OPCODE 'M'
#define SERIAL_COM_CLEAR_OPCODE 'C'

typedef struct {
  int x0;
  int y0;
  int x1;
  int y1;
  byte color; // an index in the COLORS constant array
} Line;

// With the default font, 100 characters per line can be displayed exactly.
#define MAX_STATUS_MESSAGE_BUFFER_SIZE 101

void serialInit();

// Receive a status message from the Wifi Arduino, to display in the status bar
int serialReceiveStatusMessage(char msg[MAX_STATUS_MESSAGE_BUFFER_SIZE]);

// Transmit a line drawn by the user
void serialTransmitLine(Line line);

// Receive a line from the Wifi Arduino
int serialReceiveLine(Line *line);

// Try to receive the next instruction
int serialReceiveOpCode();

#endif
