#ifndef BACKLIGHT_H
#define BACKLIGHT_H

// To call before using updateBacklightIfNecesary()
void initBacklight();

// Adjusts the backlight of the TFT screen according to a photosensor
void updateBacklightIfNecesary();

// There is an issue with the backlight PWM creating interferences with the touch data,
// so every time the user touches the screen, we call this function to put
// the brightness to 100% for a few minutes, and then we go back
// to brightness adjusted to ambiant light.
void adjustBacklightForTouch();

#endif
