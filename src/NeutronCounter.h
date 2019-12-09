#ifndef NeutronCounter_h
#define NeutronCounter_h

// for Atmega328
#define INT0_PIN 2   // D2 == Interrupt#0
#define INT1_PIN 3   // D3 == Interrupt#1
#define SIGNAL_START_EDGE RISING
#define SIGNAL_END_EDGE FALLING
#define TIMER1_MAX_COUNT 65536
#define TIMER2_MAX_COUNT 256
#define T1_PRESCALER 2
#define T2_PRESCALER 2

#include "Arduino.h"

static double T1_mksFromPrescaler[6] = {0, 0.0625, 0.5, 4, 16, 64};
static double T2_mksFromPrescaler[5] = {0, 0.0625, 0.5, 2, 4};

class NeutronCounter
{
  public:
    // Constructor
    NeutronCounter(uint8_t pin_num, uint8_t interruptNum, uint32_t pulse_time);

    void init();          // set Timers registers (need to execute once before using NeutronCounter)
    void stopCounting();  // stop ext interrupt handling
    void startCounting(); // start ext interrupt handling
    void flush();         // reset counter
    void increasePulseNumber(uint16_t n=1);   // increase pulseCounter by value

    uint16_t GetPulseNumber();  // returns pulseNumber
    
    uint8_t pin;    // digital input pin (interrupt pin)
    int intNum;     // interrupt number
    
    uint32_t timerOVF;          // timer overflow counter
    double timePerTick;         // time in mks per timer tick
    uint32_t pulseAverageTime;  // [mks] time of single pulse max=4294967296 mks (~71.5 minutes)

    bool signalContinues;   // means the rising front (start) of the signal have been detected, 
    // and the falling front (end) of the signal is still not detected

    bool have_new = false;
    uint8_t reg_info = 0;
    uint32_t ovf_info = 0;

  private:
    uint16_t pulseCounter;      // registred pulse number max=65535
};


void nSignalHandler0();  // External interrupt INT0 handler
void nSignalHandler1();  // External interrupt INT1 handler

void reAttachInterrupt(uint8_t interruptNum, int mode);  // attach interrupt without any changes to interrupt handling function

#if (N_COUNTERS_NUMBER == 1)
// NeutronCounter(uint8_t pin_num, uint8_t interruptNum, unsigned int pulse_time);
NeutronCounter nCounter[N_COUNTERS_NUMBER] {{INT0_PIN, INT0, PULSE_TIME}};
#elif (N_COUNTERS_NUMBER == 2)
NeutronCounter nCounter[N_COUNTERS_NUMBER] {{INT0_PIN, INT0, PULSE_TIME}, {INT1_PIN, INT1, PULSE_TIME}};
#endif

#endif
