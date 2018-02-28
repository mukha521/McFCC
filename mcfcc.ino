// Arduino ProMini 328P 8MHz 3.3v
// McFCC - McFLY's Car Controller
// RC car controller with SBUS input, 2 servo/ESC outputs and RGB LED strip lights


// Pin 2 - Headlights LED strip data wire
// Pin 3 - Effects LED strip data wire
// Pin 4 - Rear lights LED strip data wire
// Pin 9  - Channel 1 PWM
// Pin 10 - Channel 2 PWM

// Channel 1 - Steering
// Channel 2 - Gas / Brake
// Channel 3 - Lights
// Channel 4 - Attention lights
// Channel 5 - Effect selector
// Channel 8 - Arming


#include "SBUS.h"
#include <FastLED.h>

// LED strips pins
#define PIN_HEADLIGHTS 2
#define PIN_EFFECTS    3
#define PIN_REARLIGHTS 4

// LED variables
CRGB ledsLights[2][7];
CRGB ledsEffects[5];


SBUS sbus(Serial); // SBUS over UART1

// === Control variables ===
boolean armed = false;                // Arm/disarm flag
boolean failsafe = false;             // Failsafe flag
uint8_t ctrlMovement = 0;             // Movement control byte (bit0 - moving backwards or not; bit1 - braking or not; bit2 - direction: 0forward, 1backward)
uint8_t ctrlLights = 0;               // Main lights control byte
uint8_t ctrlBlinkers = 0;             // Main lights control byte
uint8_t ctrlEffects = 0;              // Effect LED strip control byte
boolean attentionLights = false;      // Attention head lights
uint8_t battLevel = 0;                // Battery voltage warnings. 0 - good; 1 - low; 2 - critical;
// =========================
//#define cmRG  3
#define cmDIR 2
#define cmBRK 1
#define cmMOV 0

// Channel defaults (for not armed and failsafe states)
#define DEFAULT_CH1 1500
#define DEFAULT_CH2 1500

// Defining a gap in the center of throttle channel
#define CENTER_GAS 500
#define STOP_DELTA 80

void mixLights(void);
void mixEffects(void);


void setup() {
  
  // Lights setup
  pinMode(PIN_HEADLIGHTS, OUTPUT);
  pinMode(PIN_EFFECTS, OUTPUT);
  pinMode(PIN_REARLIGHTS, OUTPUT);
  FastLED.addLeds<WS2812, PIN_HEADLIGHTS, GRB>(ledsLights[0], 7);
  FastLED.addLeds<WS2812, PIN_REARLIGHTS, GRB>(ledsLights[1], 7);
  FastLED.addLeds<WS2812, PIN_EFFECTS, GRB>(ledsEffects, 5);

  
  // PWM setup
  pinMode(9, OUTPUT);   //PWM channel 1
  pinMode(10, OUTPUT);  //PWM channel 2
  noInterrupts();

  //16-bit Timer1 setup. We need 50Hz signal for servos/ESCs (20ms period)
  TCCR1A = 0b10100010;    //Fast PWM with TOP in ICR1, clear outputs on compare prescaler = 8
  TCCR1B = 0b00011010;
  ICR1 = 19999;           //TOP value = 20000-1 (20ms period)
  OCR1A = DEFAULT_CH1;    //Default PWM value for channel 1
  OCR1B = DEFAULT_CH2;    //Default PWM value for channel 2
  TCNT1 = 0;              //Zeroing the counter
  TIMSK1 |= 0b00000001;   //Timer1 TOIE bit up: enabling overflow interrupt

  interrupts();

  sbus.begin();
}

// === Global variables ===
int signal;             // Processing commands from reciever
uint8_t effectTaps = 0; // Ticks for effects;
uint8_t effectSub = 0;  // Subcounter fpr effect taps;
boolean effectBool = false; // Boolean var for effects (left/right etc.)
int     effectInt = 0;  // Integer for effecsts
uint8_t ticks = 0;      // ticks every 100ms, resets every 500ms;
boolean tickFlop;       // inverts every 500ms;
boolean warnFlop;       // inverts every 200ms;
boolean critFlop;       // inverts every 100ms;
uint32_t col;           // Temp colors;
uint8_t hCol;           // Temp hue position;
// ========================


