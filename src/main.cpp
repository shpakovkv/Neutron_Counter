#include <Arduino.h>

#include "TM1637Display.h"
#include "SimpleLED.h"

#define DISP_CLK 6
#define DISP_DIO 7
#define DISP_BRIGHT 4   // default display brightness

#define BUT_PIN 4         // start Button pin
#define STATE_LED_PIN 5   // state LED indicator pin

#define COUNTING_TIME 10000      // [ms] default 10000 ms == 10 s
#define N1_INTERRUPT_PIN 2
#define N1_INTERRUPT 0      // D2 == Interrupt#0
#define N1_ANALOG_PIN A2    // neutron output pulled up to VDD (about +4 V)
#define N2_INTERRUPT_PIN 3
#define N2_INTERRUPT 1      // D3 == Interrupt#1
#define N2_ANALOG_PIN A3    // neutron output pulled up to VDD (about +4 V)
#define N_COUNTER_NUMBER 2  // number of used interrupts and instaces of nCounter class [1 or 2]

//==============================================================================

// function declaration
// void neutronCounter01();
// void neutronCounter02();
void displayResult();
void printRegisters();  // for debug
// void reAttachInterrupt(uint8_t interruptNum, int mode);

// variables
bool countingAllowed = false;
unsigned long lastAllowedTime = 0;
unsigned long lastDispTime = 0;
bool DEBUG = true;

// objects
TM1637Display disp(DISP_CLK, DISP_DIO);
SimpleLED state_indicator(STATE_LED_PIN);

// load and init NeutronCounter lib
#define N_COUNTERS_NUMBER 2     // number of interrupts used [1-2]
#define PULSE_TIME 5500         // [mks]
#include "NeutronCounter.h"

//==============================================================================
void setup() {
  state_indicator.init();
  // debug_port.init();

  analogReference(DEFAULT); // for analog reads
  disp.clear();
  disp.setBrightness(DISP_BRIGHT);  // brightness, [0 - 7]
  // display at startup
  // disp.setSlowMode();

  // disp.displayByte(_dash, _dash, _dash, _dash);
  disp.showNumberDec(0);

  // state_indicator.init();
  // debug_port.init();

  for (int i = 0; i < N_COUNTERS_NUMBER; i++) { nCounter[i].init();}

  // pinMode(10, OUTPUT);   // DEBUG
  if (DEBUG) { Serial.begin(57600);}  // DEBUG
  delay(200);

}

//==============================================================================
void loop() {
  // check for button pressed
  if (!countingAllowed && digitalRead(BUT_PIN) == HIGH )
  {
    disp.showNumberDec(0);
    countingAllowed = true;
    lastAllowedTime = millis();
    for (int i = 0; i < N_COUNTERS_NUMBER; i++) { nCounter[i].flush();}
    for (int i = 0; i < N_COUNTERS_NUMBER; i++) { nCounter[i].startCounting();}
    // for (int i = 0; i < N_COUNTERS_NUMBER; i++) { nCounter[i].startCounting();}
    state_indicator.on();
  }

  if (countingAllowed)
  {
    if (millis() - lastAllowedTime >= COUNTING_TIME)
    {
      for (int i = 0; i < N_COUNTERS_NUMBER; i++) { nCounter[i].stopCounting();}
      countingAllowed = false;
      // cli();
      state_indicator.off();
      // disp.setSlowMode();
      // displayResult();
      // disp.setFastMode();
      // sei();
    }
  }

  displayResult();

  // if (DEBUG && nCounter[0].have_new)
  // {
  //   Serial.print("TCNT1 = ");
  //   Serial.print(nCounter[0].reg_info);
  //   Serial.print(";  OVF = ");
  //   Serial.print(nCounter[0].ovf_info);
  //   Serial.print(";  width = ");
  //   Serial.print((double(nCounter[0].reg_info) + 65536.0 * nCounter[0].ovf_info) * nCounter[0].timePerTick, 3);
  //   Serial.print(" mks;  Count = ");
  //   Serial.println(round((double(nCounter[0].reg_info) + 65536L * nCounter[0].ovf_info) * nCounter[0].timePerTick / nCounter[0].pulseAverageTime));
  //   nCounter[0].have_new = false;
  // }
  // if (DEBUG && nCounter[1].have_new)
  // {
  //   Serial.print("TCNT2 = ");
  //   Serial.print(nCounter[1].reg_info);
  //   Serial.print(";  OVF = ");
  //   Serial.print(nCounter[1].ovf_info);
  //   Serial.print(";  width = ");
  //   Serial.print((double(nCounter[1].reg_info) + 256.0 * nCounter[1].ovf_info) * nCounter[1].timePerTick, 3);
  //   Serial.print(" mks;  Count = ");
  //   Serial.println(round((double(nCounter[1].reg_info) + 256 * nCounter[1].ovf_info) * nCounter[1].timePerTick / nCounter[1].pulseAverageTime));
  //   nCounter[1].have_new = false;
  // }
  
  
}

//==============================================================================

void displayResult()
{
  unsigned int number = 0;
  for (int i = 0; i < N_COUNTERS_NUMBER; i++)
  {
     number += nCounter[i].GetPulseNumber();
  }
  disp.showNumberDec(number);
  // Serial.println(number);  // DEBUG
}

// for debug
void printRegisters()
{
  Serial.print("  TCCR1A = ");
  Serial.print(TCCR1A);
  Serial.print("  TCCR1B = ");
  Serial.print(TCCR1B);
  Serial.print("  TIMSK1 = ");
  Serial.println(TIMSK1);
  
  Serial.print("  TCCR2A = ");
  Serial.print(TCCR2A);
  Serial.print("  TCCR2B = ");
  Serial.print(TCCR2B);
  Serial.print("  TIMSK2 = ");
  Serial.println(TIMSK2);
}