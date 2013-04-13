/*
 * Stomatopod
 * by Scott Gilroy
 * https://github.com/sgilroy/Stomatopod
 *
 * Stomatopod is a system for controlling multiple wearable synchronized RGB LED strips
 * connected wirelessly and manipulated by force sensitive resistors (FSRs) and other
 * sensors and controls. The original system was designed for two shoe-worn nodes,
 * plus a backpack node. Each node is a combination of an Arduino board, addressable
 * RBG LED strip, a ZigBee, and some sensors/controls.
 *
 * This project takes its name from Stomatopods (also known as mantis shrimp) which
 * have the most complex and sensitive visual systems of any organism on earth.
 * The eyes of stomatopods have 16 different visual pigments (compared to just three
 * in humans) allowing them to see a variety of colors and nuances of light well beyond
 * that of other creatures.
 */

// The RGB LED strip code is based on the example code from Adafruit
// to control LPD8806-based RGB LED Modules in a strip; originally
// intended for the Adafruit Digital Programmable LED Belt Kit.

// REQUIRES TIMER1 LIBRARY: http://www.arduino.cc/playground/Code/Timer1
// Also requires a library for the LED strip being used, which can be either:
//   (1) Adafruit_NeoPixel library
//   (2) LPD8806 library

// From Limor "Ladyada" Fried:
// I'm generally not fond of canned animation patterns.  Wanting something
// more nuanced than the usual 8-bit beep-beep-boop-boop pixelly animation,
// this program smoothly cycles through a set of procedural animated effects
// and transitions -- it's like a Video Toaster for your waist!  Some of the
// coding techniques may be a bit obtuse (e.g. function arrays), so novice
// programmers may have an easier time starting out with the 'strandtest'
// program also included with the LPD8806 library.

#include <Adafruit_NeoPixel.h>
//#include <avr/pgmspace.h>
//#include "SPI.h"
//#include "LPD8806.h"
#include <TimerOne.h>
#include <EasyTransfer.h>
#include <ClickButtonFsr.h>
#include <Streaming.h>

const int fps = 30;

const int dataPin = 4;

// the FSR and 10K pulldown are connected to a0
const int frontFsrPin = A0;
const int backFsrPin = A1;

const bool DEBUG_PRINTS = false;

int brightnessLimiter = 0;

// Declare the number of pixels in strand; 32 = 32 pixels in a row.  The
// LED strips have 32 LEDs per meter, but you can extend or cut the strip.
//const int numPixels = 30; // backpack
const int numPixels = 42; // shoes
// 'const' makes subsequent array declarations possible, otherwise there
// would be a pile of malloc() calls later.

// Index (0 based) of the pixel at the front of the shoe. Used by some of the render effects.
int frontOffset = 0;

// Instantiate LED strips; arguments are the total number of pixels in strip,
// the data pin number and clock pin number:
//LPD8806 stripLeft = LPD8806(numPixels, dataLeftPin, clockLeftPin);
//LPD8806 stripRight = LPD8806(numPixels, dataRightPin, clockRightPin);
//LPD8806 stripLeft2 = LPD8806(numPixels, dataLeft2Pin, clockLeft2Pin);
//LPD8806 stripRight2 = LPD8806(numPixels, dataRight2Pin, clockRight2Pin);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, dataPin, NEO_GRB + NEO_KHZ800);

// Principle of operation: at any given time, the LEDs depict an image or
// animation effect (referred to as the "back" image throughout this code).
// Periodically, a transition to a new image or animation effect (referred
// to as the "front" image) occurs.  During this transition, a third buffer
// (the "alpha channel") determines how the front and back images are
// combined; it represents the opacity of the front image.  When the
// transition completes, the "front" then becomes the "back," a new front
// is chosen, and the process repeats.
byte imgData[2][numPixels * 3], // Data for 2 strips worth of imagery
     alphaMask[numPixels],      // Alpha channel for compositing images
     backImgIdx = 0,            // Index of 'back' image (always 0 or 1)
     fxIdx[3];                  // Effect # for back & front images + alpha
