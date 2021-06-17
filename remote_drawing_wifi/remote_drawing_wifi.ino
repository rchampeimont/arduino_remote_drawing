// Program designed for Arduino Uno Wifi Rev2.
// ATMEGA328 registers emulation is not needed.

#include <WiFiNINA.h>

#include "wifi.h"
#include "serial_com.h"
#include "secrets.h"
#include "redis_client.h"
#include "pins.h"

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

  // Init serial connection to the other Arduino
  serialInit();

  initWifi();

  connectToRedisServer();

  Serial.println("Setup finished.");
  Serial.println("====================================================");
  digitalWrite(LED_BUILTIN, LOW);
}

// Handle a drawn line received from "our" UX Arduino
void handleSerialReceiveLine() {
  int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
  if (serialReceiveLine(&x0, &y0, &x1, &y1)) {
    //Serial.println("SERIAL DATA: LINE\n");
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
  if (tryRedisReceive(buf)) {
    Serial.print("REDIS: ");
    Serial.println(buf);
  }
}

void loop() {
  handleSerialReceive();
  handleRedisReceive();
}
