#include <ILI9488.h>

// ESP32-WROOM-DA Module
#include <Arduino_GFX_Library.h>
#include "Needle.cpp"
#include <Wire.h>
#include <BD37534FV.h>  // https://github.com/liman324/BD37534FV/archive/master.zip
#include <EEPROM.h>
#include "AiEsp32RotaryEncoder.h" https://github.com/igorantolic/ai-esp32-rotary-encoder
#include "AiEsp32RotaryEncoderNumberSelector.h"
#include <IRremote.hpp>
#include <IRreceive.hpp>

#define EEPROM_SIZE 20
#define TFT_CS1 13
#define TFT_CS2 2
#define TFT_RESET 16
#define TFT_DC 4
#define TFT_MOSI 23
#define TFT_SCK 18
#define TFT_LED 17
#define TFT_MISO -1     // not used for TFT
#define GFX_BL TFT_LED  // backlight pin

#define BTN_ON 14
#define BTN_ESC 14
#define ENC_S1 25
#define ENC_S2 26
#define ENC_KEY 27

#define IR 33
#define AUDIO_IN_PINL 34  // Signal in on this pin
#define AUDIO_IN_PINR 35  // Signal in on this pin

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
//#define TWILIGHT 0x95d9
#define BACKGROUND 0xef9d// 0xa616   
#define darkslategrey 0x328a
#define ROTARY_ENCODER_A_PIN 25
#define ROTARY_ENCODER_B_PIN 26
#define ROTARY_ENCODER_BUTTON_PIN 27

#define ROTARY_ENCODER_STEPS 4
#define MODE_RUN 0
#define MENU_LEVEL1 1
#define MENU_LEVEL2 2
#define MENU_OK 3
AiEsp32RotaryEncoder *rotaryEncoder = new AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, -1, ROTARY_ENCODER_STEPS);
AiEsp32RotaryEncoderNumberSelector numberSelector = AiEsp32RotaryEncoderNumberSelector();

BD37534FV bd;  //Объявляем bd для бибиотеки аудиопроцессора

double AnalogMaxScale = 4095;

int Volume = -50;
int Mode = 0;  // 0 - Work; 1,2...- Menu levels
bool Btn_Esc_Click = false;
bool Btn_Enter_Click = false;
int Level1_Index = 1;
int Level2_Index = 1;
unsigned int Color[10] = { CYAN, CYAN, CYAN, CYAN, CYAN, CYAN, CYAN, CYAN, CYAN, CYAN };
float BassQ[4] = { 0.5, 1.0, 1.5, 2.0 };
String BassF[4] = { "60Hz", "80Hz", "100Hz", "120Hz" };
float MiddleQ[4] = { 0.75, 1.0, 1.25, 1.5 };
String MiddleF[4] = { "500Hz", "1kHz", "1.5kHz", "2.5kHz" };
float TrebleQ[2] = { 0.75, 1.25 };
String TrebleF[4] = { "7.5kHz", "10kHz", "12.5kHz", "15kHz" };
String YesNo[2] = { "No", "Yes" };
int8_t treb, middle, bass, gain, in, sub, yesno;
int8_t treb_f, mid_f, bas_f, sub_f, treb_q, mid_q, bas_q;
struct Border {
  int min;
  int max;
};
Border borders[10] = {
  { 0, 3 },
  { 0, 3 },
  { -20, 20 },
  { 0, 3 },
  { 0, 3 },
  { -20, 20 },
  { 0, 1 },
  { 0, 3 },
  { -20, 20 },
  { 0, 1 }
};
unsigned long lastOnTime = millis();
unsigned long lastChangeVolumeTime = millis();
unsigned long lastEscButtonTime = millis();
unsigned long lastEnterButtonTime = millis();
int on = 0;
int center_x = 240;
int center_y = 160;
bool EscButton, lastEscButton, EnterButton, lastEnterButton;
;
/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, false, TFT_SCK, TFT_MOSI, TFT_MISO, HSPI, true);

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, TFT_RESET, 1 /* rotation */, false /* IPS */);
//Arduino_GFX *gfxR = new Arduino_ILI9488_18bit(bus, TFT_RESET, 3 /* rotation */, false /* IPS */);
//Arduino_GFX *gfx;

Needle NeedleLeft = Needle(gfx, 10, BLACK, BLUE, BACKGROUND, BLUE);
Needle NeedleRight = Needle(gfx, 10, BLACK, BLUE, BACKGROUND, BLUE);

