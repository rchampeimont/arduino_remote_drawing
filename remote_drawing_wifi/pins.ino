#include "pins.h"
#include "serial_com.h"

// Reboot the entire system
void reboot() {
  sendStatusMessage("Rebooting...");
  delay(1000);

  // Pull reset pin low, then release
  pinMode(RESET_PIN_OTHER, OUTPUT);
  delay(500);
  pinMode(RESET_PIN_OTHER, INPUT);
  delay(500);
  pinMode(RESET_PIN_SELF, OUTPUT);
  delay(500);
  pinMode(RESET_PIN_SELF, INPUT);
  delay(500);

  // We should now have rebooted, but if not, report the issue
  sendStatusMessage("System failed to reboot. Please disconnect and reconnect power.");
  while(1);
}