int  fxVars[3][50],             // Effect instance variables (explained later)
     tCounter   = -1,           // Countdown to next transition
     transitionTime;            // Duration (in frames) of current transition

const static byte SYNCHRONIZED_EFFECT_BUFFERS = 3;
const static byte SYNCHRONIZED_EFFECT_VARIABLES = 5;

const boolean startWithTransition = false;
const boolean debugRenderEffects = true; // if true, use Serial.print to debug

boolean autoTransition = true; // if true, transition to a different render effect after some time elapses

// function prototypes, leave these be :)
void renderEffectSolidFill(byte idx);
void renderEffectBluetoothLamp(byte idx);
void renderEffectRainbow(byte idx);
void renderEffectSineWaveChase(byte idx);
void renderEffectPointChase(byte idx);
void renderEffectNewtonsCradle(byte idx);
void renderEffectMonochromeChase(byte idx);
void renderEffectWavyFlag(byte idx);
void renderEffectThrob(byte idx);
void renderEffectDebug1(byte idx);
void renderEffectBlast(byte idx);
void renderEffectBounce(byte idx);
void renderEffectClickVisualization(byte idx);
void renderEffectPressureVis(byte idx);

void renderAlphaFade(void);
void renderAlphaWipe(void);
void renderAlphaDither(void);
void callback();
byte gamma(byte x);
long hsv2rgb(long h, byte s, byte v);
char fixSin(int angle);
char fixCos(int angle);
int getPointChaseAlpha(byte idx, long i, int halfPeriod);
long pickHue(long currentHue);

// List of image effect and alpha channel rendering functions; the code for
// each of these appears later in this file.  Just a few to start with...
// simply append new ones to the appropriate list here:
void (*renderEffect[])(byte) = {
  renderEffectClickVisualization,
  renderEffectMonochromeChase,
  renderEffectBlast,
  renderEffectSolidFill,
  renderEffectRainbow,
  renderEffectSineWaveChase,
  renderEffectPointChase,
  renderEffectNewtonsCradle,
  renderEffectWavyFlag,
  renderEffectThrob,
  renderEffectBounce,
  renderEffectPressureVis,

//  renderEffectDebug1
},
(*renderAlpha[])(void)  = {
  renderAlphaFade,
  renderAlphaWipe,
  renderAlphaDither
  };

/* ---------------------------------------------------------------------------------------------------
   FSR Variables
 
Connect one end of FSR to power, the other end to an analog input pin.
Then connect one end of a 10K resistor from the analog input pin to ground.
 
For more information see www.ladyada.net/learn/sensors/fsr.html */
 
const bool forceResistorInUse = true;
const bool debugFsrReading = true;

int frontFsrStepFraction = 0;
int backFsrStepFraction = 0;
const int fsrStepFractionMax = 60;
bool gammaRespondsToForce = false;

// optional filtering to smooth out values if readings are too unstable
#define numFsrReadings 1
#if numFsrReadings > 1
int fsrReadingIndex = 0;
int fsrReadings[numFsrReadings];
#endif

ClickButtonFsr frontButton = ClickButtonFsr(frontFsrPin, 300, 500);
int frontButtonClicks = 0;
ClickButtonFsr backButton = ClickButtonFsr(backFsrPin, 300, 500);
int backButtonClicks = 0;
int clickVisualization = 0;

byte colorRed = 0;
byte colorGreen = 0;
byte colorBlue = 0;
long bluetoothColor = 0;
long bluetoothColorHue; // hue 0-1535

const int maxHue = 1535;

// ------------------------------
// EasyTransfer

//create two objects
EasyTransfer inTransfer, outTransfer; 

struct EstablishMaster{
  //put your variable definitions here for the data you want to receive
  //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
  int message;
};

struct StripPatternDescription{
  byte message;
  byte backImgIdx;
  int tCounter;

  byte backImgFunctionIndex;
  byte frontImgFunctionIndex;
  byte alphaFunctionIndex;
  int transitionTime;
  int fxVars[SYNCHRONIZED_EFFECT_BUFFERS][SYNCHRONIZED_EFFECT_VARIABLES];
};

