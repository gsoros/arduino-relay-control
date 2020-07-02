#include <Arduino.h>

/*
 Advanced Home Automation Artificial Intelligence ;)
 - measures input voltage
 - turns on a random output above a threshold
 - turns off a random output below a threshold
 - turns on the wind turbine brake above a threshold
 - turns off the wind turbine brake below a threshold
 - turns on the inverter above a threshold
 - turns off the inverter below a threshold
 - repeats this over and over and over

 1.5k resistor from A0 to ground, and 20k resistor from A0 to +bat
 optional 100n cap from A0 to ground for stable readings
 (20k + 1.5k) / 1.5k = 14.33333
 */

const float divisorFactor = 14.333333;

const float loadOn = 13.3;
const float loadOff = 12.5;
const float windBrakeOn = 13.9;
const float windBrakeOff = 13.4;
const float inverterOn = 13.8;
const float inverterOff = 12.6;

const unsigned int ledDelay = 100; // ms
const unsigned int defaultDelay = 400; // ms
const unsigned int windBrakeDelay = 150; // ms
const unsigned int inverterDelay = 20; // loops
unsigned int loopDelay = defaultDelay;
unsigned int inverterCounter = 0;

// analog reference voltage calibration
const float aRef = 1.071;

// use the internal ~1.1V reference
const int refSource = INTERNAL;

// analog measurement pin
const int measurementPin = A0;

// unconnected analog pin
const int unconnectedPin = A1;

const int ledPin = 13;

// relay pins
const int loadPin[] = {2, 3, 4};

// initial relay states
int loadState[] = {LOW, LOW, LOW};

// wind turbine brake relay pin
const int windPin = 5;

// initial wind turbine brake state
int windState = LOW;

// inverter relay pin
const int inverterPin = 6;

// initial inverter state
int inverterState = LOW;

// number of relays
const int numLoad = sizeof(loadPin) / sizeof(int);

// assume this voltage when measurement overflows
const float overVolt = 20;

// max 64 readings
unsigned int measurementTotal;

// measured input voltage
float voltage;

int switchLoad(int state)
{
  // select relays that do not have the desired state
  int candidate[numLoad];
  int numCandidates = 0;
  for (int i = 0; i < numLoad; i++)
  {
    if (loadState[i] != state)
    {
      candidate[numCandidates] = i;
      numCandidates++;
    }
  }

  if (0 == numCandidates)
  {
    return 0;
  }

  int target = candidate[random(numCandidates)];
  loadState[target] = state;
  digitalWrite(loadPin[target], state);

  return 1;
}


void setup() {

  // set reference source
  analogReference(refSource);

  // set up serial monitor
  Serial.begin(9600);

  // set initial pin states
  for (int i = 0; i < numLoad; i++) {
    digitalWrite(loadPin[i], loadState[i]);
  }
  digitalWrite(windPin, windState);
  digitalWrite(inverterPin, inverterState);

  // initialize the digital pins as an output
  pinMode(ledPin, OUTPUT);
  for (int i = 0; i < numLoad; i++) {
    pinMode(loadPin[i], OUTPUT);
  }
  pinMode(windPin, OUTPUT);
  pinMode(inverterPin, OUTPUT);

  // seed
  randomSeed(analogRead(unconnectedPin));
}

void loop() {

  // turn led on
  digitalWrite(ledPin, HIGH);

  // multiple analog readings for averaging
  for (int x = 0; x < 64; x++) {
    measurementTotal = measurementTotal + analogRead(measurementPin);
  }

  if (measurementTotal == (1023 * 64)) {
    Serial.println("measurement overflow");
    voltage = overVolt;
  }
  else {
    voltage = measurementTotal / 64 * divisorFactor * aRef / 1024;
  }
  // reset measurementTotal
  measurementTotal = 0;

  // relay logic
  if (voltage >= loadOn)
  {
    switchLoad(HIGH);
  }
  else if (voltage < loadOff)
  {
    switchLoad(LOW);
  }

  // wind brake logic
  if (voltage >= windBrakeOn)
  {
    windState = HIGH;
    loopDelay = windBrakeDelay;
  }
  else if (voltage < windBrakeOff)
  {
    windState = LOW;
    loopDelay = defaultDelay;
  }
  digitalWrite(windPin, windState);
  
  // inverter logic
  if (inverterCounter < inverterDelay)
  {
    inverterCounter++;
  }
  else if (voltage >= inverterOn)
  {
    inverterState = HIGH;
    inverterCounter = 0;
  }
  else if (voltage < inverterOff)
  {
    inverterState = LOW;
    inverterCounter = 0;
  }
  digitalWrite(inverterPin, inverterState);

  Serial.print("VCC: ");
  if (voltage <= 1)
  {
    Serial.print("too low: ");
    Serial.print(voltage);
  }
  else
  {
    Serial.print(voltage);
    Serial.print("V");
  }
  Serial.print(", relays: ");
  for (int i=0; i<numLoad; i++)
  {
    Serial.print(loadState[i]);
  }
  Serial.print(", wind brake: ");
  Serial.print(windState);
  Serial.print(", inverter: ");
  Serial.print(inverterState);  
  Serial.print(", inverter delay: ");
  Serial.println(inverterCounter);

  delay(ledDelay);
  digitalWrite(ledPin, LOW);

  delay(loopDelay);
}
