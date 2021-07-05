// This file contains code to adjust TFT screen nacklight brightness
// automatically based on luminosity measure with a phototransistor.

#include "backlight.h"

// This is deliberately not a "round number" to avoid
// "synchronizing" with cycles of artifical lighting powered by AC.
#define CHECK_PHOTOSENSOR_EVERY 263

#define NUMBER_OF_BRIGHTNESS_VALUES_TO_AVERAGE 50

#define MINIMUM_BRIGHTNESS_CHANGE 5

// Reduce brightness according to ambiant light level after this time of inactivity
#define REDUCE_BRIGHTNESS_AFTER_MS 10000

byte brightnessValues[NUMBER_OF_BRIGHTNESS_VALUES_TO_AVERAGE];

unsigned long lastUserTouchTime = 0;
byte actualBrightness = 0;

// Return an optimal backlight brightness setting adjusted
// with room luminosity measured with phototransistor.
byte getOptimalBacklightFromSensor() {
  if (lastUserTouchTime == 0 || millis() >= lastUserTouchTime + REDUCE_BRIGHTNESS_AFTER_MS) {
    int photosensorValue = analogRead(PHOTOTRANSISTOR_PIN);
    return (byte) constrain(map(photosensorValue, 0, 900, 0, 255), 0, 255);
  } else {
    // The user is drawing something, so keep full brightness to avoid PWM-related interferences
    return 255;
  }
}

void initBacklight() {
  // Fill array with some initial measures
  for (byte i = 0; i < NUMBER_OF_BRIGHTNESS_VALUES_TO_AVERAGE; i++) {
    brightnessValues[i] = getOptimalBacklightFromSensor();
    delay(1);
  }
  tft.PWM1config(true, RA8875_PWM_CLK_DIV1024);
  updateBacklight();
}

void setBrightness(byte value) {
  actualBrightness = value;
  tft.PWM1out(value);
}

void updateBacklight() {
  static byte brightnessValueIndex = 0;

  // Add a new measure
  byte newValue = getOptimalBacklightFromSensor();
  brightnessValues[brightnessValueIndex] = newValue;
  brightnessValueIndex = (brightnessValueIndex + 1) % NUMBER_OF_BRIGHTNESS_VALUES_TO_AVERAGE;

  // Compute average of measures
  unsigned long total = 0;
  for (byte i = 0; i < NUMBER_OF_BRIGHTNESS_VALUES_TO_AVERAGE; i++) {
    total += brightnessValues[i];
  }
  byte newBrightnessAverage = total / NUMBER_OF_BRIGHTNESS_VALUES_TO_AVERAGE;

  //printStatusFormat("Theoretical brightness: %d / Actual brightness: %d / Last measure: %d", newBrightnessAverage, actualBrightness, newValue);

  // Avoid changing the brightness if the computed change is very small.
  // This is to avoid annoying flickering.
  if (newBrightnessAverage > actualBrightness + MINIMUM_BRIGHTNESS_CHANGE
      || newBrightnessAverage < actualBrightness - MINIMUM_BRIGHTNESS_CHANGE) {
    setBrightness(newBrightnessAverage);
  }
}

void updateBacklightIfNecesary() {
  static unsigned long lastPhotosensorCheckTime = millis();

  unsigned long now = millis();

  if (now >= lastPhotosensorCheckTime + CHECK_PHOTOSENSOR_EVERY) {
    updateBacklight();
    lastPhotosensorCheckTime = now;
  }
}

void adjustBacklightForTouch() {
  lastUserTouchTime = millis();

  if (actualBrightness != 255) {
    setBrightness(255);
    // Set all values at 255 to force the average to be 255
    for (byte i = 0; i < NUMBER_OF_BRIGHTNESS_VALUES_TO_AVERAGE; i++) {
      brightnessValues[i] = 255;
      delay(1);
    }
  }
}
