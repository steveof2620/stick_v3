/*
Hardware:
  Circuit Playground Express Bluefruit
    (uses internal accelerometer, microphone, buttons A & B, slide switch and onboard neoPixels)
  1 x Potentiometer 
  1 x 30 LED WS2812B strip
*/

// libraries used
#include "FastLED.h"
#include <Adafruit_CircuitPlayground.h>

//====================================================================================================================
// Global Definitions
//====================================================================================================================

// Buttons A & B on the CPX
#define BUTTON_A 4
#define BUTTON_B 5

// The number of menu items to cycle through (starting as 0)
#define NUM_MENU_ITEMS 6

// The total number of LEDs being used
#define NUM_LEDS 30

// NeoPixel strip connected to A3 (Seems to only work when set to 17)
#define LED_PIN 17

// Definitations for the VU_meter() function
#define SAMPLE_WINDOW   10  // Sample window for average level
#define PEAK_HANG       24  // Time of pause before peak dot falls
#define PEAK_FALL        4  // Rate of falling peak dot 

// Definitations for the fire_effect() function
#define FRAMES_PER_SECOND 120
#define COOLING  55        // COOLING: How much does the air cool as it rises?
                           //Less cooling = taller flames.  More cooling = shorter flames.
                           // Default 55, suggested range 20-100 
#define SPARKING 120       // SPARKING: What chance (out of 255) is there that a new spark will be lit?
                           // Higher chance = more roaring fire.  Lower chance = more flickery fire.
                           // Default 120, suggested range 50-200.

// For the potentiometer
#define POT_PIN  A1        //Otherwise known as pin 6

// Initialise the LED array.
CRGB leds[NUM_LEDS];

//====================================================================================================================
// Global Variables
//====================================================================================================================

// byte intensity = 150;       // intensity: default brightness
// changing intensity to brightness to better reflect its purpose
uint8_t brightness = 120;


float X, Y, Z;              // For the adafruit accelerometer

// variables for the selection function:
int mode = 0;               // mode: used to differentiate and select one out of the five effects
bool slideSwitch;           // used to switch between the 'button modes' and the music mode

// variables for the chaser() function:
int onePos = 2;
int twoPos = 2;
boolean oneDir = true;
boolean twoDir = true;

// variables for the rainbow_display() function
int rainbow_timer = 0;

// variables for the VU_meter() function
byte peak = 16;           // Peak level of column; used for falling dots
unsigned int sample;
byte dotCount = 0;        //Frame counter for peak dot
byte dotHangCount = 0;    //Frame counter for holding peak dot

// variables for the fire_effect() function
bool gReverseDirection = false;
CRGBPalette16 gPal;

// used by adjustSpeed (called via commetEffect & firestarter)
int LEDAccel=0;             // stores the acceleration value of the LED animation sequence (speed up or slow down)

// used by setDelay (called via commetEffect & firestarter) 
int maxLEDSpeed = 50;       // maxLEDSpeed: identifies the maximum speed of the LED animation sequence
int animationDelay = 0;     // animationDelay: is used in the animation Speed calculation. The greater the 
                            // animationDelay, the slower the LED sequence.

// used by commetEffect & fireStarter
int LEDSpeed=1;                   // stores the "speed" of the LED animation sequence
int LEDPosition=int(NUM_LEDS/2);  // identifies the LED within the strip to modify (leading LED). 
                                  // The number will be between 0 & NUM_LEDS-1
byte bright = 80;                 // used to modify the brightness of the trailing LEDs

// used by fireStarter
byte ledb[NUM_LEDS];
byte ledh[NUM_LEDS];

//used by sparkle (called via firestarter)
int sparkTest = 0;               // sparkTest: variable used in the "sparkle" LED animation sequence

// used by pot_bpm
int int_palletCount = 0; // currently active pallet

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
    //used by the fire_effect() function, sets a (bulit in) palette for the function to use
    gPal = HeatColors_p;
    boot_sequence();
}