void loop() {


  //Are we armed?
  armed = (sbus.getChannel(8) > 500) ? true : false;
  
  //Failsafe active?
  failsafe = (sbus.getFailsafeStatus() == SBUS_FAILSAFE_ACTIVE) ? true : false;
  
  //Movement control bits update
  signal = sbus.getChannel(2) - CENTER_GAS;
  if (abs(signal) <= STOP_DELTA) {
    // Slowing / neutral
    bitSet(ctrlMovement, cmBRK); //Braking
    if (bitRead(ctrlMovement, cmDIR) == 1) {
      // if we were moving backwards then next time it'll be rear gear
      bitSet(ctrlMovement, cmMOV);
    }
  } else {
    // Going forward / backward
    if (signal > 0) {
      // Forward
      bitClear(ctrlMovement, cmBRK); // Not braking
      bitClear(ctrlMovement, cmMOV); // Moving not backwards
      bitClear(ctrlMovement, cmDIR); // Forward direction
    } else {
      // Going backward or braking
      if (bitRead(ctrlMovement, cmMOV) == 0) {
        // Braking
        bitSet(ctrlMovement, cmBRK); //Braking
      } else {
        // Backwards
        bitClear(ctrlMovement, cmBRK); // Not braking
      }
      bitSet(ctrlMovement, cmDIR); // Backward direction
    }
  }

  //Basic lights controls
  signal = sbus.getChannel(4);
  attentionLights = (signal > 500) ? true : false;

  signal = sbus.getChannel(3);
  if (signal < 333) {
    ctrlLights = 0;
  } else if ((signal >= 333) and (signal <= 666)) {
    ctrlLights = 1;
  } else {
    ctrlLights = 2;
  }
  
  //Blinkers controls
  if (!armed or failsafe) {
    ctrlBlinkers = 3;
  } //Here will be some clever logic for turn indicators

  
  //Effect selector
  signal = sbus.getChannel(5);
  if (signal < 125) {
    ctrlEffects = 0;
  } else if ((signal > 125) and (signal <= 250)) {
    ctrlEffects = 1;
  } else if ((signal > 250) and (signal <= 375)) {
    ctrlEffects = 2;
  } else if ((signal > 375) and (signal <= 500)) {
    ctrlEffects = 3;
  } else if ((signal > 500) and (signal <= 625)) {
    ctrlEffects = 4;
  } else if ((signal > 625) and (signal <= 750)) {
    ctrlEffects = 5;
  } else if ((signal > 750) and (signal <= 875)) {
    ctrlEffects = 6;
  } else {
    ctrlEffects = 7;
  }
  
  //Checking the battery

  // Lights testing area
  //attentionLights = true;
  armed = true;
  failsafe = false;
  ctrlLights = 0;
  ctrlBlinkers = 0;
  //battLevel = 1;
  ctrlEffects = 6;
  
  //Mixing lights
  mixLights();
  
  //Process effects
  mixEffects();
  
  
  //Update LED strips
  FastLED[0].showLeds();
  FastLED[1].showLeds();
  FastLED[2].showLeds();
  FastLED.delay(100);
  //Ticking our little timer and flipping floppers
  ticks++;
  if (ticks > 221) {ticks = 0;}
  if (ticks % 6 == 0) {tickFlop = !tickFlop;}
  if (ticks % 2 == 0) {warnFlop = !warnFlop;}
  critFlop = !critFlop;
  
}


void mixLights() {
  // Mix head and rear lights

  // Clear the lights
  fill_solid(ledsLights[0], 7, 0);
  fill_solid(ledsLights[1], 7, 0);

  if (ctrlLights > 0) {
    // Basic lights
    ledsLights[0][1] = 0xCCCCCC;
    ledsLights[0][5] = 0xCCCCCC;
    ledsLights[1][1] = 0x880000;
    ledsLights[1][5] = 0x880000;
    if (ctrlLights == 2) {
      // Full lights
      ledsLights[0][2] = 0xCCCCCC;
      ledsLights[0][3] = 0xCCCCCC;
      ledsLights[0][4] = 0xCCCCCC;
    }
  }

  // Rear gear
  if (bitRead(ctrlMovement, cmDIR) == 1) {
    ledsLights[1][3] = 0xCCCCCC;
  }
  
  // Braking indicators
  if (bitRead(ctrlMovement, cmBRK) == 1) {
    ledsLights[1][1] = 0xFF0000;
    ledsLights[1][2] = 0xFF0000;
    ledsLights[1][4] = 0xFF0000;
    ledsLights[1][5] = 0xFF0000;
  }
  
  // Blinkers
  if (bitRead(ctrlBlinkers, 0) == 1 and tickFlop) {
    ledsLights[0][0] = 0xFF5500;
    ledsLights[1][0] = 0xFF5500;
  }
  if (bitRead(ctrlBlinkers, 1) == 1 and tickFlop) {
    ledsLights[0][6] = 0xFF5500;
    ledsLights[1][6] = 0xFF5500;
  }
  
  //Attention lights
  if (attentionLights) {
    fill_solid(ledsLights[0], 7, 0xFFFFFF);
  }
  
  // Battery level
  if (battLevel > 0 and critFlop) {
    col = (battLevel == 1) ? 0xFF3300 : 0xFF0000;
    ledsLights[0][0] = col;
    ledsLights[0][3] = col;
    ledsLights[0][6] = col;
    ledsLights[1][0] = col;
    ledsLights[1][3] = col;
    ledsLights[1][6] = col;
  }
}


