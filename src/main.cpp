#include "Arduino.h"
#include "GyverTM1637.h"

#define DISP_CLK 6
#define DISP_DIO 7
#define DISP_BRIGHT 4
#define DISP_UPD_EVERY 300  // update display period [milliseconds]

#define BUT_PIN 4
#define STATE_LED_PIN 5

#define COUNTING_TIME 10000      // [ms] default 10000 ms == 10 s
#define PULSE_TIME 5500         // [mks]
#define N1_INTERRUPT_PIN 2
#define N1_INTERRUPT 0      // D2 == Interrupt#0
#define N1_ANALOG_PIN A2    // neutron output pulled up to VDD (about +4 V)
#define N2_INTERRUPT_PIN 3
#define N2_INTERRUPT 1      // D3 == Interrupt#1
#define N2_ANALOG_PIN A3    // neutron output pulled up to VDD (about +4 V)
#define N_COUNTER_NUMBER 2  // number of used interrupts and instances of nCounter class [1 or 2]

class nCounter
{
  public:
    // static unsigned int neutronCounterNumber;

    // means the level is still low, maybe it's two or more signals one by one
    bool signalContinues;

    // front detected, need to update lastSignalTime
    bool needToUpdateTime;

    // Constructor - creates an instance of nCounter
    // and initializes the member variables and state
    nCounter(int digital_pin, int analog_pin, unsigned int pulse_time)
    {
      digitalPin = digital_pin;
      analogPin = analog_pin;
      pinMode(digitalPin, INPUT);
      pinMode(analogPin, INPUT);
      pulseTime = pulse_time;

      pulseNumber = 0;
      // threshold = 800;
      signalContinues = false;
      needToUpdateTime = false;
      // neutronCounterNumber++;
    }

    // checks signal
    // and update pulse number if the signal is too long for 1 pulse
    // return 1 on update number, else 0
    bool Update()
    {
      bool numIncreased = false;

      // right after front detection we need to update lastSignalTime
      if (needToUpdateTime)
      {
        lastSignalTime = micros();
        needToUpdateTime = false;
        numIncreased = true;
      }

      // check the duration of too long signal
      if (signalContinues)
      {
        // if (analogRead(analogPin) > threshold)
        if (digitalRead(analogPin) == HIGH)
        {
          unsigned long currentMicros = micros();
          // consider long signals as an overlay of several single
          if(currentMicros - lastSignalTime >= pulseTime)
          {
            pulseNumber++;
            lastSignalTime += PULSE_TIME;
            numIncreased = true;
          }
        }
        else
        {
          signalContinues = false;
        }
      }
      return numIncreased;
    }

    void increasePulseNumber()
    {
      pulseNumber++;
    }

    // resets all variables to initial state
    // do it before starts new counting process
    void Flush()
    {
      pulseNumber = 0;
      lastSignalTime = micros() - pulseTime;
      needToUpdateTime = false;
      signalContinues = false;
    }

    unsigned long GetPulseNumber()
    {
      return pulseNumber;
    }

  private:
    // These are initialized at startup

    // digital input pin (for interrupt)
    int digitalPin;

    // analog input pin (for back front & duration analysis)
    int analogPin;

    // 0-1023 when analog pin level is lower than this value,
    // then we assume that the pulse has ended
    // int threshold;

    // [mks] time of single pulse
    unsigned int pulseTime;


    // These maintain the current state
    unsigned long pulseNumber;     // registred pulse counter
    unsigned long lastSignalTime;  // [mks] last signal appearance time
};

class Led
{
  public:
    Led (int led_pin)
    {
      pin = led_pin;
    }

    void init()
    {
      pinMode(pin, OUTPUT);
      // delay(50);
      off();
    }

    void on()
    {
      digitalWrite(pin, HIGH);
      is_on = true;
    }

    void off()
    {
      digitalWrite(pin, LOW);
      is_on = false;
    }

    void toggle()
    {
      if (is_on)
      {
        digitalWrite(pin, LOW);
        is_on = false;
      }
      else
      {
        digitalWrite(pin, HIGH);
        is_on = true;
      }
    }

  private:
    int pin = -1;
    bool is_on = false;
};

//==============================================================================

// functions
void displayScientific(GyverTM1637 disp, unsigned long value);
void neutronCounter01();
void neutronCounter02();
void displayResult();

// variables
bool countingAllowed = false;
unsigned long lastAllowedTime = 0;
unsigned long lastDispUpd = 0;
bool DEBUG = false;



// objects
GyverTM1637 disp(DISP_CLK, DISP_DIO);
Led state_indicator(STATE_LED_PIN);
// Led debug_port(A1);

// use this for 1 interrupt setup
//nCounter neutronCounter[N_COUNTER_NUMBER] {{N1_INTERRUPT_PIN, N1_ANALOG_PIN, PULSE_TIME}};

