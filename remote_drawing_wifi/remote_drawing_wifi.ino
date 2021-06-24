// Program designed for Arduino Uno Wifi Rev2.
// ATMEGA328 registers emulation is not needed.

#include <WiFiNINA.h>

#include "wifi.h"
#include "serial_com.h"
#include "secrets.h"
#include "redis_client.h"
#include "system.h"

// Reboot every day to avoid issues linked with millis() overflow
// and for safety if we get stuck in a bad state.
#define REBOOT_EVERY_MS 86400000

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(DEBUG_ISR_TIME_PIN, OUTPUT);
  pinMode(DEBUG_CRASH_PIN, OUTPUT);
  pinMode(PIN_TO_OTHER_ARDUINO_RESET_CIRCUIT, OUTPUT);
  pinMode(WIFI_ARDUINO_INTERRUPT_PIN, INPUT);

  // Serial connection to computer, used for debug only
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("");
  Serial.println("====================================================");
  Serial.println("Arduino restarted.");
  Serial.println("Setting up...");

  Serial1.begin(115200, SERIAL_8E1);

  initClientId();

  // Init serial connection to the other Arduino
  serialInit();

  initWifi();

  connectToRedisServer();

  downloadInitialData();

  // Empty serial read buffer
  Serial1.end();
  Serial1.begin(115200, SERIAL_8E1);

  // Start receiving data from UX Arduino
  attachInterrupt(digitalPinToInterrupt(WIFI_ARDUINO_INTERRUPT_PIN), handleSerialReceive, FALLING);

  Serial.println("Setup finished.");
  Serial.println("====================================================");
  digitalWrite(LED_BUILTIN, LOW);
}

// This function is an Interrupt Service Routine (ISR)
void handleSerialReceive() {
  digitalWrite(DEBUG_ISR_TIME_PIN, HIGH);
  
  // We cannot wait for data in an ISR,
  // so we need to make sure the packet is already completely received
  while (Serial1.available() >= (int) sizeof(ReceivedPacket)) {
    ReceivedPacket packet;
    if (serialReceivePacket(&packet) == 0) {
      fatalError("UX Arduino failed to read serial packet");
      return;
    }
    switch (packet.opcode) {
      case SERIAL_COM_LINE_OPCODE:
        redisAddLineToSendBuffer(packet.data.line);
        break;
      case SERIAL_COM_ALIVE_OPCODE:
        aliveReceived();
        break;
      default:
        fatalError("UX->Wifi Arduino invalid opcode: 0x%x", packet.opcode);
    }
  }

  digitalWrite(DEBUG_ISR_TIME_PIN, LOW);
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
  }
}

void downloadInitialData() {
  int count;

  sendStatusMessage("Downloading drawing from server...");
  count = redisDownloadLinesBegin(0, -1);

  sendStatusMessageFormat("Downloading drawing from server (%d lines). Please wait...", count);
  getLinesFromRedisAndDrawThem(count);
  sendStatusMessageFormat("Downloaded %d lines. You can now draw!", count);
}

void loop() {
  handleRedisReceive();
  sendLinesInBuffer();
  runRedisPeriodicTasks();

  while (millis() > REBOOT_EVERY_MS) {
    sendStatusMessage("It's time for the periodic reboot! Please wait a minute...");
    // The other Arduino is then going to reset us at boot
    resetOther();
    // Retry after 60 sec if reset failed
    delay(60000);
  }

  char s[100];
  snprintf(s, sizeof(s), "I am alive. Last alive received %d seconds ago.\n", noResponseFromUxArduinoSeconds);
  Serial.write(s);
  
  // Tell the UX Arduino we are alive
  serialTransmitAlive();

  // Check if the UX Arduino is alive
  checkAlive();

  delay(1000);
}