const static int START_SLAVE_MODE = 1;
const static int SYNCHRONIZE_PATTERN = 2;

//give a name to the group of data
StripPatternDescription rxdata;
StripPatternDescription txdata;

bool slaveMode = false;

boolean inCallback = false;

// ---------------------------------------------------------------------------
//
//                  SETUP
//
// ---------------------------------------------------------------------------

void setup() {
#if numFsrReadings > 1
  for (int i = 0; i < numFsrReadings; i++)
  {
    fsrReadings[i] = 0;
  }
#endif

  // for synchronizing between two Arduinos
  Serial1.begin(9600);
  
  // for serial monitor/debugging
  Serial.begin(9600);
  
  // Start up the LED strip.  Note that strip.show() is NOT called here --
  // the callback function will be invoked immediately when attached, and
  // the first thing the calback does is update the strip.
  strip.begin();

  // Initialize random number generator from a floating analog input.
  randomSeed(analogRead(0));
  memset(imgData, 0, sizeof(imgData)); // Clear image data
  fxVars[backImgIdx][0] = 1;           // Mark back image as initialized

  if (startWithTransition) {
    tCounter = -1;
  } else {
    tCounter = 0;
  }

  // Timer1 is used so the strip will update at a known fixed frame rate.
  // Each effect rendering function varies in processing complexity, so
  // the timer allows smooth transitions between effects (otherwise the
  // effects and transitions would jump around in speed...not attractive).
  Timer1.initialize();
  
  Timer1.attachInterrupt(callback, 1000000 / fps);
//  Timer1.attachInterrupt(callback, 1000000 * 2); // 1 frame / 2 seconds

  //start the library, pass in the data details and the name of the serial port. Can be Serial, Serial1, Serial2, etc.
  inTransfer.begin(details(rxdata), &Serial1);
  outTransfer.begin(details(txdata), &Serial1);
}

void loop() {
  // All rendering happens in the callback() function below.

//  meetAndroid.receive(); // you need to keep this in your loop() to receive events
  
  frontButton.Update();
  backButton.Update();

  // Save click codes in frontButtonClicks, as click codes are reset at next Update()
  if (frontButton.click != 0) frontButtonClicks = frontButton.click;
  if (backButton.click != 0) backButtonClicks = backButton.click;
}


// ---------------------------------------------------------------------------
//
//                  FSR
//
// ---------------------------------------------------------------------------

