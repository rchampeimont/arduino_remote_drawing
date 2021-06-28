// Implements the status bar and toolbar

#include "bars.h"

#define STATUS_BAR_BGCOLOR 0b0000000000001111

typedef struct {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} ButtonCoords;

ButtonCoords buttonsCoords[NUMBER_OF_BUTTONS];

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

void initToolbar() {
  // Put buttons on two columns
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    buttonsCoords[i].x = DISPLAY_WIDTH - TOOLBAR_WIDTH + (i % 2) * BUTTON_SIZE;
    buttonsCoords[i].y = i / 2 * BUTTON_SIZE;
    buttonsCoords[i].w = BUTTON_SIZE;
    buttonsCoords[i].h = BUTTON_SIZE;
  }
}

void renderToolbar() {
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    uint16_t color;
    if (i < NUMBER_OF_COLORS) {
      color = COLORS[i];
    } else {
      color = RA8875_BLACK;
    }
    tft.fillRect(buttonsCoords[i].x, buttonsCoords[i].y, buttonsCoords[i].w, buttonsCoords[i].h, color);
    if (i == NUMBER_OF_COLORS) {
      tft.textMode();
      tft.textSetCursor(
        buttonsCoords[i].x + BUTTON_SIZE / 2 - FONT_WIDTH * (sizeof(STRING_ON_CLEAR_BUTTON) - 1) / 2,
        buttonsCoords[i].y + BUTTON_SIZE / 2 - FONT_HEIGHT / 2);
      tft.textColor(RA8875_WHITE, color);
      tft.textWrite(STRING_ON_CLEAR_BUTTON, sizeof(STRING_ON_CLEAR_BUTTON) - 1);
      tft.graphicsMode();
    }
  }
}

byte getSelectedColor() {
  return 0;
}
