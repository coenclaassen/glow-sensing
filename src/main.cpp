// Glow Sensing Project
// This Arduino sketch controls a TSL2591 light sensor and AW9523 LED driver.
// It charges a sample with UV LEDs, then measures light decay over time.
// Communication via serial commands: "start <charge_sec> <measure_sec>"

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_TSL2591.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AW9523.h>

// Sensor and driver objects
Adafruit_TSL2591 tsl;
Adafruit_AW9523 aw;

// LED settings
const uint8_t uvLedPin = 1;
const int uvBrightness = 138; // ~20mA

// Initialization flags
bool aw_ok = false;
bool tsl_ok = false;

// Timing variables (set via serial command)
unsigned long startTime = 0;       // Start of charge phase
unsigned long chargeTime = 5000;   // Charge duration in ms
unsigned long measureTime = 60000; // Measure duration in ms
unsigned long measureStart = 0;    // Start of measure phase

// Sensor readings
uint16_t full = 0; // Full-spectrum light value

// State machine
int state = 0; // 0: IDLE, 1: CHARGE, 2: MEASURE, 3: DONE

// Gain settings for TSL2591
const tsl2591Gain_t gainTable[] = {
    TSL2591_GAIN_LOW,  // 1x
    TSL2591_GAIN_MED,  // 25x
    TSL2591_GAIN_HIGH, // 428x
    TSL2591_GAIN_MAX   // 9876x
};
const uint8_t gainMin = 0;
const uint8_t gainMax = 3;
uint8_t gainIndex = 3; // Start with max gain

// Integration time settings for TSL2591
const tsl2591IntegrationTime_t intTable[] = {
    TSL2591_INTEGRATIONTIME_100MS,
    TSL2591_INTEGRATIONTIME_200MS,
    TSL2591_INTEGRATIONTIME_300MS,
    TSL2591_INTEGRATIONTIME_400MS,
    TSL2591_INTEGRATIONTIME_500MS,
    TSL2591_INTEGRATIONTIME_600MS};
const uint8_t intMin = 0;
const uint8_t intMax = 5;
uint8_t intIndex = 5; // Start with max integration

// Adjustment cooldown
unsigned long lastAdjustMs = 0;
const unsigned long adjustCooldownMs = 1000;

// Thresholds for gain/integration adjustment
const uint16_t fullMax = 65535;
const uint16_t fullHigh = fullMax * 90 / 100; // 90% of max
const uint16_t fullLow = fullMax * 10 / 100;  // 10% of max

void setGainAndInt(uint8_t gainIdx, uint8_t intIdx)
{
  tsl.setGain(gainTable[gainIdx]);
  tsl.setTiming(intTable[intIdx]);
}

void initTSL2591()
{
  tsl_ok = tsl.begin();
  if (!tsl_ok)
  {
    Serial.println("TSL2591 NOT found!");
  }
  else
  {
    Serial.println("TSL2591 found,0,0,0,0,SETUP");
    tsl.setGain(gainTable[gainIndex]);
    tsl.setTiming(intTable[intIndex]);
    tsl.enable();
  }
}

// Initialize AW9523 LED driver
void initAW9523()
{
  aw_ok = aw.begin();
  if (!aw_ok)
  {
    Serial.println("AW9523 NOT found!");
  }
  else
  {
    Serial.println("AW9523 found,0,0,0,0,SETUP");
    aw.pinMode(uvLedPin, AW9523_LED_MODE);
    aw.analogWrite(uvLedPin, 0);
  }
}

void printCSV(unsigned long t, unsigned long measureStart, uint16_t full_val, uint8_t gain, uint8_t int_idx, const char *state)
{
  Serial.print(t);
  Serial.print(",");
  Serial.print(measureStart);
  Serial.print(",");
  Serial.print(full_val);
  Serial.print(",");
  Serial.print(gain);
  Serial.print(",");
  Serial.print(int_idx);
  Serial.print(",");
  Serial.println(state);
  Serial.flush();
}

void runStateMachine()
{
  unsigned long t;
  switch (state)
  {

  case 0: // IDLE
    // LEDs off
    aw.analogWrite(uvLedPin, 0);
    digitalWrite(LED_BUILTIN, LOW);

    // listen for START command
    if (Serial.available())
    {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();
      if (cmd.startsWith("start "))
      {
        String params = cmd.substring(6); // "5 60"
        int space = params.indexOf(' ');
        if (space > 0)
        {
          chargeTime = params.substring(0, space).toInt() * 1000L;
          measureTime = params.substring(space + 1).toInt() * 1000L;
        }
        startTime = millis();
        state = 1; // go to CHARGE
      }
    }
    printCSV(millis(), 0, 0, 0, 0, "IDLE");
    break;

  case 1:                                   // CHARGE
    aw.analogWrite(uvLedPin, uvBrightness); // turn leds on for charging
    digitalWrite(LED_BUILTIN, HIGH);
    t = millis();
    printCSV(t, 0, 0, 0, 0, "CHARGE");

    if (millis() - startTime >= chargeTime)
    { // turn leds off after charge time
      aw.analogWrite(uvLedPin, 0);
      digitalWrite(LED_BUILTIN, LOW);
      measureStart = millis();
      state = 2;
    }
    break;

  case 2: // MEASURE
{
  uint32_t lum = tsl.getFullLuminosity();

  // TSL2591 packing:
  // lower 16 bits = CH0 (full spectrum)
  // upper 16 bits = CH1 (IR)
  uint16_t ch0 = lum & 0xFFFF;   // FULL spectrum channel (CH0)
  uint16_t ch1 = lum >> 16;      // IR channel (CH1)

  // Estimate visible by subtracting IR (recommended for glow measurements)
  int32_t vis = (int32_t)ch0 - (int32_t)ch1;
  if (vis < 0) vis = 0;

  // Store as your "full" output (keep your existing variable name)
  full = (uint16_t)vis;

  t = millis();

  // --- Auto-adjust gain/integration based on VISIBLE signal (not IR) ---
  if (t - lastAdjustMs >= adjustCooldownMs)
  {
    if (full > fullHigh)
    {
      if (gainIndex > gainMin)
      {
        gainIndex--;
        setGainAndInt(gainIndex, intIndex);
        lastAdjustMs = t;
      }
      else if (intIndex > intMin)
      {
        intIndex--;
        setGainAndInt(gainIndex, intIndex);
        lastAdjustMs = t;
      }
    }
    else if (full < fullLow)
    {
      if (intIndex < intMax)
      {
        intIndex++;
        setGainAndInt(gainIndex, intIndex);
        lastAdjustMs = t;
      }
      else if (gainIndex < gainMax)
      {
        gainIndex++;
        setGainAndInt(gainIndex, intIndex);
        lastAdjustMs = t;
      }
    }
  }

  // Check if measurement time is up
  if (millis() - measureStart >= measureTime)
  {
    state = 3;
    break;
  }

  printCSV(t, measureStart, full, gainIndex, intIndex, "MEASURE");
  break;
}

  case 3: // DONE
    t = millis();
    printCSV(t, 0, 0, 0, 0, "DONE");
    state = 0; // Back to IDLE
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
  initTSL2591();
  initAW9523();
  aw.analogWrite(uvLedPin, 0);
}

void loop()
{
  


  
  if (aw_ok && tsl_ok)
  {
    runStateMachine();
  }
  else if (!aw_ok)
  {
    Serial.println("AW9523 connection lost!");
  }
  else if (!tsl_ok)
  {
    Serial.println("TSL2591 connection lost!");
  }
  
}