int readForce(int fsrPin) {
  int fsrStepFraction = 0;
  int fsrReading;     // the analog reading from the FSR resistor divider
  int fsrVoltage;     // the analog reading converted to voltage
  unsigned long fsrResistance;  // The voltage converted to resistance, can be very big so make "long"
  unsigned long fsrConductance; 
  long fsrForce;       // Finally, the resistance converted to force
  
  fsrReading = analogRead(fsrPin);  
  if (debugFsrReading)
  {
    Serial.print("Analog reading = ");
    Serial.println(fsrReading);
  }
  
  #if numFsrReadings > 1
  {
    long fsrReadingSum = 0;
    long fsrReadingAvg;
    fsrReadings[fsrReadingIndex] = fsrReading;
    fsrReadingIndex = (fsrReadingIndex + 1) % numFsrReadings;
    for (int i = 0; i < numFsrReadings; i++)
    {
      fsrReadingSum += fsrReadings[i];
    }
    fsrReadingAvg = fsrReadingSum / numFsrReadings;
    fsrReading = (int)fsrReadingAvg;
  
    if (debugFsrReading)
    {
      Serial.print("Average reading = ");
      Serial.println(fsrReading);
    }
  }
  #endif
 
  // analog voltage reading ranges from about 0 to 1023 which maps to 0V to 5V (= 5000mV)
  fsrVoltage = map(fsrReading, 0, 1023, 0, 5000);
  if (debugFsrReading)
  {
    Serial.print("Voltage reading in mV = ");
    Serial.println(fsrVoltage);  
  }
 
  if (fsrVoltage == 0) {
    if (debugFsrReading)
    {
      Serial.println("No pressure");  
    }
  } else {
    // The voltage = Vcc * R / (R + FSR) where R = 10K and Vcc = 5V
    // so FSR = ((Vcc - V) * R) / V        yay math!
    fsrResistance = 5000 - fsrVoltage;     // fsrVoltage is in millivolts so 5V = 5000mV
    fsrResistance *= 10000;                // 10K resistor
    fsrResistance /= fsrVoltage;
    if (debugFsrReading)
    {
      Serial.print("FSR resistance in ohms = ");
      Serial.println(fsrResistance);
    }
 
    fsrConductance = 1000000;           // we measure in micromhos so 
    fsrConductance /= fsrResistance;
    if (debugFsrReading)
    {
      Serial.print("Conductance in microMhos: ");
      Serial.println(fsrConductance);
    }
 
    // Use the two FSR guide graphs to approximate the force
//    if (fsrConductance <= 1000) {
//      fsrForce = fsrConductance / 80;
      fsrForce = fsrConductance / 20;
//    } else {
//      fsrForce = fsrConductance - 1000;
//      fsrForce /= 30;
//    }
    fsrStepFraction = fsrForce > fsrStepFractionMax ? fsrStepFractionMax : fsrForce;
    if (debugFsrReading)
    {
      Serial.print("Force in Newtons: ");
      Serial.println(fsrForce);      
      Serial.print("Step Fraction: ");
      Serial.println(fsrStepFraction);      
  //    Serial.print("Step Fraction: ");
  //    Serial.println(fsrStepFraction);      
      Serial << "ClickButtonFsr value: " << frontButton.fsrValue << endl;
      Serial << "ClickButtonFsr btnFlick: " << frontButton.btnFlick << endl;
      Serial << "ClickButtonFsr depressed: " << frontButton.depressed << endl;
    }
  }
  if (debugFsrReading)
  {
    Serial.println("--------------------");
  }
  
  return fsrStepFraction;
}

long pickHue(long currentHue)
{
  return (bluetoothColor == 0 ? currentHue : bluetoothColorHue);
}

// Timer1 interrupt handler.  Called at equal intervals; 60 Hz by default.
void callback() {
  // don't do anything if we have not yet finished with the previous callback
  if (inCallback) return;
  
  inCallback = true;
  
  // Very first thing here is to issue the strip data generated from the
  // *previous* callback.  It's done this way on purpose because show() is
  // roughly constant-time, so the refresh will always occur on a uniform
  // beat with respect to the Timer1 interrupt.  The various effects
  // rendering and compositing code is not constant-time, and that
  // unevenness would be apparent if show() were called at the end.
  strip.show(); // Initialize all pixels to 'off'
  
  handleClicks();

  byte frontImgIdx = 1 - backImgIdx,
       *backPtr    = &imgData[backImgIdx][0],
       r, g, b;
  int  i;

  // Always render back image based on current effect index:
  (*renderEffect[fxIdx[backImgIdx]])(backImgIdx);

  // Front render and composite only happen during transitions...
  if(tCounter > 0) {
    // Transition in progress
    byte *frontPtr = &imgData[frontImgIdx][0];
    int  alpha, inv;

    // Render front image and alpha mask based on current effect indices...
    (*renderEffect[fxIdx[frontImgIdx]])(frontImgIdx);
    (*renderAlpha[fxIdx[2]])();

    // ...then composite front over back:
    for(i=0; i<numPixels; i++) {
      alpha = alphaMask[i] + 1; // 1-256 (allows shift rather than divide)
      inv   = 257 - alpha;      // 1-256 (ditto)
      // r, g, b are placed in variables (rather than directly in the
      // setPixelColor parameter list) because of the postincrement pointer
      // operations -- C/C++ leaves parameter evaluation order up to the
      // implementation; left-to-right order isn't guaranteed.
      r = gamma((*frontPtr++ * alpha + *backPtr++ * inv) >> 8);
      g = gamma((*frontPtr++ * alpha + *backPtr++ * inv) >> 8);
      b = gamma((*frontPtr++ * alpha + *backPtr++ * inv) >> 8);
      strip.setPixelColor(i, r, g, b);
    }
  } else {
    // No transition in progress; just show back image
    for(i=0; i<numPixels; i++) {
      // See note above re: r, g, b vars.
      r = gamma(*backPtr++);
      g = gamma(*backPtr++);
      b = gamma(*backPtr++);
      strip.setPixelColor(i, r, g, b);
    }
  }

  if (!slaveMode)
  {
    // Count up to next transition (or end of current one):
    if (autoTransition || tCounter >= 0)
    {
      tCounter++;
      if(tCounter == 0) { // Transition start
        startImageTransition(frontImgIdx);
      } else if(tCounter >= transitionTime) { // End transition
        endImageTransition(frontImgIdx);
      }
    }
  }
  

  if (DEBUG_PRINTS)
  {
    Serial.print("callback complete ");
    Serial.print("dataPin ");
    Serial.print(dataPin);
    Serial.println();
  }
  frontFsrStepFraction = readForce(frontFsrPin);
  backFsrStepFraction = readForce(backFsrPin);
//  meetAndroid.receive(); // you need to keep this in your loop() to receive events

  serialSynchronize(frontImgIdx);

  inCallback = false;
}

