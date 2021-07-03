/*  The code in this file is adapted from
   https://github.com/adafruit/Adafruit_RA8875/blob/master/examples/ts_calibration/ts_calibration.ino
   which was licensed under this license:

   Software License Agreement (BSD License)

   Copyright (c) 2020, Limor Fried for Adafruit Industries
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   3. Neither the name of the copyright holders nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
   EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <SPI.h>
#include <EEPROM.h>
#include "Adafruit_GFX.h"
#include "Adafruit_RA8875.h"

#include "calibrate.h"
#include "system.h"


extern Adafruit_RA8875 tft;
tsPoint_t       _tsLCDPoints[3];
tsPoint_t       _tsTSPoints[3];
tsMatrix_t      _tsMatrix;

/**************************************************************************/
/*!
    @brief Calculates the difference between the touch screen and the
           actual screen co-ordinates, taking into account misalignment
           and any physical offset of the touch screen.

    @note  This is based on the public domain touch screen calibration code
           written by Carlos E. Vidales (copyright (c) 2001).

           For more information, see the following app notes:

           - AN2173 - Touch Screen Control and Calibration
             Svyatoslav Paliy, Cypress Microsystems
           - Calibration in touch-screen systems
             Wendy Fang and Tony Chang,
             Analog Applications Journal, 3Q 2007 (Texas Instruments)
*/
/**************************************************************************/
int setCalibrationMatrix( tsPoint_t * displayPtr, tsPoint_t * screenPtr, tsMatrix_t * matrixPtr)
{
  int  retValue = 0;

  matrixPtr->Divider = ((screenPtr[0].x - screenPtr[2].x) * (screenPtr[1].y - screenPtr[2].y)) -
                       ((screenPtr[1].x - screenPtr[2].x) * (screenPtr[0].y - screenPtr[2].y)) ;

  if ( matrixPtr->Divider == 0 )
  {
    retValue = -1 ;
  }
  else
  {
    matrixPtr->An = ((displayPtr[0].x - displayPtr[2].x) * (screenPtr[1].y - screenPtr[2].y)) -
                    ((displayPtr[1].x - displayPtr[2].x) * (screenPtr[0].y - screenPtr[2].y)) ;

    matrixPtr->Bn = ((screenPtr[0].x - screenPtr[2].x) * (displayPtr[1].x - displayPtr[2].x)) -
                    ((displayPtr[0].x - displayPtr[2].x) * (screenPtr[1].x - screenPtr[2].x)) ;

    matrixPtr->Cn = (screenPtr[2].x * displayPtr[1].x - screenPtr[1].x * displayPtr[2].x) * screenPtr[0].y +
                    (screenPtr[0].x * displayPtr[2].x - screenPtr[2].x * displayPtr[0].x) * screenPtr[1].y +
                    (screenPtr[1].x * displayPtr[0].x - screenPtr[0].x * displayPtr[1].x) * screenPtr[2].y ;

    matrixPtr->Dn = ((displayPtr[0].y - displayPtr[2].y) * (screenPtr[1].y - screenPtr[2].y)) -
                    ((displayPtr[1].y - displayPtr[2].y) * (screenPtr[0].y - screenPtr[2].y)) ;

    matrixPtr->En = ((screenPtr[0].x - screenPtr[2].x) * (displayPtr[1].y - displayPtr[2].y)) -
                    ((displayPtr[0].y - displayPtr[2].y) * (screenPtr[1].x - screenPtr[2].x)) ;

    matrixPtr->Fn = (screenPtr[2].x * displayPtr[1].y - screenPtr[1].x * displayPtr[2].y) * screenPtr[0].y +
                    (screenPtr[0].x * displayPtr[2].y - screenPtr[2].x * displayPtr[0].y) * screenPtr[1].y +
                    (screenPtr[1].x * displayPtr[0].y - screenPtr[0].x * displayPtr[1].y) * screenPtr[2].y ;
  }

  return ( retValue ) ;
}

/**************************************************************************/
/*!
    @brief  Converts raw touch screen locations (tx and ty) into actual
            pixel locations on the display (dx and dy.

    @note  This is based on the public domain touch screen calibration code
           written by Carlos E. Vidales (copyright (c) 2001).
*/
/**************************************************************************/
int translateTouchCoords(uint16_t tx, uint16_t ty, int *dx, int *dy)
{
  int  retValue = 0 ;

  if ( _tsMatrix.Divider != 0 )
  {
    *dx = ( (_tsMatrix.An * tx) +
            (_tsMatrix.Bn * ty) +
            _tsMatrix.Cn
          ) / _tsMatrix.Divider ;

    *dy = ( (_tsMatrix.Dn * tx) +
            (_tsMatrix.En * ty) +
            _tsMatrix.Fn
          ) / _tsMatrix.Divider ;
  }
  else
  {
    return -1;
  }

  return ( retValue );
}

