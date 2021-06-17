// Program designed for Arduino Uno Rev3

#include "Adafruit_RA8875.h"
#include "pins.h"
#include "calibrate.h"
#include "serial_com.h"

// TFT Display resolution
const int DISPLAY_WIDTH = 800;
const int DISPLAY_HEIGHT = 480;

const int STATUS_BAR_SIZE = 16;
const uint16_t STATUS_BAR_BGCOLOR = 0b0000000000001111;

const int LINE_WIDTH = 3;

// How many filled circles to draw to draw a big line
const int LINE_STEPS = 10;

// Consider the pen is realsed after this delay
const unsigned long RELEASE_DELAY = 50; // ms

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS_PIN, RA8875_RESET_PIN);

int lastTouchX = -1;
int lastTouchY = -1;
int beforeLastTouchX = -1;
int beforeLastTouchY = -1;
int lastAverageX = -1;
int lastAverageY = -1;

// We use 0 as meaning "not released"
unsigned long penReleaseTime = 1;

void setup() {
  serialInit();

  tft.begin(RA8875_800x480);

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024); // PWM output for backlight
  tft.PWM1out(255);

  // Setup touch screen
  tft.touchEnable(true);
  setupCalibration();

  tft.fillScreen(RA8875_WHITE);

  printStatus("Copyright (c) 2021 Raphael Champeimont");
}

void drawBigLine(int x0, int y0, int x1, int y1, uint16_t color) {
  int DX = x1 - x0;
  int DY = y1 - y0;
  for (int i = 0; i < LINE_STEPS; i++) {
    tft.fillCircle(x0 + i / (float) LINE_STEPS * DX, y0 + i / (float) LINE_STEPS * DY, LINE_WIDTH, color);
  }
}


void handleReceiveLine() {
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  if (serialReceiveLine(&x0, &y0, &x1, &y1)) {
    drawBigLine(x0, y0, x1, y1, RA8875_BLUE);
  } else {
    printStatus("Invalid line data received on UX Arduino");
  }
}

void handleReceiveStatusMessage() {
  char msg[MAX_STATUS_MESSAGE_BUFFER_SIZE];
  serialReceiveStatusMessage(msg);
  printStatus(msg);
}

void handleReceive() {
  int opcode = serialReceiveOpCode();
  switch (opcode) {
    case -1:
      // No data received on serial line
      break;
    case SERIAL_COM_LINE_OPCODE:
      handleReceiveLine();
      break;
    case SERIAL_COM_MSG_OPCODE:
      handleReceiveStatusMessage();
      break;
    default:
      char errorMsg[SERIAL_COM_MSG_OPCODE];
      snprintf(errorMsg, SERIAL_COM_MSG_OPCODE, "UX Arduino received invalid opcode on serial line: %d", opcode);
      printStatus(errorMsg);
  }
}

int approxDistance(int x1, int y1, int x2, int y2) {
  // Use L1 norm because it's fast to compute and does not overflow 16-bit integers
  return abs(x1 - x2) + abs(y1 - y2);
}

void averagePoints(int x0, int y0, int x1, int y1, int *x, int *y) {
  *x = x0 + (x1 - x0) / 2;
  *y = y0 + (y1 - y0) / 2;
}

void extractPoint(int x1, int y1, int x2, int y2, int x3, int y3, int *x, int *y) {
  int d12 = approxDistance(x1, y1, x2, y2);
  int d23 = approxDistance(x2, y2, x3, y3);
  int d31 = approxDistance(x3, y3, x1, y1);
  if (d12 < d23) {
    if (d31 < d12) {
      // Points 1 and 3 are the nearest
      averagePoints(x1, y1, x3, y3, x, y);
    } else {
      // Points 1 and 2 are the nearest
      averagePoints(x1, y1, x2, y2, x, y);
    }
  } else {
    if (d31 < d23) {
      // Points 1 and 3 are the nearest
      averagePoints(x1, y1, x3, y3, x, y);
    } else {
      // Points 2 and 3 are the nearest
      averagePoints(x2, y2, x3, y3, x, y);
    }
  }
}

void handleTouch() {
  uint16_t rawX, rawY;
  int averageX, averageY, newX, newY;
  if (tft.touched()) {
    penReleaseTime = 0;
    tft.touchRead(&rawX, &rawY);

    // Translate touch screen coordinates to display coordinates
    if (translateTouchCoords(rawX, rawY, &newX, &newY) == 0
        && newX >= 0
        && newY >= 0
        && newX < DISPLAY_WIDTH
        && newY < DISPLAY_HEIGHT - STATUS_BAR_SIZE) {

      if (beforeLastTouchX >= 0 && lastTouchX >= 0) {
        extractPoint(beforeLastTouchX, beforeLastTouchY, lastTouchX, lastTouchY, newX, newY, &averageX, &averageY);
        if (lastAverageX >= 0) {
          drawBigLine(lastAverageX, lastAverageY, averageX, averageY, RA8875_BLACK);
          serialTransmitLine(lastAverageX, lastAverageY, averageX, averageY);
        }
        lastAverageX = averageX;
        lastAverageY = averageY;
      }

      beforeLastTouchX = lastTouchX;
      beforeLastTouchY = lastTouchY;
      lastTouchX = newX;
      lastTouchY = newY;
    }
  } else {
    if (penReleaseTime == 0) {
      // This is the first loop where the pen is released, so record release time.
      penReleaseTime = millis();
    } else if (millis() - penReleaseTime > RELEASE_DELAY) {
      // Pen has been released for long enough, so consider the pen really released.
      beforeLastTouchX = -1;
      beforeLastTouchY = -1;
      lastTouchX = -1;
      lastTouchY = -1;
      lastAverageX = -1;
      lastAverageY = -1;
    }
  }
}

// Print a message in the status bar
void printStatus(const char* msg) {
  tft.graphicsMode();
  tft.fillRect(0, DISPLAY_HEIGHT - STATUS_BAR_SIZE, DISPLAY_WIDTH, STATUS_BAR_SIZE, STATUS_BAR_BGCOLOR);
  tft.textMode();
  tft.textSetCursor(0, DISPLAY_HEIGHT - STATUS_BAR_SIZE);
  tft.textColor(RA8875_WHITE, STATUS_BAR_BGCOLOR);
  tft.textWrite(msg, strlen(msg));
  tft.graphicsMode();
}

void loop() {
  handleTouch();
  handleReceive();
}