void handleClicks()
{
  if (frontButtonClicks != 0)
  {
    if (frontButtonClicks == 2 && clickVisualization < numPixels)
    {
      clickVisualization++;
    }
    else if (frontButtonClicks == -1 && clickVisualization > 0)
    {
      // TODO: make the long press work again (not sure what broke this, but this does not seem to execute ever
      clickVisualization--;
    }
    
    frontButtonClicks = 0;
  }

  if (backButtonClicks != 0)
  {
    byte frontImgIdx = 1 - backImgIdx;
    if (backButtonClicks == 2)
    {
      tCounter = 0;
      startImageTransition(frontImgIdx, (fxIdx[frontImgIdx] + 1) % (sizeof(renderEffect) / sizeof(renderEffect[0])), fps / 2);
      autoTransition = false;
    }
    else if (backButtonClicks == 3)
    {
      tCounter = -1;
      autoTransition = true;
    }
    
    backButtonClicks = 0;
  }
}

void startImageTransition(byte frontImgIdx, byte effectFunctionIndex, int inTransitionTime)
{
    fxIdx[frontImgIdx] = effectFunctionIndex;
    fxIdx[2]           = random((sizeof(renderAlpha)  / sizeof(renderAlpha[0])));
    transitionTime     = inTransitionTime;
    fxVars[frontImgIdx][0] = 0; // Effect not yet initialized
    fxVars[2][0]           = 0; // Transition not yet initialized
}

void startImageTransition(byte frontImgIdx)
{
    // Randomly pick next image effect and alpha effect indices:
    // 0.5 to 3 second transitions
    startImageTransition(frontImgIdx, random((sizeof(renderEffect) / sizeof(renderEffect[0]))), random(fps / 2, fps * 3));
}

void endImageTransition(byte frontImgIdx)
{
    fxIdx[backImgIdx] = fxIdx[frontImgIdx]; // Move front effect index to back
    backImgIdx        = 1 - backImgIdx;     // Invert back index
    tCounter          = -120 - random(240); // Hold image 2 to 6 seconds
//    tCounter          = -600; // Hold image 10 seconds
}  


// ---------------------------------------------------------------------------
// Image effect rendering functions.  Each effect is generated parametrically
// (that is, from a set of numbers, usually randomly seeded).  Because both
// back and front images may be rendering the same effect at the same time
// (but with different parameters), a distinct block of parameter memory is
// required for each image.  The 'fxVars' array is a two-dimensional array
// of integers, where the major axis is either 0 or 1 to represent the two
// images, while the minor axis holds 50 elements -- this is working scratch
// space for the effect code to preserve its "state."  The meaning of each
// element is generally unique to each rendering effect, but the first element
// is most often used as a flag indicating whether the effect parameters have
// been initialized yet.  When the back/front image indexes swap at the end of
// each transition, the corresponding set of fxVars, being keyed to the same
// indexes, are automatically carried with them.

