/*
Hardware:
  For Circuit Playground Express Bluefruit:
    (uses internal accelerometer, microphone, buttons A & B, slide switch and onboard neoPixels)
    1 x Potentiometer (Pin A1, Ground, 3.3v)
    1 x 30 LED WS2812B strip (Pin A3 (17), Ground, VOUT)

  For Circuit Playground Express:
    (uses internal accelerometer, microphone, buttons A & B, slide switch and onboard neoPixels)
    1 x Potentiometer (Pin A3, Ground, 3.3v)
    1 x 30 LED WS2812B strip (Pin A1, Ground, VOUT)
*/

// libraries used
#include <Adafruit_CircuitPlayground.h>
#include "FastLED.h"

//====================================================================================================================
// Global Definitions
//====================================================================================================================

// The total number of LEDs being used
#define NUM_LEDS 30

/* For CPX Bluefruit, NeoPixel strip connected to A3 (Seems to only work when set to 17)
   For CPX NeoPixel strip connected to A1
*/
#define LED_PIN A1

// Definitations for the VU_meter() function
#define SAMPLE_WINDOW   10  // Sample window for average level
#define PEAK_HANG       24  // Time of pause before peak dot falls
#define PEAK_FALL        4  // Rate of falling peak dot 

/* For CPX Bluefruit, potentiometer connected to A1 (PIN 6)
   For CPX, potentiometer connected to A3  
*/
#define POT_PIN  A3

bool slideSwitch;

// Initialise the LED array.
CRGB leds[NUM_LEDS];

//====================================================================================================================
// Global Variables
//====================================================================================================================

// byte intensity = 150;       // intensity: default brightness
// changing intensity to brightness to better reflect its purpose
uint8_t brightness = 120;

// variables for the VU_meter() function
byte peak = 16;           // Peak level of column; used for falling dots
unsigned int sample;
byte dotCount = 0;        //Frame counter for peak dot
byte dotHangCount = 0;    //Frame counter for holding peak dot


//====================================================================================================================
// setup() : Start up and housekeeping. Display the boot sequence animation
//====================================================================================================================
void setup(){
    
    Serial.begin(9600);
    CircuitPlayground.begin(); //for the built in accelerometer, onboard LEDs etc  
    delay(2000);               //Delay for two seconds to power the LEDS before starting the data signal
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);    //initialise the LED strip
    FastLED.setBrightness(brightness);
    FastLED.clear();      //So we know where we are at 
    FastLED.show();
}

//====================================================================================================================
// loop() : The main loop
//====================================================================================================================
void loop(){

  // slideSwitch = CircuitPlayground.slideSwitch();
  
  // Setting the slideSwitch variable to true for testing purpuses
  slideSwitch = true;
  
  // using the slide switch to run the VU_meter function(s) cause it sets 
  // the mode variable to zero for reasons unknown
  if (slideSwitch){
    // Set the indicator to zero because VU_meter sets mode to zero
    // for reasons unknown. Also indicates we're not in the select mode mode.
    CircuitPlayground.clearPixels();
    VU_meter();
  }
}

//====================================================================================================================
// VU_Meter() : Displays a series of LEDS according to sound picked up by the Circuit Playground Express microphone.
//              Derived from the VU_Meter() contained within CPX examples.
//====================================================================================================================
void VU_meter(){
 
  int inputCeiling = 120;    // Upper range of mic sensitivity in db SPL -> original setting 110
                            // lower the setting the higher the range 
  int inputFloor = 65;      // Lower range of mic sensitivity in dB SPL -> original setting 56
                            // 62 seems the setting to cancel out background noise 
 
  float mapf(float x, float in_min, float in_max, float out_min, float out_max);
 
  inputCeiling = map(analogRead(POT_PIN), 0, 1023, 70, 150);
  
  /*
  Serial.print("input ceiling: ");
  Serial.println(inputCeiling);
  */

  // Tried to set the inputFloor by InputCeiling, didn't seem to work 
  // inputFloor = inputCeiling - 15;
  
  float peakToPeak = 0;   // peak-to-peak level
  unsigned int c, y;

  //get peak sound pressure level over the sample window
  peakToPeak = CircuitPlayground.mic.soundPressureLevel(SAMPLE_WINDOW);

  //limit to the floor value
  peakToPeak = max(inputFloor, peakToPeak);
 
  //Fill the strip with rainbow gradient
  for (int i=0;i<=NUM_LEDS-1;i++){
    //colours set in the map function 
    leds[i] = CHSV(map(i,0,NUM_LEDS-1,96,254), 255, 255);
  }

  c = mapf(peakToPeak, inputFloor, inputCeiling, NUM_LEDS, 0);

  // Turn off pixels that are below volume threshold.
  if(c < peak) {
    peak = c;        // Keep dot on top
    dotHangCount = 0;    // make the dot hang before falling
  }
  if (c <= NUM_LEDS) { // Fill partial column with off pixels
    drawLine(NUM_LEDS, NUM_LEDS-c, 0);
  }

  // Set the peak dot to match the rainbow gradient
  y = NUM_LEDS - peak;
  leds[y-1] = CHSV(map(y,0,NUM_LEDS-1,96,254), 255, 255);
  FastLED.show();

  // Frame based peak dot animation
  if(dotHangCount > PEAK_HANG) { //Peak pause length
    if(++dotCount >= PEAK_FALL) { //Fall rate 
      peak++;
      dotCount = 0;
    }
  } 
  else {
    dotHangCount++; 
  }
}

//===================================================================================================================
// drawLine(...) : Used by VU_Meter() to draw a line between two points of a given color.
//===================================================================================================================
void drawLine(uint8_t from, uint8_t to, uint32_t c){
  uint8_t fromTemp;
  if (from > to) {
    fromTemp = from;
    from = to;
    to = fromTemp;
  }
  for(int i=from; i<=to; i++){
    leds[i] = CRGB::Black;
  }
}

//===================================================================================================================
// mapf(...) : Used by VU_Meter() to maths support of some kind.
//===================================================================================================================
float mapf(float x, float in_min, float in_max, float out_min, float out_max){
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