//====================================================================================================================
// loop() : The main loop
//====================================================================================================================
void loop(){

  slideSwitch = CircuitPlayground.slideSwitch();
   
  // using the slide switch to run the VU_meter function(s) cause it sets 
  // the mode variable to zero for reasons unknown
  if (slideSwitch){
    // Set the indicator to zero because VU_meter sets mode to zero
    // for reasons unknown. Also indicates we're not in the select mode mode.
    CircuitPlayground.clearPixels();
    VU_meter();
  }
  else {
    mode = selectMode();  
    Serial.println(mode);
    switch(mode){
      case 0:                                               
        // Serial.println("Mode one selected");
        demoReel();
        break;

      case 1:                                               
        // Serial.println("Mode two selected");
        rainbow_display();
        break;

      case 2:                                    
        // Serial.println("Mode three, calling BPM");
        pot_bpm();
        break;      
      
      case 3:
        // Serial.println("Mode four selected");
        fire_effect();
        break;
      
      case 4:
        // Serial.println("Mode five selected");
        chaser();
        break;

      case 5:                                               
        // Serial.println("Mode six selected");
        comet_effect();
        break;

      case 6:
        // Serial.println("Mode seven selected");
        fire_starter();
        break;
    }
  }
}

// used for the demoReel
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

//====================================================================================================================
// selectMode() : Read button states. This value will be used to choose the animation sequence to display and
//                update the CPX leds accordingly.
//====================================================================================================================
int selectMode (){
  // left button A increases, right button B decreases
  
  // Indicate the current mode
  CircuitPlayground.setPixelColor(mode, 0, 255, 0);

  // Button A, going forward (or up)
    if (digitalRead(BUTTON_A)){
    mode++;
    CircuitPlayground.setPixelColor(mode, 0, 255, 0);
    
    /*    
    Serial.print("led ");
    Serial.print(mode);
    Serial.println(" activated."); 
    */

    if (mode > NUM_MENU_ITEMS){
      mode = 0;
      CircuitPlayground.clearPixels();
      CircuitPlayground.setPixelColor(mode, 0, 255, 0);
    }
    delay(500); //delay here, otherwise it slows everything.
  }

  // Button B, going backward (or down)
  if (digitalRead(BUTTON_B)){
    mode--;
    CircuitPlayground.setPixelColor(mode+1, 0, 0, 0);
    if (mode < 1){
      mode = 0;
      CircuitPlayground.clearPixels();
      CircuitPlayground.setPixelColor(mode, 0, 255, 0);
    }
    delay(500);
  }
  return mode;
}

//====================================================================================================================
// boot_sequence() : Called from void setup() to run only once. Displays a sort of flickering effect to look like
//                   its coming to life.
//====================================================================================================================
void boot_sequence(){
  lightning();
  flicker();
  
  // Setting the first pixel to indicate mode one is running
  // zero is the first LED
  CircuitPlayground.setPixelColor(0, 0, 255, 0);
}

//====================================================================================================================
// lightning() : Called from void boot_sequence(). Displays a sort of lightning effect.
//====================================================================================================================
void lightning(){
  int FLASHES = 50;       //the number of flashes
  int FREQUENCY = 50;     //delay between flashes
  int dimmer;
  int flash_length = random(3,5);
  
for (int flashCounter = 0; flashCounter < random8(int(FLASHES/3),FLASHES); flashCounter++)
  {
    if(flashCounter == 0) dimmer = 5;     // the brightness of the leader is scaled down by a factor of 5
    else dimmer = random8(1,3);           // return strokes are brighter than the leader
    
    //fill_solid(leds,NUM_LEDS,CHSV(255, 0, 255/dimmer));    
    fill_solid(leds+random(0,NUM_LEDS-flash_length), flash_length, CHSV(255, 0, 255/dimmer));
    
    FastLED.show();
    delay(random8(4,10));                 // each flash only lasts 4-10 milliseconds
    
    fill_solid(leds,NUM_LEDS,CHSV(0, 0, 0));
    FastLED.show();
    
    if (flashCounter == 0) delay (150);   // longer delay until next flash after the leader
    delay(50+random8(100));               // shorter delay between strokes  
  }
  //Not needed >> delay(random8(FREQUENCY)*100);          // delay between strikes  
}

