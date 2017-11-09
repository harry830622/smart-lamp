// Pin setup
const int ledPin = 9;
const int lightSensorPin = 1;

// Configs
const int minBrightness = 0;
const int minLight = 0;
const int maxBrightness = 255;
const int maxLight = 1023;
const int numLevels = 5;
const int levelBrightness = (maxBrightness - minBrightness) / numLevels;
const int levelLight = (maxLight - minLight) / numLevels;

// States
const int stateInitial = 0;
const int stateDetecting = 1;
const int stateStable = 2;
const int stateCounting = 3;
const int stateFading = 4;

// stateInitial
const int initialLevel = 1;
const int initialBrightness = initialLevel * levelBrightness;

// stateDetecting
const int numDetections = 1000;

// stateStable

// stateCounting
const int currentCount = 0;

// stateFading
const int fadeTime = 3000;
const int fadeStep = 5;
const int countThreshold = 50;

//
int currentState = 0;
int currentLevel = initialLevel;
int currentBrightness = initialBrightness;
int currentLight = 0;
int currentNumDetections = 0;
int lightCounts[maxLight - minLight + 1];
int maxCount = 0;
int maxCountLight = 0;

// Functions
void resetLightCounts() {
  for (int i = 0; i < maxLight - minLight + 1; ++i) {
    lightCounts[i] = 0;
  }
}

int mapLightToLevel(int light) {
  return numLevels - int(light / float(maxLight - minLight) * numLevels) + 1;
}

// Arduino
void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  switch (currentState) {
    case stateInitial:
      analogWrite(ledPin, initialBrightness);
      currentState = stateStable;
      break;
    case stateDetecting:
      analogWrite(ledPin, currentBrightness);
      ++currentNumDetections;
      currentLight = analogRead(lightSensorPin);
      ++lightCounts[currentLight];
      if (lightCounts[currentLight] > maxCount) {
        maxCount = lightCounts[currentLight];
        maxCountLight = currentLight;
      }
      if (currentNumDetections >= numDetections) {
        currentState = stateStable;
        currentLevel = mapLightToLevel(maxCountLight);
        stableLevel = currentLevel;
        currentBrightness = levelBrightness * currentLevel;
        currentNumDetections = 0;
        resetLightCounts();
        maxCount = 0;
        maxCountLight = 0;
      }
      break;
    case stateStable:
      analogWrite(ledPin, currentBrightness);
      currentLight = analogRead(lightSensorPin);
      currentLevel = mapLightToLevel(currentLight);
      if (currentLevel != stableLevel) {
        currentState = stateFading;
        targetLevel = currentLevel;
        currentCount = 0;
      }
      break;
    case stateCounting:
      analogWrite(ledPin, currentBrightness);
      currentLight = analogRead(lightSensorPin);
      currentLevel = mapLightToLevel(currentLight);
      if (currentLevel != stableLevel) {
        ++currentCount;
      } else {
        currentState = stateStable;
        currentCount = 0;
      }
      if (currentCount >= countThreshold) {
        currentState = stateFading;
        currentCount = 0;
      }
      break;
    case stateFading:
      // TODO
      break;
  }
}
