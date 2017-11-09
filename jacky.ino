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
const int numDetections = 50;
const int countThreshold = 50;
const int fadeTime = 3000;
const int fadeStep = 5;

// States
const int stateDetecting = 0;
const int stateStable = 1;
const int stateCounting = 2;
const int stateFading = 3;

int currentState = 0;
int stableBrightness = 0;
int currentNumDetections = 0;
int levelCounts[numLevels];
int maxCount = 0;
int maxCountLevel = 0;
int stableLevel = 1;
int targetLevel = 1;
int currentCount = 0;
int startTime = 0;
int fadeStepTime = 0;

// Functions
void resetLevelCounts() {
  for (int i = 0; i < numLevels; ++i) {
    levelCounts[i] = 0;
  }
}

int mapLightToLevel(int light) {
  return numLevels - int(light / float(maxLight - minLight) * numLevels + 0.5) + 1;
}

// Arduino
void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  Serial.println(currentState);
  switch (currentState) {
    case stateDetecting: {
      ++currentNumDetections;
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      ++levelCounts[level];
      if (levelCounts[level] > maxCount) {
        maxCount = levelCounts[level];
        maxCountLevel = level;
      }
      if (currentNumDetections >= numDetections) {
        currentState = stateStable;
        stableLevel = maxCountLevel;
        stableBrightness = levelBrightness * stableLevel;
        currentNumDetections = 0;
        resetLevelCounts();
        maxCount = 0;
        maxCountLevel = 1;
      }
      break;
    }
    case stateStable: {
      analogWrite(ledPin, stableBrightness);
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      if (level != stableLevel) {
        currentState = stateCounting;
        targetLevel = level;
      }
      break;
    }
    case stateCounting: {
      analogWrite(ledPin, stableBrightness);
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      if (level == targetLevel) {
        ++currentCount;
      } else {
        currentState = stateStable;
        currentCount = 0;
      }
      if (currentCount >= countThreshold) {
        currentState = stateFading;
        currentCount = 0;
        fadeStepTime = fadeTime / (abs(targetLevel - stableLevel) * levelBrightness / fadeStep);
        startTime = millis();
      }
      break;
    }
    case stateFading: {
      analogWrite(ledPin, stableBrightness);
      int t = millis();
      if (t - startTime >= fadeStepTime) {
        startTime += fadeStepTime;
        int targetBrightness = targetLevel * levelBrightness;
        if (targetBrightness > stableBrightness) {
          stableBrightness += fadeStep;
          if (stableBrightness >= targetBrightness) {
            currentState = stateStable;
            stableLevel = targetLevel;
          }
        }
        if (targetBrightness < stableBrightness) {
          stableBrightness -= fadeStep;
          if (stableBrightness <= targetBrightness) {
            currentState = stateStable;
            stableLevel = targetLevel;
          }
        }
      }
      stableBrightness = max(minBrightness, stableBrightness);
      stableBrightness = min(maxBrightness, stableBrightness);
      break;
    }
  }
}
