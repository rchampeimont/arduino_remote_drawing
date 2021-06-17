#ifndef SERIAL_COM_H
#define SERIAL_COM_H

#define SERIAL_COM_LINE_OPCODE 'L'
#define SERIAL_COM_MSG_OPCODE 'M'

// With the default font, 100 characters per line can be displayed exactly.
#define MAX_STATUS_MESSAGE_BUFFER_SIZE 101

void serialInit();

// Receive a status message from the Wifi Arduino, to display in the status bar
int serialReceiveStatusMessage(char msg[MAX_STATUS_MESSAGE_BUFFER_SIZE]);

// Transmit a line drawn by the user
void serialTransmitLine(int x0, int y0, int x1, int y1);

// Receive a line from the Wifi Arduino
int serialReceiveLine(int *x0, int *y0, int *x1, int *y1);

// Try to receive the next instruction
int serialReceiveOpCode();

#endif
