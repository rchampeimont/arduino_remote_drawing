// Implements the status bar and toolbar

#include "bars.h"

#define STATUS_BAR_BGCOLOR 0b0000000000001111
#define TOOLBAR_BGCOLOR 0xA514

#define STRING_ON_CLEAR_BUTTON "DEL"
#define BUTTON_MARGIN 4

typedef struct {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} ButtonCoords;

ButtonCoords buttonsCoords[NUMBER_OF_BUTTONS];

byte selectedColor = 0;

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

void renderButtonBorder(int buttonIndex) {
  uint16_t color;

  if (selectedColor == buttonIndex) {
    if (selectedColor == RA8875_BLACK) {
      color = RA8875_BLACK;
    } else {
      color = RA8875_BLACK;
    }
  } else {
    color = TOOLBAR_BGCOLOR;
  }

  tft.drawRect(
    buttonsCoords[buttonIndex].x,
    buttonsCoords[buttonIndex].y,
    buttonsCoords[buttonIndex].w,
    buttonsCoords[buttonIndex].h,
    color);
  tft.drawRect(
    buttonsCoords[buttonIndex].x + 1,
    buttonsCoords[buttonIndex].y + 1,
    buttonsCoords[buttonIndex].w - 2,
    buttonsCoords[buttonIndex].h - 2,
    color);
}

void renderToolbar() {
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    uint16_t color;
    if (i < NUMBER_OF_COLORS) {
      color = COLORS[i];
    } else {
      color = RA8875_BLACK;
    }
    tft.fillRect(
      buttonsCoords[i].x + BUTTON_MARGIN,
      buttonsCoords[i].y + BUTTON_MARGIN,
      buttonsCoords[i].w - 2 * BUTTON_MARGIN,
      buttonsCoords[i].h - 2 * BUTTON_MARGIN,
      color);
    renderButtonBorder(i);
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


void handleToolbarClick(int x, int y) {
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    if (x > buttonsCoords[i].x
        && y > buttonsCoords[i].y
        && x < buttonsCoords[i].x + BUTTON_SIZE
        && y < buttonsCoords[i].y + BUTTON_SIZE) {
      if (i < NUMBER_OF_COLORS) {
        selectedColor = i;
        renderToolbar();
      } else {
        // TODO implement CLEAR
      }
    }
  }
}
