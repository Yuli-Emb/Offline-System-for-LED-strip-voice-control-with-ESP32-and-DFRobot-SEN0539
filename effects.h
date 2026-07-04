#ifndef EFFECTS_H
#define EFFECTS_H

#include <Adafruit_NeoPixel.h>
#include <math.h>

extern Adafruit_NeoPixel strip;
extern int currentBrightness;
extern uint32_t currentColor;
extern int relaxBrightness;
extern bool relaxDirection;
extern bool alarmState;
extern int breathBrightness;
extern bool breathDirection;
extern int chasePosition;
extern uint8_t rainbowHue;

void tickBreathing() {
  strip.setBrightness(breathBrightness);
  strip.fill(currentColor);
  strip.show();
  if (breathDirection) {
    breathBrightness += 3;
    if (breathBrightness >= currentBrightness) breathDirection = false;
  } else {
    breathBrightness -= 3;
    if (breathBrightness <= 0) breathDirection = true;
  }
}

void tickChase() {
  strip.clear();
  strip.setPixelColor(chasePosition, currentColor);
  for (int t = 1; t <= 4; t++) {
    int pos = (chasePosition - t + strip.numPixels()) % strip.numPixels();
    uint8_t r = ((currentColor >> 16) & 0xFF) / (t * 2);
    uint8_t g = ((currentColor >> 8)  & 0xFF) / (t * 2);
    uint8_t b = ((currentColor)        & 0xFF) / (t * 2);
    strip.setPixelColor(pos, strip.Color(r, g, b));
  }
  strip.show();
  chasePosition = (chasePosition + 1) % strip.numPixels();
}

void tickTwinkle() {
  int pixel = random(strip.numPixels());
  if (random(2)) {
    strip.setPixelColor(pixel, currentColor);
  } else {
    strip.setPixelColor(pixel, 0);
  }
  strip.show();
}

void tickFire() {
  for (int i = 0; i < strip.numPixels(); i++) {
    int flicker = random(80, 180);
    int green = random(10, 50);
    strip.setPixelColor(i, strip.Color(flicker, green, 0));
  }
  strip.show();
}

void tickAlarm() {
  if (alarmState) {
    strip.fill(strip.Color(255, 0, 0));
  } else {
    strip.clear();
  }
  strip.show();
  alarmState = !alarmState;
}

void tickRelax() {
  strip.setBrightness(relaxBrightness);
  strip.fill(strip.Color(255, 147, 41));
  strip.show();
  if (relaxDirection) {
    relaxBrightness++;
    if (relaxBrightness >= 60) relaxDirection = false;
  } else {
    relaxBrightness--;
    if (relaxBrightness <= 5) relaxDirection = true;
  }
}

void startupAnimation() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 100, 255));
    strip.setPixelColor((i + 1) % strip.numPixels(), strip.Color(0, 50, 128));
    strip.show();
    delay(40);
  }
  strip.clear();
  strip.show();
}

void tickRainbow() {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.gamma32(
      strip.ColorHSV(rainbowHue + i * 65536 / strip.numPixels())
    ));
  }
  strip.show();
  rainbowHue += 256;
}

#endif