/**************************************************************************/
/*!
    @brief  Waits for a touch event
*/
/**************************************************************************/
void waitForTouchEvent(tsPoint_t * point)
{
  /* Clear the touch data object and placeholder variables */
  memset(point, 0, sizeof(tsPoint_t));

  /* Clear any previous interrupts to avoid false buffered reads */
  uint16_t x, y;
  tft.touchRead(&x, &y);
  delay(1);

  /* Wait around for a new touch event */
  while (! tft.touched());

  tft.touchRead(&x, &y);
  point->x = x;
  point->y = y;
}

/**************************************************************************/
/*!
    @brief  Renders the calibration screen with an appropriately
            placed test point and waits for a touch event
*/
/**************************************************************************/
tsPoint_t renderCalibrationScreen(uint16_t x, uint16_t y, uint16_t radius)
{
  tft.fillScreen(RA8875_BLACK);

  tft.textMode();
  const char msg[] = "We need to perform touch screen calibration. Please click on the red circles precisely.";
  tft.textSetCursor(0, 0);
  tft.textColor(RA8875_WHITE, RA8875_BLUE);
  tft.textWrite(msg, sizeof(msg) - 1);
  tft.graphicsMode();

  tft.fillCircle(x, y, radius, RA8875_RED);

  // Wait for a valid touch events
  tsPoint_t point = { 0, 0 };

  /* Keep polling until the TS event flag is valid */
  bool valid = false;
  while (!valid)
  {
    waitForTouchEvent(&point);
    if (point.x || point.y)
    {
      valid = true;
    }
  }

  return point;
}

/**************************************************************************/
/*!
    @brief  Starts the screen calibration process.  Each corner will be
            tested, meaning that each boundary (top, left, right and
            bottom) will be tested twice and the readings averaged.
*/
/**************************************************************************/
void tsCalibrate(void)
{
  tsPoint_t data;

  /* --------------- Welcome Screen --------------- */
  data = renderCalibrationScreen(tft.width() / 2, tft.height() / 2, 5);
  delay(500);

  /* ----------------- First Dot ------------------ */
  // 10% over and 10% down
  data = renderCalibrationScreen(tft.width() / 10, tft.height() / 10, 5);
  _tsLCDPoints[0].x = tft.width() / 10;
  _tsLCDPoints[0].y = tft.height() / 10;
  _tsTSPoints[0].x = data.x;
  _tsTSPoints[0].y = data.y;
  delay(500);

  /* ---------------- Second Dot ------------------ */
  // 50% over and 90% down
  data = renderCalibrationScreen(tft.width() / 2, tft.height() - tft.height() / 10, 5);
  _tsLCDPoints[1].x = tft.width() / 2;
  _tsLCDPoints[1].y = tft.height() - tft.height() / 10;
  _tsTSPoints[1].x = data.x;
  _tsTSPoints[1].y = data.y;
  delay(500);

  /* ---------------- Third Dot ------------------- */
  // 90% over and 50% down
  data = renderCalibrationScreen(tft.width() - tft.width() / 10, tft.height() / 2, 5);
  _tsLCDPoints[2].x = tft.width() - tft.width() / 10;
  _tsLCDPoints[2].y = tft.height() / 2;
  _tsTSPoints[2].x = data.x;
  _tsTSPoints[2].y = data.y;
  delay(500);

  /* Clear the screen */
  tft.fillScreen(RA8875_YELLOW);

  // Do matrix calculations for calibration
  setCalibrationMatrix(&_tsLCDPoints[0], &_tsTSPoints[0], &_tsMatrix);

  // Store matrix in EEPROM
  saveCalibrationData();

  // Wait for some time for user to release click.
  delay(500);
  // Clear input data
  if (tft.touched()) {
    uint16_t tmp_x, tmp_y;
    tft.touchRead(&tmp_x, &tmp_y);
  }
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void setupCalibration()
{
  pinMode(FORCE_CALIBRATE_PIN, INPUT_PULLUP);

  tft.fillScreen(RA8875_YELLOW);
  delay(100);

  /* Start the calibration process if needed. */
  if (digitalRead(FORCE_CALIBRATE_PIN) == LOW) {
    tsCalibrate();
  } else {
    if (loadCalibrationData() != 0) {
      tsCalibrate();
    } else {
    }
  }
}

// Save calibration data in EEPROM.
// I could not use writeCalibration from Adafruit_RA887
// because it does not work on Arduino Uno Wifi Rev2 (ATMEGA4809).
void saveCalibrationData() {
  // We will use string "CALI" to recognize that there is calibration data.
  EEPROM.put(0, "CALI");
  EEPROM.put(5, _tsMatrix);
}

// Try to load calibration data from EEPROM. Returns 0 on success.
int loadCalibrationData() {
  char s[5];
  EEPROM.get(0, s);
  if (strncmp(s, "CALI", 5) == 0) {
    EEPROM.get(5, _tsMatrix);
    return 0;
  } else {
    return 1;
  }
}
