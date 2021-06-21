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
  byte color;
} Line;

// With the default font, 100 characters per line can be displayed exactly.
#define MAX_STATUS_MESSAGE_BUFFER_SIZE 101

// Inits the serial communication with the UX Arduino
void serialInit();

// Sends a message to the UX Arduino, meant to be displayed on status bar
void sendStatusMessage(const char *msg);

// Same as sendStatusMessage() but arguments as printf()
void sendStatusMessageFormat(const char *format, ...);

// Like sendStatusMessageFormat(), but reboots after.
void fatalError(const char *format, ...);

// These functions are the same as the ones wiuth the same names in the UX Arduino
void serialTransmitLine(Line line);
int serialReceiveLine(Line *line);
int serialReceiveOpCode();

// Clear drawing
void serialTransmitClear();

#endif