//====================================================================================================================
// flicker() : Called from void boot_sequence(). Displays a sort of flickering effect to mimic a sort of flouro tube
//             coming to life, before displaying a rainbow.
//====================================================================================================================
void flicker() {
   // a flicker effect, 1000 - about 3 seconds
   
   //flickers from each end
   for (int i = 0; i < 80; i++) {
    if (random(2) == 1) {
      //set the colors here
      fill_solid(leds,int(random(NUM_LEDS/2)), CRGB::White);
      fill_solid(leds+int(NUM_LEDS-(NUM_LEDS/2)),int(random(NUM_LEDS/2)), CRGB::White);
      FastLED.show();         
    } else {
      FastLED.clear();
      FastLED.show();
    }
    delay(random(60));
  }

  //flickers the whole strip
  for (int i = 0; i < 40; i++) {
    if (random(2) == 1) {
      //set the colors here
      fill_solid(leds, NUM_LEDS, CRGB::White);
      FastLED.show();
    } else {
      FastLED.clear();
      FastLED.show();
    }
    delay(random(60));
  }
  fill_rainbow(leds, NUM_LEDS, 0, 255/NUM_LEDS);
  FastLED.show(); 
}

//====================================================================================================================
// demoReel () : Runs through a sequence of rainbow, rainbowWithGlitter, confetti, sinelon, 
//               juggle, bpm.
//====================================================================================================================
typedef void (*SimplePatternList[])(); // List of patterns to cycle through.  Each is defined as a separate function below
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void demoReel()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // Set the brightness of the LEDs
  brightness = map(analogRead(POT_PIN), 0, 1023, 0, 200); 
  FastLED.setBrightness(brightness);

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 10 ) { nextPattern(); } // change patterns periodically    
}

