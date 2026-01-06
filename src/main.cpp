#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LTR390.h>
#include <Adafruit_AW9523.h>

Adafruit_LTR390 ltr;
Adafruit_AW9523 aw;

const uint8_t EXPANDER_PIN = 1; // AW9523 pin used for demo
bool aw_ok = false;


const int uvBrightness = 138; // 140 = 20mA       don't exceed 172 = 25mA this will exceed the current limit of the LED

void initLTR390() {
  if (!ltr.begin()) {
    Serial.println("Couldn't find LTR390 sensor!");
  } else {
    Serial.println("Found LTR390");
    ltr.setMode(LTR390_MODE_ALS);
    ltr.setGain(LTR390_GAIN_3); // Options are: LTR390_GAIN_1, LTR390_GAIN_3, LTR390_GAIN_6, LTR390_GAIN_9 or LTR390_GAIN_18
    ltr.setResolution(LTR390_RESOLUTION_16BIT); // Higher resolution takes longer to read. Options are: LTR390_RESOLUTION_13BIT, LTR390_RESOLUTION_16BIT, LTR390_RESOLUTION_17BIT, LTR390_RESOLUTION_18BIT, LTR390_RESOLUTION_19BIT or LTR390_RESOLUTION_20BIT

  }
}

void initAW9523() {
  aw_ok = aw.begin();
  if (!aw_ok) {
    Serial.println("Couldn't find AW9523 expander!");
  } else {
    Serial.println("Found AW9523");
    aw.pinMode(EXPANDER_PIN, OUTPUT);
    aw.analogWrite(EXPANDER_PIN, 0);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  initLTR390();
  initAW9523();

  pinMode(LED_BUILTIN, OUTPUT);


}







void loop() {
  // Read UV data when available
  if (ltr.newDataAvailable()) {
    Serial.print("ALS: ");
    Serial.println(ltr.readALS());
  }


  
  digitalWrite(LED_BUILTIN, HIGH);
  aw.analogWrite(EXPANDER_PIN, uvBrightness);
  delay(500);
  aw.analogWrite(EXPANDER_PIN, 0);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);

  
}