// use this for 2 interrupt setup
 nCounter neutronCounter[N_COUNTER_NUMBER] {{N1_INTERRUPT_PIN, N1_ANALOG_PIN, PULSE_TIME},
                                           {N2_INTERRUPT_PIN, N2_ANALOG_PIN, PULSE_TIME}};

// nCounter neutronCounter1(N1_INTERRUPT_PIN, N1_ANALOG_PIN, PULSE_TIME);
// nCounter neutronCounter2(N2_INTERRUPT_PIN, N2_ANALOG_PIN, PULSE_TIME);


//==============================================================================
void setup() {
  analogReference(DEFAULT); // for analog reads
  disp.clear();
  disp.brightness(DISP_BRIGHT);  // brightness, [0 - 7]

  // initialaze interrupt handler
  attachInterrupt(N1_INTERRUPT, neutronCounter01, RISING);
  if (N_COUNTER_NUMBER == 2) attachInterrupt(N2_INTERRUPT, neutronCounter02, RISING);
  // arduino nano/uno has only 2 interupt pin

  // initial value
  countingAllowed = false;

  state_indicator.init();
  // debug_port.init();

  if (DEBUG)
  {
    pinMode(10, OUTPUT);   // DEBUG
    Serial.begin(57600);  // DEBUG
  }

  delay(200);
}

//==============================================================================
void loop() {
  // check for button pressed
  if (!countingAllowed && digitalRead(BUT_PIN) == HIGH )
  {
    disp.displayInt(0);
    countingAllowed = true;
    lastAllowedTime = millis();
    for (int i = 0; i < N_COUNTER_NUMBER; i++) { neutronCounter[i].Flush();}
    state_indicator.on();

    // neutronCounter[0].needToUpdateTime = true; // DEBUG
    // neutronCounter[0].signalContinues = true;  // DEBUG
  }

  // counting if needed
  if (countingAllowed)
  {
    // consider long signals as an overlay of several single
    if (millis() - lastAllowedTime < COUNTING_TIME)
    {
      bool needToUpdate = false;
      for (int i = 0; i < N_COUNTER_NUMBER; i++)
      {
        needToUpdate = needToUpdate || neutronCounter[i].Update();
      }
      if (needToUpdate)
      {
        displayResult();  // update display if needed
      }
    }
    // stop counting
    else
    {
      countingAllowed = false;
      state_indicator.off();
      displayResult();
      lastDispUpd = millis();

      if (DEBUG)
      {
        Serial.print("nCounter[0] = ");
        Serial.print(neutronCounter[0].GetPulseNumber());
        Serial.print("  nCounter[1] = ");
        Serial.println(neutronCounter[1].GetPulseNumber());
      }
    }
  }
  else
  {
    if (millis() - lastDispUpd > DISP_UPD_EVERY) 
    { 
      displayResult();
      lastDispUpd = millis();
    }
  }

}

//==============================================================================

void displayResult()
{
  unsigned int number = 0;
  for (int i = 0; i < N_COUNTER_NUMBER; i++)
  {
     number += neutronCounter[i].GetPulseNumber();
  }
  disp.displayInt(number);
  // Serial.println(number);  // DEBUG
}

// display numbers > 9999
void displayScientific(GyverTM1637 disp, unsigned long value){
  uint8_t digits[4];
  digits[2] = _E;
  int iDigits[4];
  int numOrder = 0;
  for (int i = 1; i < 16; i ++){
    if (value / pow(10, i) < 1){
      numOrder = i - 1;
      break;
    }
  }
  iDigits[0] = int(value / pow(10, numOrder));
  iDigits[1] = int(value / pow(10, numOrder - 1)) % 10;
  iDigits[3] = int(value / pow(10, numOrder - 2)) % 10;

  // insted of round()
  iDigits[1] += iDigits[2] / 5;
  // when after round we get iDigits[1] == 10
  iDigits[0] += iDigits[1] / 10;
  iDigits[1] %= 10;
  // when after round we get iDigits[0] == 10
  if (iDigits[0] == 10)
  {
    iDigits[0] = 1;
    iDigits[1] = 0;
    numOrder++;
  }

  digits[0] = TubeTab[iDigits[0]]+ 0x80; // dot added
  digits[1] = TubeTab[iDigits[1]];
  digits[3] = TubeTab[numOrder];
  disp.displayByte(digits);
}

// if counting allowed increases pulse number
void neutronCounter01()
{
  if (countingAllowed)
  {
    neutronCounter[0].increasePulseNumber();
    neutronCounter[0].needToUpdateTime = true;
    neutronCounter[0].signalContinues = true;
    // debug_port.toggle();
  }
}

void neutronCounter02()
{
  if (countingAllowed)
  {
    neutronCounter[1].increasePulseNumber();
    neutronCounter[1].needToUpdateTime = true;
    neutronCounter[1].signalContinues = true;
    // debug_port.toggle();
  }
}
