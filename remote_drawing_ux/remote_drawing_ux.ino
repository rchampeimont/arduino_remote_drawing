// Program designed for Arduino Uno Rev3

#include "Adafruit_RA8875.h"
#include "system.h"
#include "calibrate.h"
#include "serial_com.h"
#include "backlight.h"
#include "bars.h"

// Period for telling the other Arduino that we are alive
#define TELL_ALIVE_EVERY 1000

// TFT Display resolution
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480

#define LINE_WIDTH 3

// How many filled circles to draw to draw a big line
#define LINE_STEPS 10

// Record a line point every
#define POINT_SAMPLING_PERIOD 20 // ms

// Colors
#define NUMBER_OF_COLORS 15
const uint16_t COLORS[NUMBER_OF_COLORS] {
  RA8875_BLACK,
  0xBDD7, // light grey
  0x630C, // dark grey
  0x981F, // violet
  RA8875_MAGENTA,
  0xFC3E, // pink
  RA8875_RED,
  0xFB60, // orange
  RA8875_YELLOW,
  RA8875_GREEN,
  0x0443, // dark green
  RA8875_CYAN,
  RA8875_BLUE,
  0b0000000000001111, // dark blue
  RA8875_WHITE
};

// Consider the pen is realsed after this delay
#define RELEASE_DELAY 50 // ms

Adafruit_RA8875 tft = Adafruit_RA8875(RA8875_CS_PIN, RA8875_RESET_PIN);

void setup() {
  pinMode(PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT, OUTPUT);
  pinMode(WIFI_ARDUINO_INTERRUPT_PIN, OUTPUT);
  pinMode(DEBUG_LOOP_RUN_TIME_PIN, OUTPUT);
  pinMode(DEBUG_CRASH_PIN, OUTPUT);
  pinMode(DEBUG_SPECIFIC_PIN, OUTPUT);
  pinMode(READY_TO_DRAW_PIN, INPUT);

  tft.begin(RA8875_800x480);

  tft.displayOn(true);
  tft.GPIOX(true);      // Enable TFT - display enable tied to GPIOX

  initBacklight();

  // Setup touch screen
  tft.touchEnable(true);
  setupCalibration();

  tft.fillScreen(RA8875_WHITE);

  printStatus("Copyright (c) 2021 Raphael Champeimont");

  initToolbar();
  renderToolbar();

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
  tft.fillRect(0, 0, DISPLAY_WIDTH - TOOLBAR_WIDTH, DISPLAY_HEIGHT - STATUS_BAR_SIZE, RA8875_WHITE);
}


