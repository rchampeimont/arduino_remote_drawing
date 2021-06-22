#include <SPI.h>
#include <WiFiNINA.h>
#include "wifi.h"
#include "secrets.h"
#include "serial_com.h"
#include "system.h"

char targetWifiSSID[] = WIFI_NAME;
char targetWifiPassword[] = WIFI_PASSWORD;

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

  sendStatusMessageFormat("Trying to connect to \"%s\" wifi network...", targetWifiSSID);
  if (WiFi.begin(targetWifiSSID, targetWifiPassword) != WL_CONNECTED) {
    fatalError("Connection to Wifi network \"%s\" failed", targetWifiSSID);
  }

  sendStatusMessage("Connection to wifi OK.");
  delay(1000);
}
