#include "FastLED.h"

// Just the BPM function from the demoReel sketch by Mark Kriegsman, December 2014

#define DATA_PIN    2
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    30
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  120

//For the potentiometer
#define POT_PIN  A1

int int_palletCount = 0; // currently active pallet

void setup() {
  
  Serial.begin(115200);
  Serial.println("Hello Arduino\n");
  delay(3000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}

void loop()
{
  pot_bpm();
}

void pot_bpm()
{
  
  uint8_t gHue = 0; // rotating "base color"
  // int int_palletCount = 0; // currently active pallet
  
  // int int_palletCount;
  int int_palletNum = 7; // the number of available pallets, starting at 0
  int int_palletDuration = 5; // how long a pallet will run before changing


  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  int int_bpm = 80;
  int_bpm = map(analogRead(POT_PIN), 0, 1023, 62, 120); 
  uint8_t BeatsPerMinute = int_bpm;
  CRGBPalette16 selectedPallet;
  
  /*
  Available preset color pallets:

    LavaColors_p              - orange, red, black and yellow
    CloudColors_p             - blue and white
    OceanColors_p             - blue, cyan and white
    ForestColors_p            - greens and blues
    RainbowColors_p           - standard rainbow animation
    RainbowStripeColors_p     - single colour, black space, next colour and so forth
    PartyColors_p             - red, yellow, orange, purple and blue
    HeatColors_p              - red, orange, yellow and white

  */
  
  int_palletDuration = map(analogRead(POT_PIN), 0, 1023, 10, 2); 
  EVERY_N_SECONDS(int_palletDuration){
    switch (int_palletCount){
      case 0:
        selectedPallet = LavaColors_p;
        Serial.println("Selected Pallet: LavaColors");
        break;
      case 1:
        selectedPallet = CloudColors_p;
        Serial.println("Selected Pallet: CloudColors");
        break;           
      case 2:
        selectedPallet = OceanColors_p;
        Serial.println("Selected Pallet: OceanColors");
        break;
      case 3:
        selectedPallet = ForestColors_p;
        Serial.println("Selected Pallet: ForestColors");
        break;
      case 4:
        selectedPallet = RainbowColors_p;
        Serial.println("Selected Pallet: RainbowColors");
        break;
      case 5:
        selectedPallet = RainbowStripeColors_p;
        Serial.println("Selected Pallet: RainbowStripeColors");
        break;
      case 6:
        selectedPallet = PartyColors_p;
        Serial.println("Selected Pallet: PartyColors");
        break;  
      case 7:
        selectedPallet = HeatColors_p;
        Serial.println("Selected Pallet: HeatColors");
        break;
    }

    int_palletCount++;
    // Serial.print("Pallet numb: ");
    // Serial.println(int_palletNum);
    if (int_palletCount > int_palletNum){
      int_palletCount = 0;
    }
  }

  // CRGBPalette16 palette = HeatColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(selectedPallet, gHue+(i*2), beat-gHue+(i*10));
  }
  
  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 
  
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
}
