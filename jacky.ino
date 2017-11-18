#include <TimeLib.h>
#include <Wire.h>
#include "Adafruit_MPR121.h"

Adafruit_MPR121 cap = Adafruit_MPR121();

// Pin setup
const int ledPin = 9;
const int lightSensorPin = 1;
const int chargerReceiverPin = 2;

// Configs
const int numLevels = 3;
const int detectingTime = 100;
const int countingTime = 1000;
const int fadeTime = 1500;
const int fadeStep = 1;

// Constants
const int minBrightness = 0;
const int minLight = 0;
const int maxBrightness = 255;
const int maxLight = 1023;

// States
const int stateDetecting = 0;
const int stateStable = 1;
const int stateCounting = 2;
const int stateFading = 3;
const int stateMiddle21 = 4;
const int stateMiddle22 = 5;
const int stateDark1 = 6;
const int stateDark2 = 7;
const int stateFadingMiddle21 = 8;
const int stateFadingMiddle22 = 9;
const int stateFadingDark1 = 10;
const int stateFadingDark2 = 11;
const int stateCountingLight2 = 12;
const int stateCountingLight0 = 13;

// Global variables
int currentState = 0;
int stableBrightness = 0;
int currentBrightness = 0; // TODO: Refactor
int levelCounts[numLevels];
int maxCount = 0;
int maxCountLevel = 0;
int stableLevel = 0;
int targetLevel = 0;
int startTime = 0;
int startTimeInSecond = 0;
int startTimeInMinute = 0;
int fadeStepTime = 0;
int isLastTouched = 0;
int currtouched = 0;

// Functions
void resetLevelCounts() {
  for (int i = 0; i < numLevels; ++i) {
    levelCounts[i] = 0;
  }
}

int mapLightToLevel(int light) {
  const int level1 = 200;
  const int level2 = 700;
  if (light <= level1) {
    return 2;
  } else if (light <= level2) {
    return 1;
  } else {
    return 0;
  }
}

int mapLevelToBrightness(int level) {
  const int brightness1 = 25;
  const int brightness2 = maxBrightness;
  if (level == 0) {
    return 0;
  } else if (level == 1) {
    return brightness1;
  } else {
    return brightness2;
  }
}

// Main
void setup() {
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  pinMode(chargerReceiverPin, INPUT);

  startTime = millis();

  cap.begin(0x5A);
  cap.setThreshholds(254, 127);
}

