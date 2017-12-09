#include <TimeLib.h>
#include "Adafruit_MPR121.h"

// Constants
const int minBrightness = 0;
const int minLight = 0;
const int maxBrightness = 255;
const int maxLight = 1023;

// Pin setup
const int lightSensorPin = 0;
const int chargerReceiverPin = 1; // TODO
const int touchSensorPin = 4;
const int ledPin = 5;

// Configs
const int detectingTime = 100;
const int levelCountingTime = 1000;
const int chargerCountingTime = 1000;
const int numLevels = 3;
const int lightThresholds[numLevels] = { -1, 5, maxLight };
const int levelTable[numLevels] = { 0, 2, 0 };
const int brightnessTable[numLevels] = { 0, 25, maxBrightness };
const int fadeTime = 1500;
const int fadeStep = 1;
const int chargingThreshold = 5;

// States
const int stateDetecting = 0;
const int stateDarkAuto= 1;
const int stateBrightAuto= 2;
const int stateLevelCounting= 3;
const int stateChargerVoltageCounting= 4;
const int stateFading= 5;
const int stateDark= 6;
const int stateBright= 7;
const int stateChargingDarkAuto= 8;
const int stateChargingDim = 9;
const int stateChargingDark= 10;

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
    if (light <= lightThresholds[i]) {
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

  startTime = millis();

  touchSensor.begin(0x5A);
  touchSensor.setThreshholds(254, 127);
}

