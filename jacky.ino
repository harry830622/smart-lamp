#include <TimeLib.h>
#include "Adafruit_MPR121.h"

// Constants
const int minBrightness = 0;
const int minLight = 0;
const int maxBrightness = 255;
const int maxLight = 1023;

// Pin setup
const int ledPin = 3;
const int lightSensorPin = 1;
const int touchSensorPin = 4;
const int chargerReceiverPin = 2; // TODO

// Configs
const int detectingTime = 100;
const int countingTime = 1000;
const int fadeTime = 1500;
const int fadeStep = 1;
const int numLevels = 3;
const int lightThresholds[numLevels] = { 0, 5, maxLight };
const int levelTable[numLevels] = { 0, 2, 0 };
const int brightnessTable[numLevels] = { 0, 25, maxBrightness };

// States
const int stateDetecting = 0;
const int stateAutoDark= 1;
const int stateAutoBright= 2;
const int stateCounting= 3;
const int stateFading= 4;
const int stateDark= 5;
const int stateBright= 6;
const int stateDim = 7;

// Global variables
int currentState = 0;
int lastState = 0;
int nextState = 0;
int currentLevel = 0;
int nextLevel = 0;
int currentBrightness = 0;
int nextBrightness = 0;
int levelCounts[numLevels];
int maxCount = 0;
int maxCountLevel = 0;
int startTime = 0;
int startTimeInSecond = 0;
int startTimeInMinute = 0;
int fadeStepTime = 0;
int isLastTouched = 0;

// Functions
#define len(x) (sizeof(x) / sizeof(x[0]))

void resetLevelCounts() {
  for (int i = 0; i < numLevels; ++i) {
    levelCounts[i] = 0;
  }
}

int mapLightToLevel(int light) {
  for (int i = 0; i < numLevels; ++i) {
    if (light < lightThresholds[i]) {
      return levelTable[i];
    }
  }
  return 0;
}

int mapLevelToBrightness(int level) {
  return brightnessTable[level];
}

int computeFadeStepTime(int nextLevel, int currentLevel) {
  return fadeTime / (abs(mapLevelToBrightness(nextLevel) - mapLevelToBrightness(currentLevel)) / fadeStep);
}

Adafruit_MPR121 touchSensor = Adafruit_MPR121();

// Main
void setup() {
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  pinMode(chargerReceiverPin, INPUT);

  startTime = millis();

  touchSensor.begin(0x5A);
  touchSensor.setThreshholds(254, 127);
}

void loop() {
  // Serial.println(currentState);

  int light = analogRead(lightSensorPin);
  Serial.println(light);

  int t = millis();
  time_t tLib = now();
  int tInSecond = second(tLib);
  int tInMinute = minute(tLib);
  // Serial.println(tInSecond);
  // Serial.println(tInMinute);

  switch (currentState) {
    case stateDetecting: {
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      ++levelCounts[level];
      if (levelCounts[level] > maxCount) {
        maxCount = levelCounts[level];
        maxCountLevel = level;
      }
      if (t - startTime >= detectingTime) {
        switch (maxCountLevel) {
          case 0: {
            currentState = stateAutoDark;
            break;
          }
          case 2: {
            currentState = stateAutoBright;
            break;
          }
          default: {
            currentState = stateDetecting;
            break;
          }
        }
        resetLevelCounts();
        maxCount = 0;
        maxCountLevel = 0;
      }
      break;
    }

    case stateAutoDark: {
      currentLevel = 0;
      currentBrightness = brightnessTable[currentLevel];
      analogWrite(ledPin, currentBrightness);
      const int isTouched = touchSensor.touched();
      if (!(isTouched & _BV(touchSensorPin)) && (isLastTouched & _BV(touchSensorPin)) ) {
        currentState = stateFading;
        nextState = stateBright;
        nextLevel = numLevels - 1;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      if (level != currentLevel) {
        lastState = currentState;
        currentState = stateCounting;
        nextLevel = level;
        startTime = t;
        break;
      }
      break;
    }

    case stateAutoBright: {
      currentLevel = numLevels - 1;
      currentBrightness = brightnessTable[currentLevel];
      analogWrite(ledPin, currentBrightness);
      const int isTouched = touchSensor.touched();
      if (!(isTouched & _BV(touchSensorPin)) && (isLastTouched & _BV(touchSensorPin)) ) {
        currentState = stateFading;
        nextState = stateDark;
        nextLevel = 0;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      if (level != currentLevel) {
        lastState = currentState;
        currentState = stateCounting;
        nextLevel = level;
        startTime = t;
        break;
      }
      break;
    }

    case stateCounting: {
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      if (level != nextLevel) {
        currentState = lastState;
        break;
      }
      if (t - startTime > countingTime) {
        currentState = stateFading;
        switch (nextLevel) {
          case 0: {
            nextState = stateAutoDark;
            break;
          }
          case 2: {
            nextState = stateAutoBright;
            break;
          }
        }
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        break;
      }
      break;
    }

    case stateFading: {
      analogWrite(ledPin, currentBrightness);
      if (currentBrightness == nextBrightness) {
        currentState = nextState;
        currentLevel = nextLevel;
        break;
      }
      if (t - startTime >= fadeStepTime) {
        startTime += fadeStepTime;
        int nextBrightness = mapLevelToBrightness(nextLevel);
        if (currentBrightness < nextBrightness) {
          currentBrightness += fadeStep;
          if (currentBrightness >= nextBrightness) {
            currentState = nextState;
            currentLevel = nextLevel;
          }
        } else if (currentBrightness > nextBrightness) {
          currentBrightness -= fadeStep;
          if (currentBrightness <= nextBrightness) {
            currentState = nextState;
            currentLevel = nextLevel;
          }
        }
      }
      currentBrightness = max(minBrightness, currentBrightness);
      currentBrightness = min(maxBrightness, currentBrightness);
      break;
    }

    case stateDark: {
      currentLevel = 0;
      currentBrightness = brightnessTable[currentLevel];
      analogWrite(ledPin, currentBrightness);
      const int isTouched = touchSensor.touched();
      if (!(isTouched & _BV(touchSensorPin)) && (isLastTouched & _BV(touchSensorPin)) ) {
        currentState = stateFading;
        nextState = stateAutoBright;
        nextLevel = numLevels - 1;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      if (level == currentLevel) {
        lastState = currentState;
        currentState = stateCounting;
        nextLevel = level;
        startTime = t;
      }
      break;
    }

    case stateBright: {
      currentLevel = numLevels - 1;
      currentBrightness = brightnessTable[currentLevel];
      analogWrite(ledPin, currentBrightness);
      const int isTouched = touchSensor.touched();
      if (!(isTouched & _BV(touchSensorPin)) && (isLastTouched & _BV(touchSensorPin)) ) {
        currentState = stateFading;
        nextState = stateAutoDark;
        nextLevel = 0;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      if (level == currentLevel) {
        lastState = currentState;
        currentState = stateCounting;
        nextLevel = level;
        startTime = t;
      }
      break;
    }
  }
}