/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/
void LeftTFT() {
  digitalWrite(TFT_CS2, HIGH);
  digitalWrite(TFT_CS1, LOW);
  gfx->setRotation(1);
 // gfx = gfxL;
}
void RightTFT() {
  digitalWrite(TFT_CS1, HIGH);
  digitalWrite(TFT_CS2, LOW);
    gfx->setRotation(1);
 // gfx = gfxL;
}
void AllTFT() {
  digitalWrite(TFT_CS1, LOW);
  digitalWrite(TFT_CS2, LOW);
}

void IRAM_ATTR readEncoderISR() {
  rotaryEncoder->readEncoder_ISR();
}

bool Button_Esc_Click() {
  bool click = false;
  bool Btn_Esc = digitalRead(BTN_ESC);
  if (Btn_Esc && !Btn_Esc_Click) {
    click = true;
  } else {
    click = false;
  }
  Btn_Esc_Click = Btn_Esc;
  return click;
}
bool Button_Enter_Click() {
  bool click = false;
  bool Btn_Enter = digitalRead(ENC_KEY);
  if (Btn_Enter && !Btn_Enter_Click) {
    click = true;
  } else {
    click = false;
  }
  Btn_Enter_Click = Btn_Enter;
  return click;
}
void MainScreen() {
  numberSelector.setRange(-79, 15, 1.0, false, 1);
  numberSelector.setValue(Volume);
  AllTFT();
  gfx->fillScreen(BACKGROUND);
  gfx->setFont(&TimesNRCyr10pt8b);
  gfx->setTextSize(2);
  gfx->setTextColor(WHITE);
  LeftTFT();
  gfx->fillRect(5, 260, 470, 55, darkslategrey);
  gfx->setCursor(110, 300);
  gfx->print("Volume");
  RightTFT();
  gfx->fillRect(5, 260, 470, 55, darkslategrey);
  gfx->setCursor(170, 300);
  gfx->print("Input");
  Draw_Input();
  Draw_Volume();
  NeedleLeft.Reanimate();
  NeedleRight.Reanimate();
}
void MenuLevel_1_Screen() {
  RightTFT();
  gfx->fillScreen(BLACK);
  numberSelector.setRange(0, 9, 1, false, 1);
  numberSelector.setValue(Level1_Index);
  for (int i = 0; i <= 9; i++) {
    Color[i] = CYAN;
  }
  Color[Level1_Index] = YELLOW;
}
void MenuLevel_2_Screen() {
  RightTFT();
  gfx->fillRect(100, 135, 300, 50, BLUE);
  switch (Level1_Index) {
    case 0:
      numberSelector.setRange(0, 3, 1, false, 1);
      numberSelector.setValue(bas_q);
      Level2_Index = bas_q;
      break;
    case 1:
      numberSelector.setRange(0, 3, 1, false, 1);
      numberSelector.setValue(bas_f);
      Level2_Index = bas_f;
      break;
    case 2:
      numberSelector.setRange(-20, 20, 1, false, 1);
      numberSelector.setValue(bass);
      Level2_Index = bass;
      break;
    case 3:
      numberSelector.setRange(0, 3, 1, false, 1);
      numberSelector.setValue(mid_q);
      Level2_Index = mid_q;
      break;
    case 4:
      numberSelector.setRange(0, 3, 1, false, 1);
      numberSelector.setValue(mid_f);
      Level2_Index = mid_f;
      break;
    case 5:
      numberSelector.setRange(-20, 20, 1, false, 1);
      numberSelector.setValue(middle);
      Level2_Index = middle;
      break;
    case 6:
      numberSelector.setRange(0, 1, 1, false, 1);
      numberSelector.setValue(treb_q);
      Level2_Index = treb_q;
      break;
    case 7:
      numberSelector.setRange(0, 3, 1, false, 1);
      numberSelector.setValue(treb_f);
      Level2_Index = treb_f;
      break;
    case 8:
      numberSelector.setRange(-20, 20, 1, false, 1);
      numberSelector.setValue(treb);
      Level2_Index = treb;
      break;
    case 9:
      numberSelector.setRange(0, 1, 1, false, 1);
      numberSelector.setValue(yesno);
      Level2_Index = yesno;
      break;
    default:
      break;
  }
}
void Draw_Volume() {
  LeftTFT();
  gfx->setFont(&TimesNRCyr10pt8b);
  gfx->setTextSize(2);
 // gfx->setCursor(150, 300);
 // gfx->fillRect(5, 260, 470, 55, darkslategrey);
  gfx->setTextColor(WHITE);
//gfx->print("Volume");
  gfx->fillRect(260, 270, 120, 35, darkslategrey);
  gfx->setCursor(260, 300);
  //gfx->setTextColor(YELLOW);
  gfx->print(Volume);
  gfx->print(" dB");
}
void Draw_Input() {
  RightTFT();
  gfx->setFont(&TimesNRCyr10pt8b);
  gfx->setTextSize(2);
  //gfx->setCursor(170, 300);
  //gfx->fillRect(5, 260, 470, 55, darkslategrey);
  gfx->setTextColor(WHITE);
  //gfx->print("Input");
  gfx->fillRect(300, 270, 60, 35, darkslategrey);
  gfx->setCursor(300, 300);
  //gfx->setTextColor(YELLOW);
  gfx->print(in + 1);
}
//-------------------------------------------------------------
void Menu() {

  if (Button_Esc_Click() && Mode > 0) {
    Mode--;
    RightTFT();
    gfx->fillScreen(BACKGROUND);
    if (Mode == MODE_RUN) {
      MainScreen();
    }
    if (Mode == MENU_LEVEL1) {
      MenuLevel_1_Screen();
    }
  }

  if (rotaryEncoder->isEncoderButtonClicked() && Mode < 2) {
    Mode++;
    RightTFT();

    if (Mode == MENU_LEVEL1) {
      MenuLevel_1_Screen();
    }
    if (Mode == MENU_LEVEL2) {
      MenuLevel_2_Screen();
    }
  }
  //
  if (rotaryEncoder->isEncoderButtonClicked() && Mode == MENU_LEVEL2) {
    Mode--;
    RightTFT();
    MenuLevel_1_Screen();
  }

  if (Mode == MENU_LEVEL1) {
    RightTFT();

    if (rotaryEncoder->encoderChanged()) {
      for (int i = 0; i <= 9; i++) {
        Color[i] = CYAN;
      }
      Level1_Index = numberSelector.getValue();
      Color[Level1_Index] = YELLOW;
    }
    gfx->setFont(&TimesNRCyr10pt8b);
    gfx->setTextSize(2);
    gfx->setCursor(160, 30);
    gfx->setTextColor(CYAN);
    gfx->println("Settings");
    gfx->setTextSize(1);
    gfx->setTextColor(Color[0]);
    gfx->setCursor(0, 55);
    gfx->print("     Bass Q factor    ");
    gfx->println(BassQ[bas_q]);  // 0.5 1.0 1.5 2.0 bas_q 0...3
    gfx->setTextColor(Color[1]);
    gfx->print("     Bass frequency    ");
    gfx->println(BassF[bas_f]);  //  60Hz 80Hz 100Hz 120Hz bas_c 0...3
    gfx->setTextColor(Color[2]);
    gfx->print("     Bass gain    ");
    gfx->println(bass);  // -20 ... +20 dB = int -20 ... 20
    gfx->println("");
    gfx->setTextColor(Color[3]);
    gfx->print("     Middle Q factor    ");
    gfx->println(MiddleQ[mid_q]);  // 0.75 1.0 1.25 1.5 = int 0...3
    gfx->setTextColor(Color[4]);
    gfx->print("     Middle frequency    ");
    gfx->println(MiddleF[mid_f]);  // 500Hz 1kHz 1.5kHz 2.5kHz = int 0...3
    gfx->setTextColor(Color[5]);
    gfx->print("     Middle gain    ");
    gfx->println(middle);  // -20 ... +20 dB = int -20 ... 20
    gfx->println("");
    gfx->setTextColor(Color[6]);
    gfx->print("     Treble Q factor    ");
    gfx->println(TrebleQ[treb_q]);  // 0.75 1.25   = int 0...1
    gfx->setTextColor(Color[7]);
    gfx->print("     Treble frequency    ");
    gfx->println(TrebleF[treb_f]);  // 7.5kHz 10kHz 12.5kHz 15kHz = int 0...3
    gfx->setTextColor(Color[8]);
    gfx->print("     Treble gain    ");
    gfx->println(treb);  // -20 ... +20 dB = int -20 ... 20
    gfx->setTextColor(Color[9]);
    gfx->print("     Save settings    ");
  }

  if (Mode == MENU_LEVEL2) {

    RightTFT();

    if (rotaryEncoder->encoderChanged()) {
      Level2_Index = numberSelector.getValue();
      gfx->fillRect(100, 135, 300, 50, BLUE);
    }
    //gfx->fillRect(100, 135, 300, 50, BLUE);
    gfx->setFont(&TimesNRCyr10pt8b);
    gfx->setTextSize(1);
    gfx->setCursor(120, 165);
    gfx->setTextColor(WHITE);
    switch (Level1_Index) {
      case 0:
        bas_q = Level2_Index;
        bd.setBass_setup(bas_q, bas_f);
        gfx->print("Bass Q factor    ");
        gfx->println(BassQ[bas_q]);  // 0.5 1.0 1.5 2.0 bas_q 0...3
        break;
      case 1:
        bas_f = Level2_Index;
        bd.setBass_setup(bas_q, bas_f);
        gfx->print("Bass frequency    ");
        gfx->println(BassF[bas_f]);  //  60Hz 80Hz 100Hz 120Hz bas_c 0...3
        break;
      case 2:
        bass = Level2_Index;
        bd.setBass_gain(bass);
        gfx->print("Bass gain    ");
        gfx->println(bass);  // -20 ... +20 dB = int -20 ... 20
        break;
      case 3:
        mid_q = Level2_Index;
        bd.setBass_setup(mid_q, mid_f);
        gfx->print("Middle Q factor    ");
        gfx->println(MiddleQ[mid_q]);  // 0.75 1.0 1.25 1.5 = int 0...3
        break;
      case 4:
        mid_f = Level2_Index;
        bd.setBass_setup(mid_q, mid_f);
        gfx->print("Middle frequency    ");
        gfx->println(MiddleF[mid_f]);  // 500Hz 1kHz 1.5kHz 2.5kHz = int 0...3
        break;
      case 5:
        middle = Level2_Index;
        bd.setMiddle_gain(middle);
        gfx->print("Middle gain    ");
        gfx->println(middle);  // -20 ... +20 dB = int -20 ... 20

        break;
      case 6:
        treb_q = Level2_Index;
        bd.setBass_setup(treb_q, treb_f);
        gfx->print("Treble Q factor    ");
        gfx->println(TrebleQ[treb_q]);  // 0.75 1.25   = int 0...1
        break;
      case 7:
        treb_f = Level2_Index;
        bd.setBass_setup(treb_q, treb_f);
        gfx->print("Treble frequency    ");
        gfx->println(TrebleF[treb_f]);  // 7.5kHz 10kHz 12.5kHz 15kHz = int 0...3
        break;
      case 8:
        treb = Level2_Index;
        bd.setTreble_gain(treb);
        gfx->print("Treble gain    ");
        gfx->println(treb);  // -20 ... +20 dB = int -20 ... 20
        break;
      case 9:
        yesno = Level2_Index;
        gfx->print("Save settings    ");
        gfx->println(YesNo[yesno]);  // "Yes" , "No" = int 0..1
        break;
      default:
        break;
    }
  }
}

