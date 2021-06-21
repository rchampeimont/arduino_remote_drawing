// Program designed for Arduino Uno Wifi Rev2.
// ATMEGA328 registers emulation is not needed.

#include <WiFiNINA.h>

#include "wifi.h"
#include "serial_com.h"
#include "secrets.h"
#include "redis_client.h"
#include "pins.h"

// Reboot every day to avoid issues linked with millis() overflow
// and for safety if we get stuck in a bad state.
#define REBOOT_EVERY_MS 86400000

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Serial connection to computer, used for debug only
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("");
  Serial.println("====================================================");
  Serial.println("Arduino restarted.");
  Serial.println("Setting up...");

  initClientId();

  // Init serial connection to the other Arduino
  serialInit();

  initWifi();

  connectToRedisServer();

  downloadInitialData();

  Serial.println("Setup finished.");
  Serial.println("====================================================");
  digitalWrite(LED_BUILTIN, LOW);
}

// Handle a drawn line received from "our" UX Arduino
void handleSerialReceiveLine() {
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  if (serialReceiveLine(&x0, &y0, &x1, &y1)) {
    redisTransmitLine(x0, y0, x1, y1);
  } else {
    sendStatusMessage("Invalid line data received from UX Arduino on Wifi Arduino");
  }
}

void handleSerialReceive() {
  int opcode = serialReceiveOpCode();
  switch (opcode) {
    case -1:
      // No data received on serial line
      break;
    case SERIAL_COM_LINE_OPCODE:
      handleSerialReceiveLine();
      break;
    default:
      sendStatusMessageFormat("Wifi Arduino received invalid opcode on serial line: %d", opcode);
  }
}

void handleRedisReceive() {
  char buf[REDIS_RECEIVE_BUFFER_SIZE];
  if (redisTryReceiveSub(buf)) {
    Serial.print("Data received with Redis sub");
  }
}

void downloadInitialData() {
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  int count;

  sendStatusMessage("Downloading drawing from server...");
  count = redisDownloadLinesBegin();
  
  sendStatusMessageFormat("Downloading drawing from server (%d lines). Please wait...", count);
  
  for (int i=0; i<count; i++) {
    // Receive line from Redis
    redisDownloadLine(&x0, &y0, &x1, &y1);
    // Send line to UX Arduino to render it on screen
    serialTransmitLine(x0, y0, x1, y1);
    // Leave some time for the UX Arduino to render the line
    delay(10);
  }

  sendStatusMessageFormat("Downloading drawing finished (%d lines). You can now draw things!", count);
}

void loop() {
  handleSerialReceive();
  handleRedisReceive();
  runRedisPeriodicTasks();

  if (millis() > REBOOT_EVERY_MS) {
    sendStatusMessage("It's time for the periodic reboot! Please wait a minute...");
    delay(5000);
    reboot();
  }
}