// Simplest rendering effect: fill entire image with solid color
void renderEffectSolidFill(byte idx) {
  // Only needs to be rendered once, when effect is initialized:
  if(fxVars[idx][0] == 0) {
    gammaRespondsToForce = true;
    fxVars[idx][1] = random(256);
    fxVars[idx][2] = random(256);
    fxVars[idx][3] = random(256);
    fxVars[idx][0] = 1; // Effect initialized
  }
  
  byte *ptr = &imgData[idx][0],
    r = fxVars[idx][1], g = fxVars[idx][2], b = fxVars[idx][3];
  for(int i=0; i<numPixels; i++) {
    if (bluetoothColor == 0)
    {
      *ptr++ = r; *ptr++ = g; *ptr++ = b;
    }
    else
    {
      *ptr++ = bluetoothColor >> 16; *ptr++ = bluetoothColor >> 8; *ptr++ = bluetoothColor;
    }
  }
  
}

void renderEffectClickVisualization(byte idx) {
  // Only needs to be rendered once, when effect is initialized:
  if(fxVars[idx][0] == 0) {
    gammaRespondsToForce = false;
    fxVars[idx][1] = random(maxHue + 1); // Random hue
    fxVars[idx][0] = 1; // Effect initialized
  }
  
  long hue = pickHue(fxVars[idx][1]);
  long color = hsv2rgb(hue, 255, 255);

  byte *ptr = &imgData[idx][0],
    r = fxVars[idx][1], g = fxVars[idx][2], b = fxVars[idx][3];
  for(int i=0; i<numPixels; i++) {
    if (clickVisualization >= i)
    {
      *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
    }
    else
    {
      *ptr++ = 0; *ptr++ = 0; *ptr++ = 0;
    }
  }
  
}

void renderEffectDebug1(byte idx) {
  // Only needs to be rendered once, when effect is initialized:
  if(fxVars[idx][0] == 0) {
    gammaRespondsToForce = true;
    byte *ptr = &imgData[idx][0],
      r = 0, g = 0, b = 0;
    for(int i=0; i<numPixels; i++) {
      if (i % (numPixels / 4) == 0)
      {
        r = g = b = 255;
      }
      else
      {
        r = g = b = 0;
      }
        
      *ptr++ = r; *ptr++ = g; *ptr++ = b;
    }
    fxVars[idx][0] = 1; // Effect initialized
  }
}

// Rainbow effect (1 or more full loops of color wheel at 100% saturation).
// Not a big fan of this pattern (it's way overused with LED stuff), but it's
// practically part of the Geneva Convention by now.
void renderEffectRainbow(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    gammaRespondsToForce = true;
    // Number of repetitions (complete loops around color wheel); any
    // more than 4 per meter just looks too chaotic and un-rainbow-like.
    // Store as hue 'distance' around complete belt:
    fxVars[idx][1] = (1 + random(4 * ((numPixels + 31) / 32))) * (maxHue + 1);
    // Frame-to-frame hue increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.  It's generally
    // still less than a full pixel per frame, making motion very smooth.
    fxVars[idx][2] = 4 + random(fxVars[idx][1]) / numPixels;
    // Reverse speed and hue shift direction half the time.
    if(random(2) == 0) fxVars[idx][1] = -fxVars[idx][1];
    if(random(2) == 0) fxVars[idx][2] = -fxVars[idx][2];
    fxVars[idx][3] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
  }

  byte *ptr = &imgData[idx][0];
  long color, i;
  for(i=0; i<numPixels; i++) {
    color = hsv2rgb(fxVars[idx][3] + fxVars[idx][1] * i / numPixels,
      255, 255);
    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
  }
  fxVars[idx][3] += fxVars[idx][2];
}