void Vol_Up(void) {
  if (on) {
    // IR_Monitor_Off();
    Volume++;
    Draw_Volume();
    bd.setVol(Volume);
    delay(100);
    // IR_Monitor_On();
  }
}

void Vol_Down(void) {
  if (on) {
    // IR_Monitor_Off();
    Volume--;
    Draw_Volume();
    bd.setVol(Volume);
    delay(100);
    //  IR_Monitor_On();
  }
}

void FadeOut() {
  for (int BRIGHTNESS = 255; BRIGHTNESS > 0; BRIGHTNESS--) {
    analogWrite(TFT_LED, BRIGHTNESS);
    delay(20);
  }
}

void FadeIn() {
  for (int BRIGHTNESS = 0; BRIGHTNESS < 255; BRIGHTNESS++) {
    analogWrite(TFT_LED, BRIGHTNESS);
    delay(20);
  }
}

void Switch_On_Off() {
  gfx->setFont(&TimesNRCyr10pt8b);
  gfx->setTextSize(2);
  gfx->setTextColor(GREEN);
  on = !on;
  AllTFT();
  gfx->fillScreen(BLACK);
  if (on) {
    gfx->setCursor(center_x - 65, center_y - 20);
    gfx->print("Hello!");
    FadeIn();
    MainScreen();
  } else {
    gfx->setCursor(center_x - 80, center_y - 20);
    gfx->print("Goodbye!");
    FadeOut();
    AllTFT();
    gfx->fillScreen(BLACK);
  }
}

