#include "DFRobot_DF2301Q.h"
#include <Adafruit_NeoPixel.h>
#include "effects.h"

#define NUM_LEDS 60
#define DATA_PIN 4
#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 5
#define BRIGHTNESS_STEP 30
#define CCT_STEP 10

Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
DFRobot_DF2301Q_I2C asr;

bool isOn = true;
int currentBrightness = 80;
uint32_t currentColor = 0;
uint8_t rainbowHue = 0;
int colorTemp = 50;

enum Mode {
  MODE_STATIC,
  MODE_RAINBOW,
  MODE_BREATHING,
  MODE_CHASE,
  MODE_TWINKLE,
  MODE_FIRE,
  MODE_ALARM,
  MODE_RELAX
};
Mode currentMode = MODE_STATIC;

unsigned long lastUpdate = 0;
int breathBrightness = 0;
bool breathDirection = true;
int chasePosition = 0;
bool alarmState = false;
int relaxBrightness = 5;
bool relaxDirection = true;


struct ColorCommand {
  uint8_t cmdID;
  uint8_t r, g, b;
};

const ColorCommand colorCommands[] = {
  {116, 255, 0,   0},
  {117, 255, 128, 0},
  {118, 255, 255, 0},
  {119, 0,   255, 0},
  {120, 0,   255, 255},
  {121, 0,   128, 255},
  {122, 153, 51,  255},
  {123, 255, 255, 255},
};
const int colorCount = sizeof(colorCommands) / sizeof(colorCommands[0]);

struct ModeCommand {
  uint8_t cmdID;
  int cctTemp;
  int brightness;
};

const ModeCommand modeCommands[] = {
  {113, 100, 200},
  {114, 20,  15},
  {128, 90,  150},
  {129, 10,  150},
};
const int modeCount = sizeof(modeCommands) / sizeof(modeCommands[0]);

struct BrightnessCommand {
  uint8_t cmdID;
  int value;
  bool isAbsolute;
};

const BrightnessCommand brightnessCommands[] = {
  {105,  BRIGHTNESS_STEP, false},
  {106, -BRIGHTNESS_STEP, false},
  {107,  MAX_BRIGHTNESS,  true},
  {108,  MIN_BRIGHTNESS,  true},
};
const int brightnessCount = sizeof(brightnessCommands) / sizeof(brightnessCommands[0]);

struct EffectCommand {
  uint8_t cmdID;
  Mode mode;
  int brightness;
};

const EffectCommand effectCommands[] = {
  {5,  MODE_TWINKLE,   -1},
  {6,  MODE_FIRE,      -1},
  {7,  MODE_BREATHING, -1},
  {8,  MODE_CHASE,     -1},
  {9,  MODE_ALARM,     200},
  {115, MODE_RAINBOW, -1},
  {10, MODE_RELAX,     -1},
};
const int effectCount = sizeof(effectCommands) / sizeof(effectCommands[0]);

struct EffectTick {
  Mode mode;
  void (*tickFn)();
  unsigned long interval;
};

const EffectTick effectTicks[] = {
  {MODE_BREATHING, tickBreathing, 15},
  {MODE_CHASE,     tickChase,     40},
  {MODE_TWINKLE,   tickTwinkle,   80},
  {MODE_FIRE,      tickFire,      120},
  {MODE_ALARM,     tickAlarm,     300},
  {MODE_RAINBOW, tickRainbow, 20},
  {MODE_RELAX,     tickRelax,     30},
};
const int tickCount = sizeof(effectTicks) / sizeof(effectTicks[0]);

uint32_t cctToColor(int temp) {
  temp = constrain(temp, 0, 100);
  uint8_t r = map(temp, 0, 100, 255, 180);
  uint8_t g = map(temp, 0, 100, 180, 220);
  uint8_t b = map(temp, 0, 100, 80, 255);
  return strip.Color(r, g, b);
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  currentMode = MODE_STATIC;
  currentColor = strip.Color(r, g, b);
  if (isOn) { strip.fill(currentColor); strip.show(); }
}

