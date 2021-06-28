#ifndef BARS_H
#define BARS_H

#define STATUS_BAR_SIZE 16

// Print a message in the status bar
void printStatus(const char* msg);

// Like printStatus() but takes printf()-like arguments
void printStatusFormat(const char *format, ...);

#endif