void handleReceive() {
  char statusMessage[MAX_STATUS_MESSAGE_LENGTH + 1];

  while (Serial.available() >= (int) sizeof(ReceivedPacket)) {
    ReceivedPacket packet;
    if (serialReceivePacket(&packet, statusMessage)) {
      switch (packet.opcode) {
        case SERIAL_COM_LINE_OPCODE:
          drawBigLine(packet.data.line);
          // While loading the initial drawing, the Wifi Arduino does not send
          // "alive" packets yet, and sends only lines, so we must consider
          // them as proof that the other Arduino is alive.
          aliveReceived();
          break;
        case SERIAL_COM_MSG_OPCODE:
          printStatus(statusMessage);
          break;
        case SERIAL_COM_CLEAR_OPCODE:
          clearDisplayedDrawing();
          break;
        case SERIAL_COM_ALIVE_OPCODE:
          aliveReceived();
          break;
        default:
          error("Wifi->UX Arduino invalid opcode: 0x%x", packet.opcode);
      }
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
  static int lastTouchX = -1;
  static int lastTouchY = -1;
  static int beforeLastTouchX = -1;
  static int beforeLastTouchY = -1;
  static int lastLineX = -1;
  static int lastLineY = -1;
  static unsigned long lastLinePointTime = 0;
  static bool buttonPressed = false;

  // We use 0 as meaning "not released"
  static unsigned long penReleaseTime = 1;

  uint16_t rawX, rawY;
  int highPrecisionX = -1, highPrecisionY = -1, lowPrecisionX = -1, lowPrecisionY = -1;

  // Wifi Arduino is not ready to receive data, so don't allow user to draw
  if (digitalRead(READY_TO_DRAW_PIN) == LOW) return;

  if (tft.touched()) {
    penReleaseTime = 0;
    tft.touchRead(&rawX, &rawY);

    // Translate touch screen coordinates to display coordinates
    if (translateTouchCoords(rawX, rawY, &lowPrecisionX, &lowPrecisionY) == 0
        && lowPrecisionX >= 0
        && lowPrecisionY >= 0
        && lowPrecisionX < DISPLAY_WIDTH
        && lowPrecisionY < DISPLAY_HEIGHT) {

      if (beforeLastTouchX >= 0 && lastTouchX >= 0) {
        // Compute high-precision coordinates of clicked point by averaging successive coordinates
        extractPoint(beforeLastTouchX, beforeLastTouchY, lastTouchX, lastTouchY, lowPrecisionX, lowPrecisionY, &highPrecisionX, &highPrecisionY);

        unsigned long now = millis();
        if (highPrecisionX >= DRAWABLE_WIDTH || highPrecisionY >= DRAWABLE_HEIGHT) {
          // We are oustide the drawable area, so try to detect a click on a button
          if (lastLineX == -1 && !buttonPressed && handleToolbarClick(highPrecisionX, highPrecisionY)) {
            // This flag is to avoid repeating click on button until user releases pen.
            buttonPressed = true;
          }
        } else if (lastLineX >= 0 && ! buttonPressed) {
          // We are continuing a line.
          // Limit the number of line points generated per second, to optimize bandwidth
          if (now >= lastLinePointTime + POINT_SAMPLING_PERIOD) {
            Line line;
            line.x0 = lastLineX;
            line.y0 = lastLineY;
            line.x1 = highPrecisionX;
            line.y1 = highPrecisionY;
            line.color = selectedColor;

            drawBigLine(line);
            serialTransmitLine(line);

            lastLineX = highPrecisionX;
            lastLineY = highPrecisionY;
            lastLinePointTime = now;
          }
        } else if (! buttonPressed) {
          // This is the first point of the line, so let's draw a single point
          // because the user might want to draw a single point and release the pen.
          Line line;
          line.x0 = highPrecisionX;
          line.y0 = highPrecisionY;
          line.x1 = highPrecisionX;
          line.y1 = highPrecisionY;
          line.color = selectedColor;

          drawBigLine(line);
          serialTransmitLine(line);

          lastLineX = highPrecisionX;
          lastLineY = highPrecisionY;
          lastLinePointTime = now;
        }
      }

      beforeLastTouchX = lastTouchX;
      beforeLastTouchY = lastTouchY;
      lastTouchX = lowPrecisionX;
      lastTouchY = lowPrecisionY;
    }
  } else {
    if (penReleaseTime == 0) {
      // This is the first loop where the pen is released, so record release time.
      penReleaseTime = millis();
    } else if (millis() - penReleaseTime > RELEASE_DELAY) {
      // Pen has been released for long enough, so consider the pen really released.
      buttonPressed = false;
      beforeLastTouchX = -1;
      beforeLastTouchY = -1;
      lastTouchX = -1;
      lastTouchY = -1;
      lastLineX = -1;
      lastLineY = -1;
    }
  }
}


void loop() {
  static unsigned long lastAliveSentTime = millis();

  digitalWrite(DEBUG_LOOP_RUN_TIME_PIN, HIGH);
  handleTouch();

  digitalWrite(DEBUG_LOOP_RUN_TIME_PIN, LOW);
  handleReceive();

  unsigned long now = millis();
  if (now >= lastAliveSentTime + TELL_ALIVE_EVERY) {
    // Tell the other Arduino we are still alive
    serialTransmitAlive();

    // Check that the other Arduino is still alive
    checkAlive();

    lastAliveSentTime = now;
  }

  updateBacklightIfNecesary();
}
