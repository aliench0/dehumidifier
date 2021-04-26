#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C
#define RELAY_PIN 33
#define FORCE_OFF_PIN 18
#define FORCE_ON_PIN 5
#define TRESHOLD_UP_PIN 4
#define TRESHOLD_DOWN_PIN 15
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

unsigned long delayTime = 1000;
unsigned int fanHumidityTreshold = 55; // Value after which the fan is triggered.
unsigned int countOfLastSimilarMeasurments = 0;
unsigned int changeStateCounter = 10000; //Counts when we need to reconcile fan state, initial number must be higher than "changeStateCheckAfterSeconds"
unsigned int changeStateCheckAfterSeconds = 30; // Each 30 seconds we will check if we need reconcile the fan state.

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN,  OUTPUT);
  pinMode(TRESHOLD_UP_PIN,  INPUT_PULLUP);
  pinMode(TRESHOLD_DOWN_PIN,  INPUT_PULLUP);
  pinMode(FORCE_ON_PIN,  INPUT_PULLUP);
  pinMode(FORCE_OFF_PIN,  INPUT_PULLUP);
  
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1); // Don't proceed, loop forever
  }
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1); // Don't proceed, loop forever
  }
}

void loop() {
  bool tresholdUpPin = digitalRead(TRESHOLD_UP_PIN);
  bool tresholdDownPin = digitalRead(TRESHOLD_DOWN_PIN);
  bool forceOnPin = digitalRead(FORCE_ON_PIN);
  bool forceOffPin = digitalRead(FORCE_OFF_PIN);
  Serial.println("TRESHOLD UP " + String(isButtonOn(tresholdUpPin)));
  Serial.println("TRESHOLD DOWN " + String(isButtonOn(tresholdDownPin)));
  Serial.println("FORCE ON " + String(isButtonOn(forceOnPin)));
  Serial.println("FORCE OFF " + String(isButtonOn(forceOffPin)));
  Serial.println("Change state counter " + String(changeStateCounter) + " of " + String(changeStateCheckAfterSeconds));
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  

  if (isButtonOn(tresholdUpPin)) {
    incrementTreshold();
  } else if (isButtonOn(tresholdDownPin)) {
    decrementTreshold();
  }

  if (isButtonOn(forceOnPin)) {
    turnOnFan();
  } else if (isButtonOn(forceOffPin)) {
    turnOffFan();
  } else if (changeStateCounter > changeStateCheckAfterSeconds) {
    Serial.println("Reconciling the fan state.");
    changeStateCounter = 0;
    if(humidity > fanHumidityTreshold) {
      turnOnFan();
    } else {
      turnOffFan();
    }
  }

  bool isRelayOn = isRelayTurnOn();
  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(" *C");
  Serial.print("Humidity = ");
  Serial.print(humidity);
  Serial.println(" %");
  Serial.print("Treshold Humidity = ");
  Serial.print(fanHumidityTreshold);
  Serial.println(" %");
  Serial.println("Relay State: " + getOnOffText(isRelayOn));
  Serial.println();
  drawValues(temperature, humidity, isRelayOn, forceOnPin, forceOffPin);
  changeStateCounter++;
  delay(delayTime);
}

bool isButtonOn(bool value) {
  return !value; // Because we use pinMode INPUT_PULLUP when the button is on, the value is 0
}

void incrementTreshold() {
  if (fanHumidityTreshold < 100) {
    fanHumidityTreshold++;
    Serial.println("Increment treshold to " + String(fanHumidityTreshold));
  } else {
    Serial.println("Skip increment treshold. Max of 100% is reached.");
  }
}

void decrementTreshold() {
  if (fanHumidityTreshold > 0) {
    fanHumidityTreshold--;
    Serial.println("Decrement treshold to " + String(fanHumidityTreshold));
  } else {
    Serial.println("Skip increment treshold. Min of 0% is reached.");
  }
}


void turnOnFan() {
  Serial.println("Turn on fan.");
  digitalWrite(RELAY_PIN, LOW); // Low turns the fan on
}

void turnOffFan() {
  Serial.println("Turn off fan.");
  digitalWrite(RELAY_PIN, HIGH); // High turns the fan off
}

bool isRelayTurnOn() {
  return !digitalRead(RELAY_PIN); // Used "!" because when the relay is on the read value is 0.
}

String getOnOffText(bool isTurnedOn) {
  if (isTurnedOn) {
    return String("On");
  }
  return String("Off");
}

void drawValues(float temperature, float humidity, bool isFanOn, bool forceOnPin, bool forceOffPin) {
  String fanAutoModeString = !forceOnPin || !forceOffPin ? String("") : String("(A)");
  display.clearDisplay();
  
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("T:" + String(temperature) + "C");
  display.println("H:" + String(humidity) + "%");
  display.println("Hmax:" + String(fanHumidityTreshold) + "%");
  display.println("Fan:" + getOnOffText(isFanOn) + fanAutoModeString);

  display.display();
}
