#include "Arduino.h"
#include <math.h>
#include "NeutronCounter.h"

extern NeutronCounter nCounter[];

// Atmega328 Timer1 mks per tick for all available prescalers
static double T1_mksFromPrescaler[6] = {0, 0.0625, 0.5, 4, 16, 64};

// Atmega328 Timer2 mks per tick for all available prescalers
static double T2_mksFromPrescaler[8] = {0, 0.0625, 0.5, 2, 4, 8, 16, 64};

NeutronCounter::NeutronCounter(uint8_t pinNum, uint8_t interruptNum, uint32_t pulseTime)
{
  pin = pinNum;
  intNum = interruptNum;
  pinMode(pin, INPUT);
  pulseAverageTime = pulseTime;
  pulseCounter = 0;
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
    TCCR1B |= (1 << WGM12);   // set Timer1 CTC mode
    
  }
  else if (intNum == 1)
  {
    TCCR2A = 0;  // flush Timer2 settings
    TCCR2B = 0;
    TCCR2A |= (1 << WGM21);   // set Timer2 CTC mode
  }
  sei();
}

// Timer1 Compare A interrupt handler
ISR(TIMER1_COMPA_vect)
{
    nCounter[0].increasePulseNumber();
}

// Timer2 Compare A interrupt handler
ISR(TIMER2_COMPA_vect)
{
    nCounter[1].increasePulseNumber();
}

// stop interrupt handling
void NeutronCounter::stopCounting(){
  detachInterrupt(intNum);  // stop external interrupt

  if (intNum == 0) 
  { 
    TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 << CS12)); // stop Timer1
    TIMSK1 &= ~(1 << OCIE1A);                             // turn off Timer1 Compare A Match Interrupt
  }      
  else if (intNum == 1) 
  { 
    TCCR2B &= ~((1 << CS20) | (1 << CS21) | (1 << CS22));  // stop Timer2 
    TIMSK2 = 0;
  }  
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
  // timerOVF = 0; // delete
  pulseCounter = 0;
  signalContinues = false;

  // if (intNum == 0)
  // {
  //   TCNT1 = 0;                            // clear Timer1 counter
  //   TIFR1 |= (1 << OCF1A) | (1 << TOV1);  // clear Timer1 interrupt flags
  // }
  // else if (intNum == 1)
  // {
  //   TCNT2 = 0;                            // clear Timer2 counter
  //   TIFR2 |= (1 << OCF2A) | (1 << TOV2);  // clear Timer2 interrupt flags
  // }
  
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
    // signal's tail detected (end of the signal)
    TIMSK1 &= ~(1 << OCIE1A);                                 // turn off Timer1 Compare A Match Interrupt
    TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 << CS12));     // turn off Timer1
    nCounter[0].signalContinues = false;  
    reAttachInterrupt(nCounter[0].intNum, SIGNAL_START_EDGE); // serch for rising front (new signal)
  }
  else 
  {
    // signal's head detected (signal start)
    TCNT1 = 0;                            // clear Timer1 counter
    TIFR1 |= (1 << OCF1A) | (1 << TOV1);  // clear timer interrupt flags
    TIMSK1 = (1 << OCIE1A);               // turn on Timer1 Compare A Match Interrupt
    TCCR1B |= (T1_PRESCALER << CS10);     // set Timer1 prescaler and start Timer1
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
    TIMSK2 &= ~(1 << OCIE2A);                                   // turn off Timer2 Compare A Match Interrupt
    TCCR2B &= ~((1 << CS20) | (1 << CS21) | (1 << CS22));       // stop Timer2 
    nCounter[1].signalContinues = false;
    reAttachInterrupt(nCounter[1].intNum, SIGNAL_START_EDGE);   // serch for rising front (new signal)
  }
  else 
  { 
    // rising front detected (signal start)
    TCNT2 = 0;                            // clear Timer2 counter
    TIFR2 |= (1 << OCF2A) | (1 << TOV2);  // clear timer interrupt flags
    TIMSK2 = (1 << OCIE2A);               // turn on Timer1 Compare A Match Interrupt
    TCCR2B |= (T2_PRESCALER << CS20);     // set Timer2 prescaler 8x (b010) and start Timer2 
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