void loop() {
  Serial.println(currentState);

  int t = millis();
  time_t tLib = now();
  int tInSecond = second(tLib);
  int tInMinute = minute(tLib);

  // Serial.println(tInSecond);
  // Serial.println(tInMinute);

  const int light = analogRead(lightSensorPin);
  const int isTouched = touchSensor.touched();
  const int chargerVoltage = analogRead(chargerReceiverPin);

  // Serial.println(light);
  // Serial.println(chargerVoltage);

  switch (currentState) {
    case stateDetecting: {
      int level = mapLightToLevel(light);
      ++levelCounts[level];
      if (levelCounts[level] > maxCount) {
        maxCount = levelCounts[level];
        maxCountLevel = level;
      }
      if (t - startTime >= detectingTime) {
        switch (maxCountLevel) {
          case 0: {
            currentState = stateDarkAuto;
            break;
          }
          case 2: {
            currentState = stateBrightAuto;
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

    case stateDarkAuto: {
      currentLevel = 0;
      currentBrightness = brightnessTable[currentLevel];
      analogWrite(ledPin, currentBrightness);
      if (!(isTouched & _BV(touchSensorPin)) && (isLastTouched & _BV(touchSensorPin)) ) {
        currentState = stateFading;
        nextState = stateBright;
        nextLevel = numLevels - 1;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      int level = mapLightToLevel(light);
      if (level != currentLevel) {
        lastState = currentState;
        currentState = stateLevelCounting;
        nextLevel = level;
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        break;
      }
      if (chargerVoltage > chargingThreshold) {
        lastState = currentState;
        currentState = stateChargerVoltageCounting;
        nextLevel = 0;
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        break;
      }
      break;
    }

    case stateBrightAuto: {
      currentLevel = numLevels - 1;
      currentBrightness = brightnessTable[currentLevel];
      analogWrite(ledPin, currentBrightness);
      if (!(isTouched & _BV(touchSensorPin)) && (isLastTouched & _BV(touchSensorPin)) ) {
        currentState = stateFading;
        nextState = stateDark;
        nextLevel = 0;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      int level = mapLightToLevel(light);
      if (level != currentLevel) {
        lastState = currentState;
        currentState = stateLevelCounting;
        nextLevel = level;
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        break;
      }
      if (chargerVoltage > chargingThreshold) {
        lastState = currentState;
        currentState = stateChargerVoltageCounting;
        nextLevel = 1;
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        break;
      }
      break;
    }

    case stateLevelCounting: {
      int level = mapLightToLevel(light);
      if (nextLevel != 1 && level != nextLevel) {
        currentState = lastState;
        break;
      }
      if (t - startTime > levelCountingTime) {
        currentState = stateFading;
        switch (nextLevel) {
          case 0: {
            nextState = stateDarkAuto;
            break;
          }
          case 1: {
            nextState = stateChargingDim;
            break;
          }
          case 2: {
            nextState = stateBrightAuto;
            break;
          }
          default: {
            nextState = lastState;
            break;
          }
        }
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        break;
      }
      break;
    }

    case stateChargerVoltageCounting: {
      if (chargerVoltage <= chargingThreshold) {
        currentState = lastState;
        break;
      }
      if (t - startTime > chargerCountingTime) {
        currentState = stateFading;
        switch (nextLevel) {
          case 0: {
            nextState = stateChargingDarkAuto;
            break;
          }
          case 1: {
            nextState = stateChargingDim;
            break;
          }
          default: {
            nextState = lastState;
            break;
          }
        }
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
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
      if (!(isTouched & _BV(touchSensorPin)) && (isLastTouched & _BV(touchSensorPin)) ) {
        currentState = stateFading;
        nextState = stateBrightAuto;
        nextLevel = numLevels - 1;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      int level = mapLightToLevel(light);
      if (level == currentLevel) {
        lastState = currentState;
        currentState = stateLevelCounting;
        nextLevel = level;
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
      }
      if (chargerVoltage > chargingThreshold) {
        lastState = currentState;
        currentState = stateChargerVoltageCounting;
        nextLevel = 1;
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        break;
      }
      break;
    }

    case stateBright: {
      currentLevel = numLevels - 1;
      currentBrightness = brightnessTable[currentLevel];
      analogWrite(ledPin, currentBrightness);
      if (!(isTouched & _BV(touchSensorPin)) && (isLastTouched & _BV(touchSensorPin)) ) {
        currentState = stateFading;
        nextState = stateDarkAuto;
        nextLevel = 0;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      int level = mapLightToLevel(light);
      if (level == currentLevel) {
        lastState = currentState;
        currentState = stateLevelCounting;
        nextLevel = level;
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
      }
      if (chargerVoltage > chargingThreshold) {
        lastState = currentState;
        currentState = stateChargerVoltageCounting;
        nextLevel = 1;
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        break;
      }
      break;
    }

    case stateChargingDarkAuto: {
      currentLevel = 0;
      currentBrightness = brightnessTable[currentLevel];
      analogWrite(ledPin, currentBrightness);
      if (!(isTouched & _BV(touchSensorPin)) && (isLastTouched & _BV(touchSensorPin)) ) {
        currentState = stateFading;
        nextState = stateChargingDim;
        nextLevel = 1;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      int level = mapLightToLevel(light);
      if (level != currentLevel) {
        lastState = currentState;
        currentState = stateLevelCounting;
        nextLevel = 1;
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        break;
      }
      if (chargerVoltage <= chargingThreshold) {
        currentState = stateDarkAuto;
        break;
      }
      break;
    }

    case stateChargingDim: {
      currentLevel = 1;
      currentBrightness = brightnessTable[currentLevel];
      analogWrite(ledPin, currentBrightness);
      if (!(isTouched & _BV(touchSensorPin)) && (isLastTouched & _BV(touchSensorPin)) ) {
        currentState = stateFading;
        nextState = stateChargingDark;
        nextLevel = 0;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      // TODO: Count down in minutes.
      if (tInSecond < startTimeInSecond) {
        tInSecond += 60;
      }
      if (tInSecond - startTimeInSecond >= 5) {
        currentState = stateFading;
        nextState = stateChargingDark;
        nextLevel = 0;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        break;
      }
      if (chargerVoltage <= chargingThreshold) {
        currentState = stateFading;
        nextState = stateBrightAuto;
        nextLevel = 2;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        break;
      }
      break;
    }

    case stateChargingDark: {
      currentLevel = 0;
      currentBrightness = brightnessTable[currentLevel];
      analogWrite(ledPin, currentBrightness);
      if (!(isTouched & _BV(touchSensorPin)) && (isLastTouched & _BV(touchSensorPin)) ) {
        currentState = stateFading;
        nextState = stateChargingDim;
        nextLevel = 1;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      if (chargerVoltage <= chargingThreshold) {
        currentState = stateFading;
        nextState = stateDarkAuto;
        nextLevel = 0;
        currentBrightness = brightnessTable[currentLevel];
        nextBrightness = brightnessTable[nextLevel];
        startTime = t;
        startTimeInSecond = tInSecond;
        startTimeInMinute = tInMinute;
        fadeStepTime = computeFadeStepTime(nextLevel, currentLevel);
        break;
      }
      break;
    }
  }
}