const uint8_t pulseDelta = 32; // Brightness increment in effects
int deltaInc = 3;              // Delta for incrementing delta
const uint8_t rwSub[5] = {4, 2, 1, 2, 4}; // Subcounters for each position of the "eye" for "Road warrior" effect
void mixEffects() {
  // Lights off
  if (ctrlEffects != 6) {fill_solid(ledsEffects, 5, 0);}
  switch (ctrlEffects) {
    case 1:
      // Police slow blinks
      if (effectTaps >= 128) {
        effectSub = 0;
        col = 0xFF0000;
      } else {
        effectSub = 2;
        col = 0x0000FF;
      }
      fill_solid(ledsEffects + effectSub, 3, col);
      effectTaps += 16;
      break;
    case 2:
      // Police slow siren
      if (abs(effectInt) < pulseDelta) {effectInt = pulseDelta;}
      if (effectTaps >= 255 - pulseDelta) {
        effectInt = -pulseDelta;
        deltaInc = -deltaInc;
      } else if (effectTaps <= 10 + pulseDelta) {
        effectInt = pulseDelta;
        deltaInc = -deltaInc;
        effectBool = !effectBool;
      } 
      if (effectBool) {
        effectSub = 0;
        hCol = 0;
      } else {
        effectSub = 2;
        hCol = 160;
      }
      fill_solid(ledsEffects + effectSub, 3, CHSV(hCol, 255, effectTaps));
      effectInt += deltaInc;
      effectTaps += effectInt;
      break;
    case 3:
      // Police double taps
      if (effectTaps > 12) {effectTaps = 0;}
      switch (effectTaps) {
        case 0:
        case 2:
          fill_solid(ledsEffects, 3, 0xFF0000);
          break;
        case 4:
        case 6:
          fill_solid(ledsEffects + 2, 3, 0x0000FF);
          break;
      }
      effectTaps++;
      break;
    case 4:
      // Yellow slow blinks
      effectSub = (effectTaps >= 128) ? 0: 2;
      col = 0xFF5500;
      fill_solid(ledsEffects + effectSub, 3, col);
      effectTaps += 16;
      break;
    case 5:
      // Yellow slow siren
      if (abs(effectInt) < pulseDelta) {effectInt = pulseDelta;}
      if (effectTaps >= 255 - pulseDelta) {
        effectInt = -pulseDelta;
        deltaInc = -deltaInc;
      } else if (effectTaps <= 10 + pulseDelta) {
        effectInt = pulseDelta;
        deltaInc = -deltaInc;
        effectBool = !effectBool;
      } 
      if (effectBool) {
        effectSub = 0;
      } else {
        effectSub = 2;
      }
      hCol = 12;
      fill_solid(ledsEffects + effectSub, 3, CHSV(hCol, 255, effectTaps));
      effectInt += deltaInc;
      effectTaps += effectInt;
      break;
    case 6:
      // Road warrior (Cylon)
      if (abs(effectInt) != 1) {effectInt = 1;}
      if (effectSub > rwSub[0]) {effectSub = rwSub[0];}
      if (effectTaps >= 4) {
        effectTaps = 4;
        effectInt = -1;
      } else if (effectTaps == 0) {
        effectInt = 1;
      }
      //Dim all
      for (uint8_t i=0; i<5; i++) {
        ledsEffects[i].nscale8(128);
      }
      
      ledsEffects[effectTaps] = 0xFF0000; //Set current position to bright red
      effectSub--;
      if (effectSub == 0) {
        effectTaps += effectInt;
        effectSub = rwSub[effectTaps];
      }
      
      break;
    case 7:
      fill_solid(ledsEffects, 5, CHSV(effectTaps, 255, 255));
      effectTaps += 5;
      break;
  }
}


ISR(TIMER1_OVF_vect) {
 // Updating values for ch1 and ch2 PWMs
 if (armed and !failsafe) {
   OCR1A = 1000 + sbus.getChannel(1);
   OCR1B = 1000 + sbus.getChannel(2);  
 } else {
   OCR1A = DEFAULT_CH1;
   OCR1B = DEFAULT_CH2;
 }
 
}

ISR(TIMER2_COMPA_vect) {
  // Timer2 registers are configured in SBUS.cpp for 8HMz version of Arduino Pro Mini
  // Processing SBUS data
  sbus.process();
}
