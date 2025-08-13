#if ARDUINO >= 100
#include "Arduino.h"
#include "Print.h"
#else
#include "WProgram.h"
#endif
#include <Arduino_GFX_Library.h>
#include "TimesNRCyr10.h"

#define BLACK 0x0000  // some extra colors
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define ORANGE 0xFBE0
#define GREY 0x84B5
#define BORDEAUX 0xA000
#define AFRICA 0xAB21
#define AMBER 0xFD00
 #define TEXTBGRND 0xc636
class Needle {
private:

  int center_x = 240;  // center x of dial on 240*240 TFT display
  int center_y = 160;  // center y of dial on 240*240 TFT display
  float filtVal;
  float p1_x, p1_y, ps1_x, ps1_y;
  float p1_x_old, p1_y_old, ps1_x_old, ps1_y_old;
  float angleOffset = 3.14;
  int radius = 234;  // center y of circular scale
  int hidden = 30;
  float needleAngle = 0;
  float needle_setter;
  float Value;
  float Old_Value = 23;

  int needlecolor;
  int scalecolor;
  int backcolor;
  int peakcolor;
  int y;
  float pivot_x, pivot_y;
  Arduino_GFX *gfx;
  bool reanimate = false;

public:
  int StartScale = 230;
  int EndScale = 1875;
  int Span;
  float filtCoeff = 0.05;

  Needle(Arduino_GFX *GFX, int Position, int NeedleColor, int ScaleColor, int BackColor, int PeakColor) {  // конструктор.
    gfx = GFX;
    y = Position;
    needlecolor = NeedleColor;
    scalecolor = ScaleColor;
    backcolor = BackColor;
    peakcolor = PeakColor;
    Span = EndScale - StartScale;
    pivot_x = 240;
    pivot_y = y + 264;
  };

  void Reanimate() {
    reanimate = true;
  }

  void Draw(int ADCCode) {
    Value = (ADCCode - StartScale) * 100 / Span;

    if ((abs(Old_Value - Value) >= 0.1) || reanimate) {

      gfx->setFont();
      gfx->setTextSize(2);
      gfx->setCursor(0, y + 5);
      gfx->setTextColor(BLACK);
      gfx->print("   -50  -40  -30  -20  -10    0    10");
      for (int x = 0; x < 5; x++) {
        gfx->writeSlashLine(x, y + 30, x + 72, y + 98, scalecolor);
        gfx->writeSlashLine(x + 60, y + 30, x + 114, y + 98, scalecolor);
        gfx->writeSlashLine(x + 120, y + 30, x + 156, y + 98, scalecolor);
        gfx->writeSlashLine(x + 180, y + 30, x + 198, y + 98, scalecolor);
        gfx->writeSlashLine(x + 240, y + 30, x + 240, y + 98, scalecolor);
        gfx->writeSlashLine(300 - x, y + 30, 282 - x, y + 98, scalecolor);  //ok
        gfx->writeSlashLine(360 - x, y + 30, 324 - x, y + 98, scalecolor);
        gfx->writeSlashLine(420 - x, y + 30, 366 - x, y + 98, peakcolor);
        gfx->writeSlashLine(480 - x, y + 30, 408 - x, y + 98, peakcolor);
      }
      for (int x = 0; x < 3; x++) {
        gfx->writeSlashLine(x + 30, y + 30, x + 61, y + 66, scalecolor);
        gfx->writeSlashLine(x + 90, y + 30, x + 112, y + 66, scalecolor);
        gfx->writeSlashLine(x + 150, y + 30, x + 163, y + 66, scalecolor);
        gfx->writeSlashLine(x + 210, y + 30, x + 214, y + 66, scalecolor);
        gfx->writeSlashLine(270 - x, y + 30, 265 - x, y + 66, scalecolor);  //ok
        gfx->writeSlashLine(330 - x, y + 30, 316 - x, y + 66, scalecolor);
        gfx->writeSlashLine(390 - x, y + 30, 367 - x, y + 66, peakcolor);
        gfx->writeSlashLine(450 - x, y + 30, 418 - x, y + 66, peakcolor);
      }
      gfx->setFont(&TimesNRCyr10pt8b);
      gfx->setTextSize(2);
      gfx->setCursor(220, y + 170);
      gfx->setTextColor(AFRICA);
      gfx->print("dB");

      if (Value < filtVal) {
        filtVal += ((Value - filtVal) * filtCoeff);  // filter
      } else filtVal = Value;
      needleAngle = filtVal*0.01745331 * 0.905 - 2.36;

      ps1_x = (pivot_x + ((hidden)*cos(needleAngle)));  // needle start point
      ps1_y = (pivot_y + ((hidden)*sin(needleAngle)));
      p1_x = (pivot_x + ((radius)*cos(needleAngle)));  // needle tip
      p1_y = (pivot_y + ((radius)*sin(needleAngle)));
        
      for (int x = 0; x < 5; x++) {
        gfx->writeSlashLine(ps1_x_old + x, ps1_y_old, p1_x_old + x, p1_y_old, backcolor);  // remove old needle
      }

      for (int x = 0; x < 5; x++) {
        gfx->writeSlashLine(ps1_x + x, ps1_y, p1_x + x, p1_y, needlecolor);  // create needle
      }

      p1_x_old = p1_x;
      p1_y_old = p1_y;
      ps1_x_old = ps1_x;
      ps1_y_old = ps1_y;
      reanimate = false;
    }

    Old_Value = filtVal;
  }
};