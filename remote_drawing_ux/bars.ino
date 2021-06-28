// Implements the status bar and toolbar

#include "bars.h"

#define STATUS_BAR_BGCOLOR 0b0000000000001111

void printStatus(const char* msg) {
  tft.graphicsMode();
  tft.fillRect(0, DISPLAY_HEIGHT - STATUS_BAR_SIZE, DISPLAY_WIDTH, STATUS_BAR_SIZE, STATUS_BAR_BGCOLOR);
  tft.textMode();
  tft.textSetCursor(0, DISPLAY_HEIGHT - STATUS_BAR_SIZE);
  tft.textColor(RA8875_WHITE, STATUS_BAR_BGCOLOR);
  tft.textWrite(msg, strlen(msg));
  tft.graphicsMode();
}

void printStatusFormat(const char *format, ...) {
  char buf[MAX_STATUS_MESSAGE_LENGTH + 1];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, MAX_STATUS_MESSAGE_LENGTH + 1, format, args);
  printStatus(buf);
  va_end(args);
}
