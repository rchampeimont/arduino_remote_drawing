// Program designed for Arduino Uno Rev3

#include "Adafruit_RA8875.h"
#include "system.h"
#include "calibrate.h"
#include "serial_com.h"

// Period for telling the other Arduino that we are alive
#define TELL_ALIVE_EVERY 1000

// TFT Display resolution
const int DISPLAY_WIDTH = 800;
const int DISPLAY_HEIGHT = 480;

const int STATUS_BAR_SIZE = 16;
const uint16_t STATUS_BAR_BGCOLOR = 0b0000000000001111;

const int LINE_WIDTH = 3;

// How many filled circles to draw to draw a big line
const int LINE_STEPS = 10;

// Record a line point every
#define POINT_SAMPLING_PERIOD 20 // ms

// Colors
#define NUMBER_OF_COLORS 7
const uint16_t COLORS[NUMBER_OF_COLORS] {
  RA8875_BLACK,
  RA8875_RED,
  RA8875_GREEN,
  RA8875_BLUE,
  RA8875_CYAN,
  RA8875_MAGENTA,
  RA8875_YELLOW
};

// Consider the pen is realsed after this delay
const unsigned long RELEASE_DELAY = 50; // ms

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS_PIN, RA8875_RESET_PIN);

int lastTouchX = -1;
int lastTouchY = -1;
int beforeLastTouchX = -1;
int beforeLastTouchY = -1;
int lastLineX = -1;
int lastLineY = -1;
unsigned long lastLinePointTime = 0;

// We use 0 as meaning "not released"
unsigned long penReleaseTime = 1;

// Last Alive packet sent
unsigned long lastAliveSentTime = millis();

byte selectedColor = 0;

void setup() {
  pinMode(PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT, OUTPUT);
  pinMode(WIFI_ARDUINO_INTERRUPT_PIN, OUTPUT);
  Serial.begin(115200, SERIAL_8E1);

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

  // Reset the "Wifi" Arduino so that it resends us the drawing
  resetOther();
}

void drawBigLine(Line line) {
  int DX = line.x1 - line.x0;
  int DY = line.y1 - line.y0;
  for (int i = 0; i < LINE_STEPS; i++) {
    tft.fillCircle(
      line.x0 + i / (float) LINE_STEPS * DX,
      line.y0 + i / (float) LINE_STEPS * DY,
      LINE_WIDTH,
      COLORS[line.color]);
  }
}


void clearDisplayedDrawing() {
  tft.fillRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT - STATUS_BAR_SIZE, RA8875_WHITE);
}


void handleReceive() {
  while (Serial.available() >= (int) sizeof(Packet)) {
    Packet packet;
    if (serialReceivePacket(&packet) == 0) {
      printStatus("UX Arduino failed to read serial packet");
      return;
    }
    switch (packet.opcode) {
      case SERIAL_COM_LINE_OPCODE:
        drawBigLine(packet.data.line);
        break;
      case SERIAL_COM_MSG_OPCODE:
        printStatus(packet.data.statusMessage);
        break;
      case SERIAL_COM_CLEAR_OPCODE:
        clearDisplayedDrawing();
        break;
      case SERIAL_COM_ALIVE_OPCODE:
        aliveReceived();
        break;
      default:
        printStatusFormat("UX Arduino received invalid opcode on serial line: 0x%x", packet.opcode);
    }
  }
}

// Compute an approximate distance between (x1,y1) and (x2,y2)
int approxDistance(int x1, int y1, int x2, int y2) {
  // Use L1 norm because it's fast to compute and does not overflow 16-bit integers
  return abs(x1 - x2) + abs(y1 - y2);
}

// Average the coordinates of two points
void averagePoints(int x0, int y0, int x1, int y1, int *x, int *y) {
  *x = x0 + (x1 - x0) / 2;
  *y = y0 + (y1 - y0) / 2;
}

// We sometimes get random incorrect values from the touch screen.
// To handle that, we perform the following operation for every 3 successive ponits:
// We take the two that are nearest to each other,
// and average their corrdinates to make the final coordinates.
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
  int lineX, lineY, newX, newY;
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
        extractPoint(beforeLastTouchX, beforeLastTouchY, lastTouchX, lastTouchY, newX, newY, &lineX, &lineY);

        unsigned long now = millis();
        if (lastLineX >= 0) {
          // We are continuing a line.
          // Limit the number of line points generated per second, to optimize bandwidth
          if (now >= lastLinePointTime + POINT_SAMPLING_PERIOD) {
            Line line;
            line.x0 = lastLineX;
            line.y0 = lastLineY;
            line.x1 = lineX;
            line.y1 = lineY;
            line.color = selectedColor;

            drawBigLine(line);
            serialTransmitLine(line);

            lastLineX = lineX;
            lastLineY = lineY;
            lastLinePointTime = now;
          }
        } else {
          // This is the first point of athe line, so let's draw a single point
          // because the user might want to draw a single point and release the pen.
          Line line;
          line.x0 = lineX;
          line.y0 = lineY;
          line.x1 = lineX;
          line.y1 = lineY;
          line.color = selectedColor;

          drawBigLine(line);
          serialTransmitLine(line);

          lastLineX = lineX;
          lastLineY = lineY;
          lastLinePointTime = now;
        }
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

      // Temporary code: cycle color on each pen release.
      // This will be replaced by a color palette in the final version.
      if (lastLineX != -1) {
        selectedColor = (selectedColor + 1) % NUMBER_OF_COLORS;
      }

      beforeLastTouchX = -1;
      beforeLastTouchY = -1;
      lastTouchX = -1;
      lastTouchY = -1;
      lastLineX = -1;
      lastLineY = -1;
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

// Like printStatus() but takes printf()-like arguments
void printStatusFormat(const char *format, ...) {
  char buf[MAX_STATUS_MESSAGE_BUFFER_SIZE];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, MAX_STATUS_MESSAGE_BUFFER_SIZE, format, args);
  printStatus(buf);
  va_end(args);
}

void loop() {
  handleTouch();
  handleReceive();

  unsigned long now = millis();
  if (now >= lastAliveSentTime + TELL_ALIVE_EVERY) {
    // Tell the other Arduino we are still alive
    serialTransmitAlive();

    // Check that the other Arduino is still alive
    checkAlive();

    lastAliveSentTime = now;
  }
}
