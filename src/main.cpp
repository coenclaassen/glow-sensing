#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LTR390.h>
#include <Adafruit_AW9523.h>

Adafruit_LTR390 ltr;
Adafruit_AW9523 aw;

const uint8_t uvLed1 = 1; // AW9523 pin used for demo
bool aw_ok = false;
bool ltr_ok = false;

const int uvBrightness = 138; // 140 = 20mA       don't exceed 172 = 25mA this will exceed the current limit of the LED

unsigned long startTime = 0;
const long chargeTime = 5000; // milliseconds
uint32_t als = 0;

int state = 0; // state machine variable

const ltr390_gain_t gainTable[] = { // Gain options
    LTR390_GAIN_1,
    LTR390_GAIN_3,
    LTR390_GAIN_6,
    LTR390_GAIN_9,
    LTR390_GAIN_18};

const uint8_t gainMin = 0;
const uint8_t gainMax = 4;

unsigned long t = 0;
unsigned long lastGainChangeMs = 0;
uint8_t gainIndex = 0;
const unsigned long gainCooldownMs = 1000; // example value

const uint32_t alsMax = 1048575;            // 2^20 - 1
const uint32_t alsHigh = alsMax * 80 / 100; // ~80% full scale
const uint32_t alsLow = alsMax * 5 / 100;   // ~5% full scale

void setGain(uint8_t index)
{
  ltr.setGain(gainTable[index]);
}

void initLTR390()
{
  ltr_ok = ltr.begin();
  if (!ltr_ok)
  {
    Serial.println("LTR390 NOT found!");
  }
  else
  {
    Serial.println("LTR390 found");
    ltr.setMode(LTR390_MODE_ALS);
    ltr.setResolution(LTR390_RESOLUTION_20BIT);
    setGain(0);
  }
}

void initAW9523()
{
  aw_ok = aw.begin();
  if (!aw_ok)
  {
    Serial.println("AW9523 NOT found!");
  }
  else
  {
    Serial.println("AW9523 found");
    aw.pinMode(uvLed1, AW9523_LED_MODE);
    aw.analogWrite(uvLed1, 0);
  }
}

void printCSV(unsigned long t, uint32_t als_val, uint8_t gain, const char *state)
{
  Serial.print(t);
  Serial.print(",");
  Serial.print(als_val);
  Serial.print(",");
  Serial.print(gain);
  Serial.print(",");
  Serial.println(state);
}

void runStateMachine()
{
  switch (state)
  {

  case 0: // IDLE
    // LEDs off
    aw.analogWrite(uvLed1, 0);
    digitalWrite(LED_BUILTIN, LOW);

    // listen for START command
    printCSV(t, 0, 0, "IDLE");
    startTime = millis();
    state = 1; // TEMPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP replace later with serial command for start
    break;

  case 1:                                 // CHARGE
    aw.analogWrite(uvLed1, uvBrightness); // turn leds on for charging
    digitalWrite(LED_BUILTIN, HIGH);
    t = millis();
    printCSV(t, 0, 0, "CHARGE");

    if (millis() - startTime >= chargeTime)
    { // turn leds off after charge time
      aw.analogWrite(uvLed1, 0);
      digitalWrite(LED_BUILTIN, LOW);
      state = 2;
    }
    break;

  case 2: // MEASURE

    if (ltr.newDataAvailable())
    {
      als = ltr.readALS();
      t = millis();

      if (t - lastGainChangeMs >= gainCooldownMs)
      {

        if (als > alsHigh && gainIndex > gainMin)
        {
          gainIndex--;
          setGain(gainIndex);
          lastGainChangeMs = t;
        }
        else if (als < alsLow && gainIndex < gainMax)
        {
          gainIndex++;
          setGain(gainIndex);
          lastGainChangeMs = t;
        }
      }

      Serial.print(t);
      Serial.print(", ");
      Serial.print(ltr.getGain());
      Serial.print(", ");
      Serial.print(ltr.getResolution()),
          Serial.print(", ");
      Serial.print(als);
      Serial.println(); // debug

      // printCSV(t, als, gainIndex, "MEASURE");
    }
    break;

  case 3: // DONE
    printCSV(t, 0, 0, "DONE");
    // wait for RESET or START
    break;
  }
}

void setup()
{
  // Start serial and wait untill working
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  pinMode(LED_BUILTIN, OUTPUT);
  initLTR390();
  initAW9523();
}

void loop()
{
  if (aw_ok && ltr_ok)
  {
    runStateMachine();
  }
  else if (!aw_ok)
  {
    Serial.println("AW9523 connection lost!");
  }
  else if (!ltr_ok)
  {
    Serial.println("LTR390 connection lost!");
  }
}
