#include <Adafruit_CircuitPlayground.h>

#define SAMPLE_WINDOW   10  // Sample window for average level
#define PEAK_HANG       24  // Time of pause before peak dot falls
#define PEAK_FALL        4  // Rate of falling peak dot

#define INPUT_FLOOR     65  // Lower range of mic sensitivity in dB SPL, original = 56
#define INPUT_CEILING   90  // Upper range of mic sensitivity in db SPL, original = 110

#define LED_PIN A1
#define NUM_LEDS 30

//Adafruit_CircuitPlayground cpx;

// use Adafruit_CPlay_NeoPixel to create a separate external NeoPixel strip
Adafruit_CPlay_NeoPixel strip = Adafruit_CPlay_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

byte peak = 16;        // Peak level of column; used for falling dots
unsigned int sample;
byte dotCount = 0;     //Frame counter for peak dot
byte dotHangCount = 0; //Frame counter for holding peak dot

float mapf(float x, float in_min, float in_max, float out_min, float out_max);

void setup() {
  CircuitPlayground.begin();
  strip.setBrightness(50); // Adjust brightness if needed
  strip.begin();
}

void loop() {
  int numPixels = NUM_LEDS;
  float peakToPeak = 0;   // peak-to-peak level
  unsigned int c, y;

  // Get peak sound pressure level over the sample window
  peakToPeak = CircuitPlayground.mic.soundPressureLevel(SAMPLE_WINDOW);

  // Limit to the floor value
  peakToPeak = max(INPUT_FLOOR, peakToPeak);

  // Fill the strip with rainbow gradient
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, Wheel(map(i, 0, NUM_LEDS - 1, 30, 150)));
  }

  c = mapf(peakToPeak, INPUT_FLOOR, INPUT_CEILING, NUM_LEDS, 0);

  // Turn off pixels that are below volume threshold
  if (c < peak) {
    peak = c;        // Keep dot on top
    dotHangCount = 0;    // Make the dot hang before falling
  }
  if (c <= NUM_LEDS) { // Fill partial column with off pixels
    drawLine(NUM_LEDS, NUM_LEDS - c, strip.Color(0, 0, 0));
  }

  // Set the peak dot to match the rainbow gradient
  y = NUM_LEDS - peak;
  strip.setPixelColor(y - 1, Wheel(map(y, 0, NUM_LEDS - 1, 30, 150)));
  strip.show();

  // Frame-based peak dot animation
  if (dotHangCount > PEAK_HANG) { // Peak pause length
    if (++dotCount >= PEAK_FALL) { // Fall rate 
      peak++;
      dotCount = 0;
    }
  } else {
    dotHangCount++;
  }
}

// Used to draw a line between two points of a given color
void drawLine(uint8_t from, uint8_t to, uint32_t c) {
  uint8_t fromTemp;
  if (from > to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  for (int i = from; i <= to; i++) {
    strip.setPixelColor(i, c);
  }
}

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