void setBrightness(int brightness) {
  currentBrightness = constrain(brightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  strip.setBrightness(currentBrightness);
  strip.show();
}

void applyColorTemp() {
  currentMode = MODE_STATIC;
  currentColor = cctToColor(colorTemp);
  if (isOn) { strip.fill(currentColor); strip.show(); }
}

void adjustCCT(int delta) {
  colorTemp = constrain(colorTemp + delta, 0, 100);
  applyColorTemp();
}

void activateMode(Mode mode, int brightness = -1) {
  currentMode = mode;
  isOn = true;
  breathBrightness = 0;
  breathDirection = true;
  chasePosition = 0;
  rainbowHue = 0;
  alarmState = false;
  relaxBrightness = 5;
  relaxDirection = true;
  if (brightness >= 0) strip.setBrightness(brightness);
  else strip.setBrightness(currentBrightness);
}

void setup() {
  Serial.begin(115200);
  Serial.println("STARTING...\n");

  strip.begin();
  strip.setBrightness(currentBrightness);
  strip.clear();
  strip.show();
  strip.fill(strip.Color(0, 0, 255));
  strip.show();
  Serial.println("LEDS INITIALIZED\n");

  delay(2000);
  Wire.begin(16, 17);
  while (!(asr.begin())) {
    Serial.println("Communication with device failed");
    strip.fill(strip.Color(255, 0, 0));
    strip.show();
    delay(3000);
  }
  Serial.println("DFROBOT INITIALIZED\n");

  asr.setVolume(3);
  asr.setMuteMode(0);
  asr.setWakeTime(20);

  startupAnimation();
  Serial.println("READY FOR WORK");
}

void loop() {
  uint8_t CMDID = asr.getCMDID();

  if (CMDID != 0) {
    unsigned long responseStart = millis();

    for (int i = 0; i < colorCount; i++) {
      if (CMDID == colorCommands[i].cmdID) {
        setColor(colorCommands[i].r, colorCommands[i].g, colorCommands[i].b);
        Serial.print("Color: "); Serial.println(CMDID); Serial.print(" ");
        CMDID = 0; break;
      }
    }

    for (int i = 0; i < effectCount; i++) {
      if (CMDID == effectCommands[i].cmdID) {
        activateMode(effectCommands[i].mode, effectCommands[i].brightness);
        Serial.print("Effect: "); Serial.println(CMDID); Serial.print(" ");
        CMDID = 0; break;
      }
    }

    for (int i = 0; i < brightnessCount; i++) {
      if (CMDID == brightnessCommands[i].cmdID) {
        if (brightnessCommands[i].isAbsolute)
          setBrightness(brightnessCommands[i].value);
        else
          setBrightness(currentBrightness + brightnessCommands[i].value);
        Serial.print("Brightness: "); Serial.println(CMDID); Serial.print(" ");
        CMDID = 0; break;
      }
    }

    for (int i = 0; i < modeCount; i++) {
      if (CMDID == modeCommands[i].cmdID) {
        colorTemp = modeCommands[i].cctTemp;
        applyColorTemp();
        setBrightness(modeCommands[i].brightness);
        Serial.print("Mode: "); Serial.println(CMDID); Serial.print(" ");
        CMDID = 0; break;
      }
    }

    switch (CMDID) {
      case 109: adjustCCT(+CCT_STEP); Serial.println("CCT+");    break;
      case 110: adjustCCT(-CCT_STEP); Serial.println("CCT-");    break;
      case 111: adjustCCT(+100);      Serial.println("CCT max"); break;
      case 112: adjustCCT(-100);      Serial.println("CCT min"); break;
      case 103:
        isOn = true;
        currentMode = MODE_STATIC;
        strip.setBrightness(currentBrightness);
        strip.fill(currentColor);
        strip.show();
        Serial.println("ON");
        break;
      case 104:
        isOn = false;
        currentMode = MODE_STATIC;
        strip.clear();
        strip.show();
        Serial.println("OFF");
        break;
      default:
        if (CMDID != 0) {
          Serial.print("Unknown: "); Serial.println(CMDID);Serial.print(" ");
        }
        break;
    }
  }

  if (!isOn && currentMode != MODE_ALARM) {
    delay(100);
    return;
  }

  unsigned long now = millis();
  for (int i = 0; i < tickCount; i++) {
    if (currentMode == effectTicks[i].mode) {
      if (now - lastUpdate > effectTicks[i].interval) {
        effectTicks[i].tickFn();
        lastUpdate = now;
      }
      break;
    }
  }
}