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

  pinMode(DEBUG_PIN, OUTPUT);

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


  attachInterrupt(digitalPinToInterrupt(WIFI_ARDUINO_INTERRUPT_PIN), handleSerialReceive, FALLING);

  initWifi();

  connectToRedisServer();

  downloadInitialData();

  Serial.println("Setup finished.");
  Serial.println("====================================================");
  digitalWrite(LED_BUILTIN, LOW);
}

// This function is an Interrupt Service Routine (ISR)
void handleSerialReceive() {
  // This allows to measure time spent in ISR with an oscilloscope
  digitalWrite(DEBUG_PIN, HIGH);

  // We cannot wait for data in an ISR,
  // so we need to make sure the packet is already completely received
  while (Serial1.available() >= (int) sizeof(Packet)) {
    Packet packet;
    if (serialReceivePacket(&packet) == 0) {
      fatalError("UX Arduino failed to read serial packet");
      return;
    }
    switch (packet.opcode) {
      case SERIAL_COM_LINE_OPCODE:
        redisAddLineToSendBuffer(packet.data.line);
        break;
      default:
        fatalError("UX Arduino received invalid opcode on serial line: %d", packet.opcode);
    }
  }

  digitalWrite(DEBUG_PIN, LOW);
}

void handleRedisReceive() {
  int fromClientId, newLinesStartIndex, newLinesStopIndex;
  if (redisReceiveMessage(&fromClientId, &newLinesStartIndex, &newLinesStopIndex)) {
    Serial.write("Redis message received: From client ");
    Serial.print(fromClientId);
    Serial.write(" - Interval: ");
    Serial.print(newLinesStartIndex);
    Serial.write(" to ");
    Serial.println(newLinesStopIndex);

    // Download lines only if they come from the other client,
    // otherwise it means that the lines were drawn by our own user
    // so they are already displayed on our screen.
    if (fromClientId != myClientId) {
      int count = redisDownloadLinesBegin(newLinesStartIndex, newLinesStopIndex);
      getLinesFromRedisAndDrawThem(count);
    }
  }
}

void getLinesFromRedisAndDrawThem(int count) {
  Line line;

  for (int i = 0; i < count; i++) {
    // Receive line from Redis
    redisDownloadLine(&line);
    // Send line to UX Arduino to render it on screen
    serialTransmitLine(line);
    // Leave some time for the UX Arduino to render the line
    delay(10);
  }
}

void downloadInitialData() {
  int count;

  sendStatusMessage("Downloading drawing from server...");
  count = redisDownloadLinesBegin(0, -1);

  sendStatusMessageFormat("Downloading drawing from server (%d lines). Please wait...", count);
  getLinesFromRedisAndDrawThem(count);
  sendStatusMessageFormat("Downloading drawing finished (%d lines). You can now draw things!", count);
}

void loop() {
  handleRedisReceive();
  sendLinesInBuffer();
  runRedisPeriodicTasks();

  if (millis() > REBOOT_EVERY_MS) {
    sendStatusMessage("It's time for the periodic reboot! Please wait a minute...");
    delay(5000);
    reboot();
  }

  Serial.println("Alive");

  delay(1000);
}
