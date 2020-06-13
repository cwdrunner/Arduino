#include "dht.h"

// DHT Humidity and Temperature module
#define dht_apin A0 // Analog Pin sensor is connected to
dht DHT;

// LED
#include <LiquidCrystal.h>
const int rs = 4, en = 5, d4 = 6, d5 = 7, d6 = 8, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
byte statusLed    = 13;

// Flow Sensor
byte sensorInterrupt = 0;  // 0 = digital pin 2
byte sensorPin       = 3;

float calibrationFactor = 7.5;
volatile byte pulseCount;

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;


// Loop variables
unsigned long dhtPrevEventTime = 0;
const unsigned long dhtEventInterval = 5000; // temp interval in ms

unsigned long flowPrevEventTime = 0;
const unsigned long flowEventInterval = 1000; // flow interval in ms

unsigned long prevDisplayTime = 0;
const unsigned long displayTimerInterval = 5000; //Change the display every three seconds
unsigned long displayInternal = 0;
int displayId = 1;

// "Database" - Thought about making this more sophiticated but want Arduino processing fast not like a db
float temperature = 0;
float humidity = 0;


void setup() {

  Serial.begin(9600);
  delay(500);//Delay to let system boot
  Serial.println("DHT11 Humidity & temperature Sensor\n\n");
  delay(1000);//Wait before accessing Sensor

  // Set up the status LED line as an output
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

}//end "setup()"

void loop() {
  /* Updates frequently */
  unsigned long currentTime = millis();

  // Rotate the display
  if ((currentTime - prevDisplayTime) > displayTimerInterval) {
    displayId = displayId + 1;
    if (displayId > 3) {
      displayId = 1;
    }
    Serial.print("Display id is ");
    Serial.print(displayId);
    prevDisplayTime = currentTime;

    if (displayId == 1) {
      lcd.begin(16, 2);
      lcd.print("Temperature ");
      lcd.setCursor(1, 1);
      lcd.print(temperature * 1.8 + 32);
      lcd.setCursor(6, 1);
      lcd.println("F");
    }

    if (displayId == 2) {
      lcd.begin(16, 2);
      lcd.print("Humidity ");
      lcd.setCursor(1, 1);
      lcd.print(humidity);
      lcd.setCursor(6, 1);
      lcd.println("%");
    }

    if (displayId == 3) {
      lcd.begin(16, 2);
      lcd.print("Flow Rate");
      lcd.setCursor(1, 1);
      lcd.print(flowRate);
      lcd.setCursor(6, 1);
      lcd.print("L/Min");
    }

  }

  if (currentTime - dhtPrevEventTime >= dhtEventInterval) {

    DHT.read11(dht_apin);

    // Store the temp, humidity
    temperature = DHT.temperature;
    humidity = DHT.humidity;

    Serial.print("\n Current humidity = ");
    Serial.print(DHT.humidity);
    Serial.print("%  ");
    Serial.print("temperature = ");
    Serial.print((DHT.temperature) * 1.8 + 32);
    Serial.println("F  ");

    /* Update the timing for the next event */
    dhtPrevEventTime = currentTime;

  }

  if (currentTime - flowPrevEventTime >= flowEventInterval) {

    detachInterrupt(sensorInterrupt);

    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - flowPrevEventTime)) * pulseCount) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    //oldTime = millis();

    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;

    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;

    unsigned int frac;

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

    /* Update the timing for the next event */
    flowPrevEventTime = currentTime;

  }
}// end loop()

/* Interrupt Service Routine (ISR). Called for every pulse */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
