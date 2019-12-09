#ifndef SimpleLED_h
#define SimpleLED_h

#include <Arduino.h>

class SimpleLED
{
  public:
  
    SimpleLED(int led_pin)
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

#endif