//===============================================================================================================
void setup(void) {
  on = 1;
  Wire.begin();
  EEPROM.begin(EEPROM_SIZE);

  EEPROM.get(11, Volume);  // Read Data from EEPROM
  EEPROM.get(1, treb);
  EEPROM.get(2, middle);
  EEPROM.get(3, bass);
  EEPROM.get(4, in);
  EEPROM.get(5, treb_f);
  EEPROM.get(6, mid_f);
  EEPROM.get(7, bas_f);
  EEPROM.get(8, treb_q);
  EEPROM.get(9, mid_q);
  EEPROM.get(10, bas_q);

  IrReceiver.begin(IR, ENABLE_LED_FEEDBACK);

  gfx->begin(70000000);
  gfx->fillScreen(BACKGROUND);
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);
  pinMode(TFT_RESET, OUTPUT);
  pinMode(TFT_CS1, OUTPUT);
  pinMode(TFT_CS2, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(BTN_ON, INPUT);
  pinMode(BTN_ESC, INPUT);
  pinMode(ENC_S1, INPUT);
  pinMode(ENC_S2, INPUT);
  pinMode(ENC_KEY, INPUT);
  pinMode(IR, INPUT);
  NeedleLeft.filtCoeff = 0.4;
  NeedleRight.filtCoeff = 0.4;
  Serial.begin(9600);

  rotaryEncoder->begin();
  rotaryEncoder->setup(readEncoderISR);
  rotaryEncoder->setAcceleration(10);
  numberSelector.attachEncoder(rotaryEncoder);
  numberSelector.setRange(-79, 15, 1.0, false, 1);
  numberSelector.setValue(Volume);

  bd.setLoudness_f(0);                 // Loudness fo (250Hz 400Hz 800Hz Prohibition) = lon_f (0...3)
  bd.setIn(in);                        // Input Selector (A, B, C, D single, E1 single, E2 single) = in (0...6)
  bd.setIn_gain(0, 0);                 // Input Gain (0...20dB) = gain (0...20), Mute ON/OFF (1-on 0-off) = mute (0...1)
  bd.setVol(Volume);                   // -79...+15 dB = int -79...15
  bd.setFront_1(0);                    // -79...+15 dB = int -79...15
  bd.setFront_2(0);                    // -79...+15 dB = int -79...15
  bd.setRear_1(0);                     // -79...+15 dB = int -79...15
  bd.setRear_2(0);                     // -79...+15 dB = int -79...15
  bd.setSub(0);                        // -79...+15 dB = int -79...15
  bd.setBass_setup(bas_q, bas_f);      // 0.5 1.0 1.5 2.0 bas_q 0...3, 60Hz 80Hz 100Hz 120Hz bas_c 0...3
  bd.setMiddle_setup(mid_q, mid_f);    // 0.75 1.0 1.25 1.5 = int 0...3, 500Hz 1kHz 1.5kHz 2.5kHz = int 0...3
  bd.setTreble_setup(treb_q, treb_f);  // 0.75 1.25         = int 0...1, 7.5kHz 10kHz 12.5kHz 15kHz = int 0...3
  bd.setMiddle_gain(middle);           // -20 ... +20 dB = int -20 ... 20
  bd.setTreble_gain(treb);             // -20 ... +20 dB = int -20 ... 20
  bd.setBass_gain(bass);               // -20 ... +20 dB = int -20 ... 20
  bd.setLoudness_gain(0);              //   0 ... +20 dB = int   0 ... 20
                                       //bd.setSetup_2(sub_f, sub_out, 0, faza);
  MainScreen();
  Draw_Volume();
  Draw_Input();
  Level1_Index = 0;
  Serial.println("Setup done");
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void loop() {


  // "Power(Esc)" button
  EscButton = !digitalRead(BTN_ON);
  if (EscButton) {
    if (EscButton && (millis() - lastEscButtonTime > 2000)) {
      Switch_On_Off();
    }
  } else {
    lastEscButtonTime = millis();
  }


  if (IrReceiver.decode()) {
    int Command = IrReceiver.decodedIRData.command;
    switch (Command) {
      case 64:
        if (millis() - lastOnTime > 500) {  // If it's been at least 1/2 second since the last
          // IR received, toggle the relay
          Switch_On_Off();
        }
        break;
      case 25:  // Esc
        if (millis() - lastOnTime > 500) {
          if (Mode == MENU_LEVEL2) {
            Mode = MENU_LEVEL1;
            RightTFT();
            MenuLevel_1_Screen();
            break;
          }
          if (Mode == MENU_LEVEL1) {
            Mode = MODE_RUN;
            RightTFT();
            gfx->fillScreen(BACKGROUND);
            MainScreen();
          }
          break;
        }
      case 17:  // Home
        Mode = MODE_RUN;
        MainScreen();
        break;
      case 24:  // Vol+
        Vol_Up();
        break;
      case 16:  // Vol-
        Vol_Down();
        break;
      case 76:  // Menu
        Mode = MENU_LEVEL1;
        MenuLevel_1_Screen();
        break;
      case 22:  // Menu Up
        if (millis() - lastOnTime > 250) {
          if ((Mode == MENU_LEVEL1) & (Level1_Index > 0)) {
            Level1_Index--;
            numberSelector.setValue(Level1_Index);
            for (int i = 0; i <= 9; i++) {
              Color[i] = CYAN;
            }
            Color[Level1_Index] = YELLOW;
          }
        }
        if (Mode == MODE_RUN) {
          Vol_Up();
        }
        break;
      case 26:  // Menu Down
        if (millis() - lastOnTime > 250) {
          if ((Mode == MENU_LEVEL1) & (Level1_Index < 10)) {
            Level1_Index++;
            numberSelector.setValue(Level1_Index);
            for (int i = 0; i <= 9; i++) {
              Color[i] = CYAN;
            }
            Color[Level1_Index] = YELLOW;
          }
        }
        if (Mode == MODE_RUN) {
          Vol_Down();
        }
        break;
      case 81:  // Menu Left
        if (millis() - lastOnTime > 250) {
          if ((Mode == MENU_LEVEL2) & (Level2_Index > borders[Level1_Index].min)) {
            Level2_Index--;
            RightTFT();
            gfx->fillRect(100, 135, 300, 50, BLUE);
          }
          if (Mode == MODE_RUN) {
            in--;
            if (in < 0) {
              in = 2;
            }
            bd.setIn(in);
            Draw_Input();
          }
        }
        break;
      case 80:  // Menu Right
        if (millis() - lastOnTime > 250) {
          if ((Mode == MENU_LEVEL2) & (Level2_Index < borders[Level1_Index].max)) {
            Level2_Index++;
            RightTFT();
            gfx->fillRect(100, 135, 300, 50, BLUE);
          }
          if (Mode == MODE_RUN) {
            in++;
            if (in > 2) {
              in = 0;
            }
            bd.setIn(in);
            Draw_Input();
          }
        }
        break;
      case 19:  // OK
        if (millis() - lastOnTime > 500) {
          if (Mode == MENU_LEVEL1) {
            Mode = MENU_LEVEL2;
            MenuLevel_2_Screen();
            break;
          }
          if (Mode == MENU_LEVEL2) {
            Mode = MENU_LEVEL1;
            MenuLevel_1_Screen();
          }
          break;
        }
      default:
        break;
    }

    lastOnTime = millis();
    lastChangeVolumeTime = millis();
    if (Mode == MODE_RUN) {
      Draw_Volume();
      numberSelector.setValue(Volume);
    }
    Serial.print("command : ");
    Serial.print(IrReceiver.decodedIRData.command, HEX);
    Serial.println("");

    IrReceiver.resume();
  } else if (millis() - lastChangeVolumeTime > 100) {
    //--
    lastChangeVolumeTime = millis();
    //--
  }

  if (on) {

    Menu();

    LeftTFT();
    NeedleLeft.Draw(analogRead(AUDIO_IN_PINL));

    if (Mode == MODE_RUN) {

      if (rotaryEncoder->encoderChanged()) {
        Volume = numberSelector.getValue();
        bd.setVol(Volume);
        Draw_Volume();
      }

      RightTFT();
      NeedleRight.Draw(analogRead(AUDIO_IN_PINR));

      // Input selection
      if (Button_Esc_Click()) {
        in++;
        if (in > 2) {
          in = 0;
        }
        bd.setIn(in);
        Draw_Input();
      }
    }

    if (yesno == 1) {
      EEPROM.put(11, Volume);
      EEPROM.put(1, treb);
      EEPROM.put(2, middle);
      EEPROM.put(3, bass);
      EEPROM.put(4, in);
      EEPROM.put(5, treb_f);
      EEPROM.put(6, mid_f);
      EEPROM.put(7, bas_f);
      EEPROM.put(8, treb_q);
      EEPROM.put(9, mid_q);
      EEPROM.put(10, bas_q);
      EEPROM.commit();
      yesno = 0;
    }
  }
}