void loop() {
  // Serial.println(currentState);
  // int light = analogRead(lightSensorPin);
  // Serial.println(light);
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
        currentState = stateStable;
        stableLevel = maxCountLevel;
        resetLevelCounts();
        maxCount = 0;
        maxCountLevel = 1;
      }
      break;
    }

    case stateStable: {
      stableBrightness = mapLevelToBrightness(stableLevel);
      analogWrite(ledPin, stableBrightness);
      bool isCharging = digitalRead(chargerReceiverPin);
      if (isCharging) {
        currentState = stateFadingMiddle21;
        startTime = t;
        targetLevel = 1;
        fadeStepTime = fadeTime / (abs(mapLevelToBrightness(targetLevel) - mapLevelToBrightness(stableLevel)) / fadeStep);
        currentBrightness = stableBrightness;
        break;
      }
      int isTouched = cap.touched();
      if (!(isTouched & _BV(4)) && (isLastTouched & _BV(4)) ) {
        targetLevel = 0;
        if (stableLevel == targetLevel) {
          currentState = stateDark2;
          isLastTouched = 0;
          break;
        }
        currentState = stateFadingDark2;
        startTime = t;
        fadeStepTime = fadeTime / (abs(mapLevelToBrightness(targetLevel) - mapLevelToBrightness(stableLevel)) / fadeStep);
        currentBrightness = stableBrightness;
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      if (level != stableLevel) {
        currentState = stateCounting;
        startTime = t;
        targetLevel = level;
      }
      break;
    }

    case stateCounting: {
      stableBrightness = mapLevelToBrightness(stableLevel);
      analogWrite(ledPin, stableBrightness);
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      if (level != targetLevel) {
        currentState = stateStable;
      }
      if (t - startTime >= countingTime) {
        currentState = stateFading;
        startTime = t;
        fadeStepTime = fadeTime / (abs(mapLevelToBrightness(targetLevel) - mapLevelToBrightness(stableLevel)) / fadeStep);
        currentBrightness = stableBrightness;
      }
      break;
    }

    case stateFading: {
      analogWrite(ledPin, currentBrightness);
      if (t - startTime >= fadeStepTime) {
        startTime += fadeStepTime;
        int targetBrightness = mapLevelToBrightness(targetLevel);
        if (currentBrightness < targetBrightness) {
          currentBrightness += fadeStep;
          if (currentBrightness >= targetBrightness) {
            currentState = stateStable;
            stableLevel = targetLevel;
          }
        }
        if (currentBrightness > targetBrightness) {
          currentBrightness -= fadeStep;
          if (currentBrightness <= targetBrightness) {
            currentState = stateStable;
            stableLevel = targetLevel;
          }
        }
      }
      currentBrightness = max(minBrightness, currentBrightness);
      currentBrightness = min(maxBrightness, currentBrightness);
      break;
    }

    case stateMiddle21: {
      stableBrightness = mapLevelToBrightness(stableLevel);
      analogWrite(ledPin, stableBrightness);
      // if (tInMinute < startTimeInMinute) {
      //   tInMinute += 60;
      // }
      // if (tInMinute - startTimeInMinute >= 2) {
      //   currentState = stateFadingDark1;
      //   targetLevel = 0;
      //   startTime = t;
      //   currentBrightness = stableBrightness;
      // }
      if (tInSecond < startTimeInSecond) {
        tInSecond += 60;
      }
      if (tInSecond - startTimeInSecond >= 5) {
        currentState = stateFadingDark1;
        targetLevel = 0;
        fadeStepTime = fadeTime / (abs(mapLevelToBrightness(targetLevel) - mapLevelToBrightness(stableLevel)) / fadeStep);
        startTime = t;
        currentBrightness = stableBrightness;
      }
      break;
    }

    case stateMiddle22: {
      stableBrightness = mapLevelToBrightness(stableLevel);
      analogWrite(ledPin, stableBrightness);
      int isTouched = cap.touched();
      if (!(isTouched & _BV(4)) && (isLastTouched & _BV(4)) ) {
        currentState = stateFadingDark2;
        startTime = t;
        targetLevel = 0;
        fadeStepTime = fadeTime / (abs(mapLevelToBrightness(targetLevel) - mapLevelToBrightness(stableLevel)) / fadeStep);
        currentBrightness = stableBrightness;
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      // if (tInMinute < startTimeInMinute) {
      //   tInMinute += 60;
      // }
      // if (tInMinute - startTimeInMinute >= 2) {
      //   currentState = stateFadingDark1;
      //   targetLevel = 0;
      //   startTime = t;
      //   currentBrightness = stableBrightness;
      // }
      if (tInSecond < startTimeInSecond) {
        tInSecond += 60;
      }
      if (tInSecond - startTimeInSecond >= 5) {
        currentState = stateFadingDark2;
        targetLevel = 0;
        fadeStepTime = fadeTime / (abs(mapLevelToBrightness(targetLevel) - mapLevelToBrightness(stableLevel)) / fadeStep);
        startTime = t;
        currentBrightness = stableBrightness;
      }
      break;
    }

    case stateDark1: {
      stableBrightness = mapLevelToBrightness(stableLevel);
      analogWrite(ledPin, stableBrightness);
      int isTouched = cap.touched();
      if (!(isTouched & _BV(4)) && (isLastTouched & _BV(4)) ) {
        currentState = stateFadingMiddle22;
        startTime = t;
        targetLevel = 1;
        fadeStepTime = fadeTime / (abs(mapLevelToBrightness(targetLevel) - mapLevelToBrightness(stableLevel)) / fadeStep);
        currentBrightness = stableBrightness;
        isLastTouched = 0;
        break;
      }
      isLastTouched = isTouched;
      break;
    }

    case stateDark2: {
      stableBrightness = mapLevelToBrightness(stableLevel);
      analogWrite(ledPin, stableBrightness);
      currentState = stateCountingLight2;
      break;
    }

    case stateFadingMiddle21: {
      analogWrite(ledPin, currentBrightness);
      if (t - startTime >= fadeStepTime) {
        startTime += fadeStepTime;
        int targetBrightness = mapLevelToBrightness(targetLevel);
        if (currentBrightness < targetBrightness) {
          currentBrightness += fadeStep;
          if (currentBrightness >= targetBrightness) {
            currentState = stateMiddle21;
            stableLevel = targetLevel;
            startTimeInMinute = tInMinute;
            startTimeInSecond = tInSecond;
          }
        }
        if (currentBrightness > targetBrightness) {
          currentBrightness -= fadeStep;
          if (currentBrightness <= targetBrightness) {
            currentState = stateMiddle21;
            stableLevel = targetLevel;
            startTimeInMinute = tInMinute;
            startTimeInSecond = tInSecond;
          }
        }
      }
      currentBrightness = max(minBrightness, currentBrightness);
      currentBrightness = min(maxBrightness, currentBrightness);
      break;
    }

    case stateFadingMiddle22: {
      analogWrite(ledPin, currentBrightness);
      if (t - startTime >= fadeStepTime) {
        startTime += fadeStepTime;
        int targetBrightness = mapLevelToBrightness(targetLevel);
        if (currentBrightness < targetBrightness) {
          currentBrightness += fadeStep;
          if (currentBrightness >= targetBrightness) {
            currentState = stateMiddle22;
            stableLevel = targetLevel;
            startTimeInMinute = tInMinute;
            startTimeInSecond = tInSecond;
          }
        }
        if (currentBrightness > targetBrightness) {
          currentBrightness -= fadeStep;
          if (currentBrightness <= targetBrightness) {
            currentState = stateMiddle22;
            stableLevel = targetLevel;
            startTimeInMinute = tInMinute;
            startTimeInSecond = tInSecond;
          }
        }
      }
      currentBrightness = max(minBrightness, currentBrightness);
      currentBrightness = min(maxBrightness, currentBrightness);
      break;
    }

    case stateFadingDark1: {
      analogWrite(ledPin, currentBrightness);
      if (t - startTime >= fadeStepTime) {
        startTime += fadeStepTime;
        int targetBrightness = mapLevelToBrightness(targetLevel);
        if (currentBrightness < targetBrightness) {
          currentBrightness += fadeStep;
          if (currentBrightness >= targetBrightness) {
            currentState = stateDark1;
            stableLevel = targetLevel;
          }
        }
        if (currentBrightness > targetBrightness) {
          currentBrightness -= fadeStep;
          if (currentBrightness <= targetBrightness) {
            currentState = stateDark1;
            stableLevel = targetLevel;
          }
        }
      }
      currentBrightness = max(minBrightness, currentBrightness);
      currentBrightness = min(maxBrightness, currentBrightness);
      break;
    }

    case stateFadingDark2: {
      analogWrite(ledPin, currentBrightness);
      if (t - startTime >= fadeStepTime) {
        startTime += fadeStepTime;
        int targetBrightness = mapLevelToBrightness(targetLevel);
        if (currentBrightness < targetBrightness) {
          currentBrightness += fadeStep;
          if (currentBrightness >= targetBrightness) {
            currentState = stateDark2;
            stableLevel = targetLevel;
          }
        }
        if (currentBrightness > targetBrightness) {
          currentBrightness -= fadeStep;
          if (currentBrightness <= targetBrightness) {
            currentState = stateDark2;
            stableLevel = targetLevel;
          }
        }
      }
      currentBrightness = max(minBrightness, currentBrightness);
      currentBrightness = min(maxBrightness, currentBrightness);
      break;
    }

    case stateCountingLight2: {
      stableBrightness = mapLevelToBrightness(stableLevel);
      analogWrite(ledPin, stableBrightness);
      targetLevel = 0;
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      if (level != targetLevel) {
        startTime = t;
      }
      if (t - startTime >= countingTime) {
        currentState = stateCountingLight0;
        startTime = t;
      }
      break;
    }

    case stateCountingLight0: {
      stableBrightness = mapLevelToBrightness(stableLevel);
      analogWrite(ledPin, stableBrightness);
      targetLevel = 2;
      int light = analogRead(lightSensorPin);
      int level = mapLightToLevel(light);
      if (level != targetLevel) {
        startTime = t;
      }
      if (t - startTime >= countingTime) {
        currentState = stateFading;
        startTime = t;
        fadeStepTime = fadeTime / (abs(mapLevelToBrightness(targetLevel) - mapLevelToBrightness(stableLevel)) / fadeStep);
        currentBrightness = stableBrightness;
      }
      break;
    }
  }
}
