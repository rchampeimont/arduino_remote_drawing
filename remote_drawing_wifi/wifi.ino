#include <SPI.h>
#include <WiFiNINA.h>
#include "wifi.h"
#include "secrets.h"
#include "serial_com.h"
#include "system.h"

void initWifi() {
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    fatalError("INTERNAL ERROR: Communication with WiFi hardware failed!");
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  WiFi.lowPowerMode();

  sendStatusMessageFormat("Trying to connect to \"%s\" wifi network...", WIFI_NAME);
  if (WiFi.begin(WIFI_NAME, WIFI_PASSWORD) != WL_CONNECTED) {
    fatalError("Connection to Wifi network \"%s\" failed", WIFI_NAME);
  }

  sendStatusMessage("Connection to wifi OK.");
  delay(1000);
}
