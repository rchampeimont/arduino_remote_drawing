#ifndef BACKLIGHT_H
#define BACKLIGHT_H

// To call before using updateBacklightIfNecesary()
void initBacklight();

// Adjusts the backlight of the TFT screen according to a photosensor
void updateBacklightIfNecesary();

#endif