// Sine wave chase effect
void renderEffectSineWaveChase(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    gammaRespondsToForce = true;
    fxVars[idx][1] = random(maxHue + 1); // Random hue
    // Number of repetitions (complete loops around color wheel);
    // any more than 4 per meter just looks too chaotic.
    // Store as distance around complete belt in half-degree units:
    fxVars[idx][2] = (1 + random(4 * ((numPixels + 31) / 32))) * 720;
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.  It's generally
    // still less than a full pixel per frame, making motion very smooth.
    fxVars[idx][3] = 4 + random(fxVars[idx][1]) / numPixels;
    // Reverse direction half the time.
    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
    fxVars[idx][4] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
  }

  byte *ptr = &imgData[idx][0];
  int  foo;
  long color, i;
  long hue = pickHue(fxVars[idx][1]);
  for(long i=0; i<numPixels; i++) {
    foo = fixSin(fxVars[idx][4] + fxVars[idx][2] * i / numPixels);
    // Peaks of sine wave are white, troughs are black, mid-range
    // values are pure hue (100% saturated).
    color = (foo >= 0) ?
       hsv2rgb(hue, 254 - (foo * 2), 255) :
       hsv2rgb(hue, 255, 254 + foo * 2);
    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
  }
  fxVars[idx][4] += fxVars[idx][3];
}

void renderEffectBlast(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    gammaRespondsToForce = false;
    fxVars[idx][1] = random(maxHue + 1); // Random hue
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.  It's generally
    // still less than a full pixel per frame, making motion very smooth.
//    fxVars[idx][3] = 1 + random(720) / numPixels;
//    fxVars[idx][3] = 1;
    fxVars[idx][3] = 4;
    // Reverse direction half the time.
    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
    fxVars[idx][4] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
//    fxVars[idx][5] = 15 + random(360); // wave period
//    fxVars[idx][5] = 30 + random(150); // wave period (width)
    fxVars[idx][5] = 720 * 4 / numPixels; // wave period (width)
//    fxVars[idx][5] = 720 * 8 / numPixels; // wave period (width)
//    fxVars[idx][5] = random(720 * 2 / numPixels, 180); // wave period (width)
  }

  byte *ptr = &imgData[idx][0];
  int alpha;
  int halfPeriod = fxVars[idx][5] / 2;
  int distance;
  long color;
  long hue = pickHue(fxVars[idx][1]);
  for(long i=0; i<numPixels; i++) {
    alpha = getPointChaseAlpha(idx, (i + frontOffset) % numPixels, halfPeriod) + getPointChaseAlpha(idx, (numPixels - 1 - i + frontOffset) % numPixels, halfPeriod);
//    alpha = getPointChaseAlpha(idx, (i + frontOffset) % numPixels, halfPeriod);
    if (alpha > 255) alpha = 255;
    
    // Peaks of sine wave are white, troughs are black, mid-range
    // values are pure hue (100% saturated).

    color = hsv2rgb(hue, 255, alpha);
    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
  }
//  fxVars[idx][4] += fxVars[idx][3];
//  fxVars[idx][4] %= 720;

  // max force = half way around (360 half degrees)
  if (!slaveMode)
    fxVars[idx][4] = map(frontFsrStepFraction, 0, fsrStepFractionMax, 0, 360);
//  fxVars[idx][5] = map(fsrStepFraction, 720 / numPixels, fsrStepFractionMax, 0, 720 * 2);
}