//===================================================================================================================
// nextPattern() : called by demoreel, add one to the current pattern number, and wrap around at the end 
//===================================================================================================================
void nextPattern()
{
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

//===================================================================================================================
// rainbow() : called by demoreel, FastLED's built-in rainbow generator
//===================================================================================================================
void rainbow() 
{
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

//===================================================================================================================
// rainbowWithGlitter() : called by demoreel, / built-in FastLED rainbow, plus some random sparkly glitter
//===================================================================================================================
void rainbowWithGlitter() 
{
  rainbow();
  addGlitter(80);
}

//===================================================================================================================
// addGlitter() : called by demoreel
//===================================================================================================================
void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

//===================================================================================================================
// addGlitter() : called by demoreel, random colored speckles that blink in and fade smoothly
//===================================================================================================================
void confetti() 
{
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

//===================================================================================================================
// sinelon() : called by demoreel, a colored dot sweeping back and forth, with fading trails
//===================================================================================================================
void sinelon()
{
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

//===================================================================================================================
// bpm() : called by demoreel, colored stripes pulsing at a defined. Beats-Per-Minute (BPM).
//===================================================================================================================
void bpm()
{
  uint8_t BeatsPerMinute = 82;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

//===================================================================================================================
// juggle() : called by demoreel, eight colored dots, weaving in and out of sync with each other
//===================================================================================================================
void juggle() {
  fadeToBlackBy( leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

//====================================================================================================================
// rainbow_display () : This is where the rainbow sequence is controlled. It calls display_rainbow,
//                      clear_rainbow and rainbow cylon.
//====================================================================================================================
void rainbow_display(){
 
  int rainbowRunTime = 2; // time in seconds for the initial rainbow display to run
  int cylonRunTime = 10;
 
  // Increment a counter every second so that each sequence runs for a specified
  // number of seconds
  EVERY_N_SECONDS( 1 ) { rainbow_timer++; };
 
  // call the required sequence according to the time the routine has run.
  if (rainbow_timer <= rainbowRunTime) {
    display_rainbow();  
  }
  else if (rainbow_timer == rainbowRunTime +1) {
    clear_rainbow();
    rainbow_timer ++;  //run only once  
  }
  else if (rainbow_timer > rainbowRunTime) {
    rainbow_cylon();
    FastLED.show();
    // reset the timer so that it can run from the beginning again
    if (rainbow_timer > rainbowRunTime + cylonRunTime){
      rainbow_timer = 0;
    }
  }
}

//===================================================================================================================
// pot_bpm() : Called from the main routine. Similar to the one in demoreel but 
//             Beats-Per-Minute (BPM) set via the potentiometer.
//===================================================================================================================
void pot_bpm()
{
  
  uint8_t gHue = 0; // rotating "base color"
  
  // For the colour palette rotation.
  int int_palletNum = 7; // the number of available pallets, starting at 0
  int int_palletDuration = 5; // how long a pallet will run before changing


  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  int int_bpm = 80;
  
  // Obatin the bpm setting from the potentiometer
  int_bpm = map(analogRead(POT_PIN), 0, 1023, 62, 120); 
  uint8_t BeatsPerMinute = int_bpm;
  
  //assign the colour palette
  CRGBPalette16 selectedPallet;
  
  /*
  Available preset color pallets for selection:

    LavaColors_p              - orange, red, black and yellow
    CloudColors_p             - blue and white
    OceanColors_p             - blue, cyan and white
    ForestColors_p            - greens and blues
    RainbowColors_p           - standard rainbow animation
    RainbowStripeColors_p     - single colour, black space, next colour and so forth
    PartyColors_p             - red, yellow, orange, purple and blue
    HeatColors_p              - red, orange, yellow and white

  */
  
  // Obtain the pallet during from the potentiometer
  int_palletDuration = map(analogRead(POT_PIN), 0, 1023, 10, 2); 
  
  // Assign the selected colourPalette
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

    // Incriment the index. If it exceeds the number of assigned palettes
    // then reset the counter back to zero.
    int_palletCount++;
    // Serial.print("Pallet numb: ");
    // Serial.println(int_palletNum);
    if (int_palletCount > int_palletNum){
      int_palletCount = 0;
    }
  }

  // activate the palette 
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


//====================================================================================================================
// void display_rainbow () : The beginning of the sequence. A rainbow is displayed with a glitter effect and the the 
//                           rainbow_cylon running underneath.
//====================================================================================================================
void display_rainbow(){
  int glitter_led;
 
  FastLED.setBrightness (brightness/2);
  fill_rainbow(leds, NUM_LEDS, 0, 255/NUM_LEDS);
 
  if( random8() < NUM_LEDS/2) {
    glitter_led = random16(NUM_LEDS);
    leds[glitter_led].maximizeBrightness();
    leds[glitter_led] += CRGB::White;
   }
  rainbow_cylon();  
  FastLED.show();
}

//====================================================================================================================
// void clear_rainbow () : Clears the rainbow one led at time to give a swipe effect.
//====================================================================================================================
void clear_rainbow(){    
  for (int i = 0; i< NUM_LEDS; i++){
    leds[i] = CRGB::Black;
    delay(15); //rainbowRunTime * 5 ??
    FastLED.show();
  }
}

//====================================================================================================================
// void rainbow_cylon () : Displays one dot for each colour of the rainbow, in sort of chase effect.
//====================================================================================================================
void rainbow_cylon(){
 
  uint8_t fadeval = 100;            // Trail behind the LED's. Lower => faster fade.
  uint8_t bpm = 75;
 
  uint8_t inner = beatsin8(bpm, NUM_LEDS/4, NUM_LEDS/4*3);          // Move 1/4 to 3/4
  uint8_t outer = beatsin8(bpm, 0, NUM_LEDS-1);                     // Move entire length
  uint8_t middle = beatsin8(bpm, NUM_LEDS/3, NUM_LEDS/3*2);         // Move 1/3 to 2/3
  uint8_t lower_middle = beatsin8(bpm, NUM_LEDS/5, NUM_LEDS/5*4);   // Move 1/5 to 4/5
  uint8_t lower = beatsin8(bpm, NUM_LEDS/10, NUM_LEDS/10*9);        // Move 1/6 to 5/6
 
  leds[lower] = CRGB::Yellow;
  leds[inner] = CRGB::Red;
  leds[lower_middle] = CRGB::Green;
  leds[middle] = CRGB::Blue;
  leds[outer] = CRGB::Purple;
 
  nscale8(leds,NUM_LEDS,fadeval); // Fade the entire array. Or for just a few LED's, use  
}

//====================================================================================================================
// VU_Meter() : Displays a series of LEDS according to sound picked up by the Circuit Playground Express microphone.
//              Derived from the VU_Meter() contained within CPX examples.
//====================================================================================================================
void VU_meter(){

  //println(mode);
  
  int inputCeiling = 90;    // Upper range of mic sensitivity in db SPL -> original setting 110
                            // lower the setting the higher the range 
  int inputFloor = 70;      // Lower range of mic sensitivity in dB SPL -> original setting 56
                            // 62 seems the setting to cancel out background noise 
 
  float mapf(float x, float in_min, float in_max, float out_min, float out_max);
 
  inputCeiling = map(analogRead(POT_PIN), 0, 1023, 120, 80);
  
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

//===================================================================================================================
// fire_effect() : A flame effect. Derived from Fire2012WithPalette provided as an example within the FastLED libary
//===================================================================================================================
void fire_effect(){
  
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];
  
  random16_add_entropy( random());
  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      // Scale the heat value from 0-255 down to 0-240
      // for best results with color palettes.
      byte colorindex = scale8( heat[j], 240);
      CRGB color = ColorFromPalette( gPal, colorindex);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }

  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

//===================================================================================================================
// chaser() : Two sets of LEDs chasing one another. Colours randomly selected from a list of colours of the rainbow, 
//            pace set by potentiometer.
//            Derived from 'dotBeat' By John Burrougs and modified by Andrew Tuline.
//===================================================================================================================
void chaser(){
  
  int delay_speed = map(analogRead(POT_PIN), 0, 1023, 50, 10);
  int random_colour;

  //a list of colours from the rainbow, one is randomly selected
  CRGB colour_choices[5] = {CHSV(HUE_RED, 255, 255), CHSV(HUE_YELLOW, 255, 255), CHSV(HUE_GREEN, 255, 255), 
                            CHSV(HUE_BLUE, 255, 255), CHSV(HUE_PURPLE, 255, 255)};
  
  //select a colour from the list
  random_colour = random(0,5);
  
  // turn on the LEDs to draw the current position of each line along the strip  
  //leds[onePos] = CRGB::Green;  
  leds[onePos] = colour_choices[random_colour];
  leds[max(onePos - 1, 0)] = leds[onePos];
  leds[min(onePos + 1, NUM_LEDS - 1)] = leds[onePos];

  //leds[twoPos] = CRGB::Blue;
  if (random_colour < 4){
    leds[twoPos] = colour_choices[random_colour+1];
  }
  else{
     leds[twoPos] = colour_choices[0];   
  } 
  leds[max(twoPos - 1, 0)] = leds[twoPos];
  leds[min(twoPos + 1, NUM_LEDS - 1)] = leds[twoPos];
  
  FastLED.show();
  
  delay(delay_speed);

  // turn all LEDs off so they can be re-drawn on the next loop in the next calculated position
  leds[onePos] = CRGB::Black;
  leds[max(onePos - 1, 0)] = leds[onePos];
  leds[min(onePos + 1, NUM_LEDS - 1)] = leds[onePos];

  leds[twoPos] = CRGB::Black;
  leds[max(twoPos - 1, 0)] = leds[twoPos];
  leds[min(twoPos + 1, NUM_LEDS - 1)] = leds[twoPos];

  FastLED.show();

  // calculate the next position of each LED line by advancing the LED position in the forward or reverse 
  // direction as required. If the line reaches the end of the LED strip, it is time to change its 
  // direction. 
  if (oneDir) {     // if going forward
    onePos += 4;
    if (onePos >= NUM_LEDS) {
      onePos = NUM_LEDS - 1;
      oneDir = false;  // go in reverse direction
    }
  }
  // else {   //if going reverse
  if (!oneDir) {
    onePos -= 4;
    if (onePos <= 0) {
      onePos = 0;
      oneDir = true;  // go in forward direction
    }
  }

  if (twoDir) {     // if going forward
    twoPos += 3;
    if (twoPos >= NUM_LEDS) {
      twoPos = NUM_LEDS - 1;
      twoDir = false;  // go in reverse direction
    }
  }
  //else {   //if going reverse
  if (!twoDir) {
    twoPos -= 3;
    if (twoPos <= 0) {
      twoPos = 0;
      twoDir = true;  // go in forward direction
    }
  }
}

//===================================================================================================================
// cometEffect() :  random brightness of the trailing LEDs produces an interesting comet-like effect. Colour via
//                  potentiometer, speed set via accelerometer.
//                  Derived from NeoPixel Playground project by Scott C
//                  https://arduinobasics.blogspot.com/2015/07/neopixel-playground.html
//===================================================================================================================
void comet_effect(){

      adjustSpeed();
      constrainLEDs();
      
      byte potVal = map(analogRead(POT_PIN), 0, 1023, 0, 255);

      //Serial.println(potVal);
      
      showLED(LEDPosition, potVal, 255, brightness);       // Hue set via potentiometer.

      //The following lines create the comet effect
      bright = random(50, 100);                           // Randomly select a brightness between 50 and 100
      leds[LEDPosition] = CHSV((potVal+40),255, bright);  // The trailing LEDs will have a different hue to the 
      fadeLEDs(8);                                        // leading LED, and will have a random brightness. 
      setDelay(LEDSpeed);                                 // This will affect the length of the Trailing LEDs. 
}                                                         // The LEDSpeed will be affected by the slope of the 
                                                          // Accelerometer's X-Axis

//===================================================================================================================
// fireStarter() : using the accelerometer. Starts off looking like a ball of fire, leaving a trail of little fires. 
//                 As the potentiometer is turned, it becomes more like a shooting star with a rainbow-sparkle trail.
//                 Derived from NeoPixel Playground project by Scott'
//                 C https://arduinobasics.blogspot.com/2015/07/neopixel-playground.html
//===================================================================================================================
void fire_starter(){

      adjustSpeed();
      constrainLEDs();
      
      byte potVal = map(analogRead(POT_PIN), 0, 1023, 0, 255);
      
      ledh[LEDPosition] = potVal;                      // Hue set by potentiometer
      showLED(LEDPosition, ledh[LEDPosition], 255, brightness);

      //The following lines create the fire starter effect
      bright = random(50, 100);                       // Randomly select a brightness between 50 and 100
      ledb[LEDPosition] = bright;                     // Assign this random brightness value to the trailing LEDs
      sparkle(potVal/5);                              // Call the sparkle routine to create that sparkling effect. 
                                                      // The potentiometer controls the difference in hue from
      fadeLEDs(1);                                    //  LED to LED. A low number creates a longer tail
      setDelay(LEDSpeed);                             // The LEDSpeed will be affected by the slope of the
}                                                     //  Accelerometer's X Axis                                

//===================================================================================================================
// adjustSpeed() : called by commetEffect & fireStarter. Uses the X  axis value of the accelerometer to adjust speed
//                 and direction of the LED animation sequence
//===================================================================================================================
void adjustSpeed(){
  // Take a reading from the Y Pin of the accelerometer and adjust the value so that positive numbers move in one
  // direction, and negative numbers move in the opposite diraction. map function used to convert accelerometer
  // readings, constrain function to ensure it stays within the desired limits
  // values of -2.5 and -7 determined by trial and error.
 
  // added circuit playground Y
  X = CircuitPlayground.motionX();
  LEDAccel = constrain(map(X, -2.5, -7 , maxLEDSpeed, -maxLEDSpeed),-maxLEDSpeed, maxLEDSpeed);

  //delay(500);
  //Serial.println(X);
  
  // The Speed of the LED animation sequence can increase (accelerate), decrease (decelerate)
  LEDSpeed = LEDSpeed + LEDAccel;

  //The following lines of code are used to control the direction of the LED animation sequence, and limit 
  //the speed of that animation.
  if (LEDSpeed>0){
    LEDPosition++;                         // Illuminate the LED in the Next position
    if (LEDSpeed>maxLEDSpeed){
      LEDSpeed=maxLEDSpeed;                // Ensure that the speed does not go beyond the maximum 
    }                                      // speed in the positive direction              
  }

  if (LEDSpeed<0){
    LEDPosition--;                         // Illuminate the LED in the Prior position
    if (LEDSpeed<-maxLEDSpeed){
      LEDSpeed = -maxLEDSpeed;             // Ensure that the speed does not go beyond the maximum speed
    }                                      // in the negative direction            
  }
}

//===================================================================================================================
// constrainLEDs() : called by commetEffect & fireStarter. Ensures that the LED animation sequence remains within
//                   boundaries of the various arrays (and the LED strip) and it also creates a "bouncing" effect
//                   at both ends of the LED strip.
//===================================================================================================================
void constrainLEDs(){
  LEDPosition = constrain(LEDPosition, 0, NUM_LEDS-1);    // Make sure that the LEDs stay within the boundaries of
  if(LEDPosition == 0 || LEDPosition == NUM_LEDS-1) {     //  the LED strip
    LEDSpeed = (LEDSpeed * -0.9);                         // Reverse the direction of movement when LED gets to 
  }                                                       // end of strip.Creating a bouncing ball effect.
}

//===================================================================================================================
// fadeLEDs(fadeVal):  called by commetEffect & fireStarter, used to fade the LEDs back to black (OFF)
//===================================================================================================================
void fadeLEDs(int fadeVal){
  for (int i = 0; i<NUM_LEDS; i++){
    leds[i].fadeToBlackBy( fadeVal );
  }
}

//===================================================================================================================
// showLED(...) : called by commetEffect & fireStarter, used to fire the LEDs
//===================================================================================================================
void showLED(int pos, byte LEDhue, byte LEDsat, byte LEDbright){
  leds[pos] = CHSV(LEDhue,LEDsat,LEDbright);
  FastLED.show();
}

//===================================================================================================================
// setDelay(LSpeed) : called by commetEffect & fireStarter.
//===================================================================================================================
void setDelay(int LSpeed){
  animationDelay = maxLEDSpeed - abs(LSpeed);
  delay(animationDelay);
}

//===================================================================================================================
// sparkle(hDiff) : called by fireStarter to create a sparkling/fire-like effect each LED. Hue and brightness is 
//                  monitored and modified using arrays (ledh & ledb)
//===================================================================================================================
void sparkle(byte hDiff){
  for(int i = 0; i < NUM_LEDS; i++) {
    ledh[i] = ledh[i] + hDiff;         // hDiff controls the extent to which the hue changes along the trailing LEDs

    //prevents "negative" brightness.
    if(ledb[i]<3){
      ledb[i]=0;
    }

    // The probability of "re-igniting" an LED will decrease as you move along the tail
    // Once the brightness reaches zero, it cannot be re-ignited unless the leading LED passes over it again.
    if(ledb[i]>0){
      ledb[i]=ledb[i]-2;
      sparkTest = random(0,bright);
      if(sparkTest>(bright-(ledb[i]/1.1))){
        ledb[i] = bright;
      } else {
        ledb[i] = ledb[i] / 2;
      }
    }
    leds[i] = CHSV(ledh[i],255,ledb[i]);
  }
}
