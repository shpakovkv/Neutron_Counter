#include "Arduino.h"
#include <math.h>
#include "NeutronCounter.h"

extern NeutronCounter nCounter[];

NeutronCounter::NeutronCounter(uint8_t pinNum, uint8_t interruptNum, uint32_t pulseTime)
{
  pin = pinNum;
  intNum = interruptNum;
  pinMode(pin, INPUT);
  pulseAverageTime = pulseTime;
  pulseCounter = 0;
  timerOVF = 0;
  if (interruptNum == 0) {timePerTick = T1_mksFromPrescaler[T1_PRESCALER];}
  else if (interruptNum == 1) {timePerTick = T2_mksFromPrescaler[T2_PRESCALER];}
}

// set Timers registers
void NeutronCounter::init(){
  detachInterrupt(intNum);  // stop external interrupt
  // Timer init
  cli();
  if (intNum == 0)
  {
    TCCR1A = 0;  // flush Timer1 settings
    TCCR1B = 0;
    TIMSK1 = (1 << TOIE1);   // turn on Timer1 overflow interrupt
  }
  else if (intNum == 1)
  {
    TCCR2A = 0;  // flush Timer2 settings
    TCCR2B = 0;
    TIMSK2 = (1 << TOIE2);   // turn on Timer2 overflow interrupt
  }
  sei();
}

// Timer1 overflow interrupt handler
ISR(TIMER1_OVF_vect)
{
    ++nCounter[0].timerOVF;
}

// Timer2 overflow interrupt handler
ISR(TIMER2_OVF_vect)
{
    ++nCounter[1].timerOVF;
}

// stop interrupt handling
void NeutronCounter::stopCounting(){
  if (intNum == 0 && nCounter[0].signalContinues) 
  {
    nCounter[0].increasePulseNumber(round((double(TCNT1) + TIMER1_MAX_COUNT * nCounter[0].timerOVF) * nCounter[0].timePerTick / nCounter[0].pulseAverageTime));
  }
  else if (intNum == 1 && nCounter[1].signalContinues) 
  {
    nCounter[1].increasePulseNumber(round((double(TCNT2) + TIMER2_MAX_COUNT * nCounter[1].timerOVF) * nCounter[1].timePerTick / nCounter[1].pulseAverageTime));
  }
  
  detachInterrupt(intNum);  // stop external interrupt

  if (intNum == 0) { TCCR1B = 0; }      // stop Timer1
  else if (intNum == 1) { TCCR2B = 0;}  // stop Timer2 
}

// start interrupt handling
void NeutronCounter::startCounting(){
  cli();
  // reAttachInterrupt(intNum, SIGNAL_START_EDGE);
  if (intNum == 0)
  {
    EIFR |= (1 << INTF0);   // clear INT0 flag
    attachInterrupt(0, nSignalHandler0, SIGNAL_START_EDGE);
    TCCR1B |= (T1_PRESCALER << CS10);    // set Timer1 prescaler 8x (b010) and start Timer1
  }  
  else if (intNum == 1)
  {
    EIFR |= (1 << INTF1);   // clear INT1 flag
    attachInterrupt(1, nSignalHandler1, SIGNAL_START_EDGE);
    TCCR2B |= (T2_PRESCALER << CS20);    // set Timer2 prescaler 8x (b010) and start Timer2 
  }
  sei();
}

void NeutronCounter::increasePulseNumber(uint16_t n)
{
  pulseCounter += n;
}

void NeutronCounter::flush()
{
  timerOVF = 0;
  pulseCounter = 0;
  signalContinues = false;
}

uint16_t NeutronCounter::GetPulseNumber()
{
  return pulseCounter;
}

// External interrupt INT0 handler
void nSignalHandler0()
{
  if (nCounter[0].signalContinues)
  {
    // falling front detected (end of the signal)
    // calc signal lenght and divide by pulseAverageTime
    nCounter[0].reg_info = TCNT1;
    nCounter[0].ovf_info = nCounter[0].timerOVF;
    nCounter[0].have_new = true;
    nCounter[0].increasePulseNumber(round((double(nCounter[0].reg_info) + TIMER1_MAX_COUNT * nCounter[0].ovf_info) * nCounter[0].timePerTick / nCounter[0].pulseAverageTime));
    nCounter[0].signalContinues = false;
    reAttachInterrupt(nCounter[0].intNum, SIGNAL_START_EDGE);  // serch for rising front (new signal)
  }
  else 
  {
    // rising front detected (signal start)
    TCNT1 = 0;                  // clear Timer1 counter
    nCounter[0].timerOVF = 0;   // clear Timer1 overflow counter
    nCounter[0].signalContinues = true;  
    reAttachInterrupt(nCounter[0].intNum, SIGNAL_END_EDGE);   // serch for falling front (end of the signal)
  }
}

// External interrupt INT1 handler
void nSignalHandler1()
{
  if (nCounter[1].signalContinues)
  {
    // falling front detected (end of the signal)
    // calc signal lenght and divide by pulseAverageTime
    nCounter[1].reg_info = TCNT2;
    nCounter[1].ovf_info = nCounter[1].timerOVF;
    nCounter[1].have_new = true;
    nCounter[1].increasePulseNumber(round((double(nCounter[1].reg_info) + TIMER2_MAX_COUNT * nCounter[1].ovf_info) * nCounter[1].timePerTick / nCounter[1].pulseAverageTime));
    
    nCounter[1].signalContinues = false;
    reAttachInterrupt(nCounter[1].intNum, SIGNAL_START_EDGE);   // serch for rising front (new signal)
  }
  else 
  { 
    // rising front detected (signal start)
    TCNT2 = 0;                  // clear Timer2 counter
    nCounter[1].timerOVF = 0;   // clear Timer2 overflow counter
    nCounter[1].signalContinues = true;
    reAttachInterrupt(nCounter[1].intNum, SIGNAL_END_EDGE);   // serch for falling front (end of the signal)
  }
}

// attach interrupt without any changes to interrupt handling function
void reAttachInterrupt(uint8_t interruptNum, int mode) {
  switch(interruptNum){
    case 0:
      // reset Interrupn_0 mode to 00 and set new mode
      EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
      // turn on Interrupn_0
      EIMSK |= (1 << INT0);
      break;
    case 1:
      // reset Interrupn_1 mode to 00 and set new mode
      EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (mode << ISC10);
      // turn on Interrupn_1
      EIMSK |= (1 << INT1);
      break;
  }
}