void renderEffectBluetoothLamp(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    gammaRespondsToForce = false;
    fxVars[idx][1] = random(maxHue + 1); // Random hue
    // Number of repetitions (complete loops around color wheel);
    // any more than 4 per meter just looks too chaotic.
    // Store as distance around complete belt in half-degree units:
//    fxVars[idx][2] = (1 + random(4 * ((numPixels + 31) / 32))) * 720;
    fxVars[idx][2] = 1 * 720;
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.  It's generally
    // still less than a full pixel per frame, making motion very smooth.
//    fxVars[idx][3] = 1 + random(720) / numPixels;
//    fxVars[idx][3] = 1;
    fxVars[idx][3] = 4;
    // Reverse direction half the time.
    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
    fxVars[idx][4] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
//    fxVars[idx][5] = 15 + random(360); // wave period
//    fxVars[idx][5] = 30 + random(150); // wave period (width)
    fxVars[idx][5] = 720 * 4 / numPixels; // wave period (width)
//    fxVars[idx][5] = random(720 * 2 / numPixels, 180); // wave period (width)
  }

  byte *ptr = &imgData[idx][0];
  int alpha;
  int halfPeriod = fxVars[idx][5] / 2;
  int distance;
  long color;
  for(long i=0; i<numPixels; i++) {
    alpha = getPointChaseAlpha(idx, (i + frontOffset + 1) % numPixels, halfPeriod) + getPointChaseAlpha(idx, (numPixels - 1 - i + (numPixels - frontOffset)) % numPixels, halfPeriod);
    if (alpha > 255) alpha = 255;
    
    // Peaks of sine wave are white, troughs are black, mid-range
    // values are pure hue (100% saturated).

//    color = hsv2rgb(fxVars[idx][1], 255, alpha);
//    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
    *ptr++ = colorRed; *ptr++ = colorGreen; *ptr++ = colorBlue;
  }
  fxVars[idx][4] = map(frontFsrStepFraction, 0, fsrStepFractionMax, 0, 720);
}

void renderEffectPointChase(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    gammaRespondsToForce = false;
    fxVars[idx][1] = random(maxHue + 1); // Random hue
    // Number of repetitions (complete loops around color wheel);
    // any more than 4 per meter just looks too chaotic.
    // Store as distance around complete belt in half-degree units:
//    fxVars[idx][2] = (1 + random(4 * ((numPixels + 31) / 32))) * 720;
    fxVars[idx][2] = 1 * 720;
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.  It's generally
    // still less than a full pixel per frame, making motion very smooth.
    fxVars[idx][3] = 1 + random(720) / numPixels;
//    fxVars[idx][3] = 1;
    // Reverse direction half the time.
    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
    fxVars[idx][4] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
//    fxVars[idx][5] = 15 + random(360); // wave period
    fxVars[idx][5] = random(720 * 2 / numPixels, 180); // wave period (width)
  }

  byte *ptr = &imgData[idx][0];
  int  foo;
  int theta;
  int offset;
  long color, i;
  int halfPeriod = fxVars[idx][5] / 2;
  int distance;
  long hue = pickHue(fxVars[idx][1]);
  for(long i=0; i<numPixels; i++) {
    // position of current pixel in 1/2 degrees
    offset = fxVars[idx][2] * i / numPixels;
    theta = offset - fxVars[idx][4];
    distance = (offset + fxVars[idx][4]) % fxVars[idx][2];
    foo = distance > fxVars[idx][5] || distance < 0 ? -127 : fixSin((distance * 360 / halfPeriod) - 180);
    // Peaks of sine wave are white, troughs are black, mid-range
    // values are pure hue (100% saturated).
    color = hsv2rgb(hue, 255, 127 + foo);
    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
  }
  fxVars[idx][4] += fxVars[idx][3];
  fxVars[idx][4] %= 720;
}

void renderEffectThrob(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    gammaRespondsToForce = false;
    fxVars[idx][1] = random(maxHue + 1); // Random hue
    // Number of repetitions (complete loops around color wheel);
    // any more than 4 per meter just looks too chaotic.
    // Store as distance around complete belt in half-degree units:
    fxVars[idx][2] = (1 + random(4 * ((numPixels + 31) / 32))) * 720;
//    fxVars[idx][2] = 4;
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.  It's generally
    // still less than a full pixel per frame, making motion very smooth.
    fxVars[idx][3] = 4 + random(fxVars[idx][1] / 10) / numPixels;
    // Reverse direction half the time.
    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
    fxVars[idx][4] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
  }

  byte *ptr = &imgData[idx][0];
  int  foo;
  long color, i;
  long hue = pickHue(fxVars[idx][1]);
    foo = fixSin(fxVars[idx][4]);
  for(long i=0; i<numPixels; i++) {
    // Peaks of sine wave are white, troughs are black, mid-range
    // values are pure hue (100% saturated).
    color = hsv2rgb(hue, 255, 127 + foo);
    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
  }
  fxVars[idx][4] += fxVars[idx][3];
}

// TO DO: Add more effects here...Larson scanner, etc.


