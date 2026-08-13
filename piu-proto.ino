#include <Keyboard.h>



// ==========================================
// Global Variables
// ==========================================

enum Mode {
  GAMEPLAY,
  TEST
};

// Gameplay is default mode
Mode curMode = GAMEPLAY;

String cmd = "";

// Amount of time needed for reading to be accepted as stable
const unsigned long debounceDelay = 20;

bool awaitingCalibration = false;
bool awaitingGameplay = false;
bool calibrationActive = false;
int calibrationTgt = 10;



// ==========================================
// Panels
// ==========================================

struct Panel {
  int pin;
  char key;
  int stableState;
  int prevReading;
  unsigned long debounceTimer;

  int calibrationCount;

  bool activeTest;
  bool finishedTest;

  // Raw bounce measurment
  unsigned long bounceStart;
  unsigned long lastBounce;

  // Press bounce stats
  unsigned long pressTotal;
  unsigned long pressMax;
  int tempPressChanges;

  // Release bounce stats
  unsigned long releaseTotal;
  unsigned long releaseMax;
  int tempReleaseChanges;

  int tempChanges;

  int glitchEvents;
};

Panel topLeft = {2, 'w', HIGH, HIGH, 0, 0, false, false, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0};
Panel center = {3, 's', HIGH, HIGH, 0, 0, false, false, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0};
Panel topRight = {4, 'd', HIGH, HIGH, 0, 0, false, false, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0};
Panel bottomLeft = {5, 'a', HIGH, HIGH, 0, 0, false, false, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0};
Panel bottomRight = {6, 'x', HIGH, HIGH, 0, 0, false, false, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0};



// ==========================================
// Main Setup and Loop
// ==========================================

void setup() {
  pinMode(topLeft.pin, INPUT_PULLUP);
  pinMode(center.pin, INPUT_PULLUP);
  pinMode(topRight.pin, INPUT_PULLUP);
  pinMode(bottomLeft.pin, INPUT_PULLUP);
  pinMode(bottomRight.pin, INPUT_PULLUP);

  Serial.begin(115200);
  Keyboard.begin();
}

void loop() {

  readSerialInput();

  handlePanel(topLeft);
  handlePanel(center);
  handlePanel(topRight);
  handlePanel(bottomLeft);
  handlePanel(bottomRight);
}



// ==========================================
// Panel Input Handling
// ==========================================

void handlePanel (Panel &panel) {
  int curReading = digitalRead(panel.pin);

  // If the current raw electrical reading is different from the previous raw reading
  if (curReading != panel.prevReading) {

    if (calibrationActive) {
      unsigned long curTime = micros();
      if (!panel.activeTest) {
        panel.activeTest = true;
        panel.bounceStart = curTime;
        panel.tempChanges = 0;
      }
      panel.lastBounce = curTime;
      // Record number of bounces
      panel.tempChanges++;
    }
    panel.debounceTimer = millis();
    panel.prevReading = curReading;
  }

  // If the current raw reading has been stable for longer than the debounce delay
  if (millis() - panel.debounceTimer >= debounceDelay) {

    // Check if the current stable state is different from the last stable state
    if (curReading != panel.stableState) {
      panel.stableState = curReading;

      if (curMode == GAMEPLAY) {
        if (panel.stableState == LOW) {
          Keyboard.press(panel.key);
        } else {
          Keyboard.release(panel.key);
        }
      }

      if (curMode == TEST) {
        if (awaitingCalibration) {
          visualizePanels();
        } 
        else if (calibrationActive) {
          if (panel.activeTest) {
            recordBounce(panel);
            panel.activeTest = false;
          }
          if (panel.stableState == LOW) {
            recordCalibration(panel);
          }
          else if (panel.stableState == HIGH) {
            checkCalibration();
          }
        }
      }
    }
    else if (panel.activeTest) {
      // Raw glitch returned to original stable state
      panel.activeTest = false;
      panel.glitchEvents++;
    }
  }
}



// ==========================================
// Test Mode Functions
// ==========================================

void visualizePanels() {
  Serial.println();
  Serial.println();
  displayPad("RED", "LEFT", topLeft);
  Serial.print("          ");
  displayPad("RED", "RIGHT", topRight);
  Serial.println();
  Serial.println();
  Serial.print("       ");
  displayPad("YELLOW", "CENTER", center);
  Serial.println();
  Serial.println();
  displayPad("BLUE", "LEFT", bottomLeft);
  Serial.print("          ");
  displayPad("BLUE", "RIGHT", bottomRight);
  Serial.println();
  Serial.println();
  Serial.print("Hit pad to visualize key presses, ");
  Serial.print("or release all pads before continuing to calibration (y)");
  Serial.println();
}

void displayPad(String color, String position, Panel &panel) {
  Serial.print("[");
  if (panel.stableState == LOW) {
    Serial.print(color);
    Serial.print(": ");
    Serial.print(position);
  }
  Serial.print("] ");
}

void startCalibration() {
  Serial.println();
  Serial.println("Starting calibration...");
  Serial.println("Hit each pad " + String(calibrationTgt) + " times.");
  Serial.println();

  resetPanel(topLeft);
  resetPanel(center);
  resetPanel(topRight);
  resetPanel(bottomLeft);
  resetPanel(bottomRight);

  showCalibration();
}

void resetPanel(Panel &panel) {
  panel.calibrationCount = 0; 
  panel.activeTest = false;
  panel.finishedTest = false;

  panel.bounceStart = 0;
  panel.lastBounce = 0;

  panel.pressTotal = 0;
  panel.pressMax = 0;
  panel.tempPressChanges = 0;

  panel.releaseTotal = 0;
  panel.releaseMax = 0;
  panel.tempReleaseChanges = 0;

  panel.tempChanges = 0;
  panel.glitchEvents = 0;
}

void recordCalibration(Panel &panel) {
  if (panel.calibrationCount == calibrationTgt) {
    return;
  }

  panel.calibrationCount++;
  showCalibration();
}

void showCalibration() {
  Serial.print("Top Left: ");
  Serial.print(topLeft.calibrationCount);
  Serial.print("/");
  Serial.println(calibrationTgt);
  
  Serial.print("Top Right: ");
  Serial.print(topRight.calibrationCount);
  Serial.print("/");
  Serial.println(calibrationTgt);
  
  Serial.print("Center: ");
  Serial.print(center.calibrationCount);
  Serial.print("/");
  Serial.println(calibrationTgt);

  Serial.print("Bottom Left: ");
  Serial.print(bottomLeft.calibrationCount);
  Serial.print("/");
  Serial.println(calibrationTgt);

  Serial.print("Bottom Right: ");
  Serial.print(bottomRight.calibrationCount);
  Serial.print("/");
  Serial.println(calibrationTgt);

  Serial.println();
}

void checkCalibration() {
    if (topLeft.finishedTest &&
    topRight.finishedTest &&
    center.finishedTest &&
    bottomLeft.finishedTest &&
    bottomRight.finishedTest) {

    calibrationActive = false;
    awaitingGameplay = true;

    Serial.println();
    Serial.println("Calibration presses complete.");
    Serial.println();

    Serial.println("TOP LEFT:");
    showResults(topLeft);
    Serial.println("TOP RIGHT:");
    showResults(topRight);
    Serial.println("CENTER:");
    showResults(center);
    Serial.println("BOTTOM LEFT:");
    showResults(bottomLeft);
    Serial.println("BOTTOM RIGHT:");
    showResults(bottomRight);
    Serial.print("Continue to Gameplay Mode? (y)");
  }
}

void recordBounce(Panel &panel) {

  unsigned long bounceDuration = panel.lastBounce - panel.bounceStart;

  if (panel.stableState == LOW) {
    if (panel.calibrationCount == calibrationTgt) {
      return;
    }
    panel.pressTotal += bounceDuration;
    if (bounceDuration > panel.pressMax) {
      panel.pressMax = bounceDuration;
    }
    panel.tempPressChanges += panel.tempChanges;
  }
  else {
    if (panel.finishedTest) {
      return;
    }
    panel.releaseTotal += bounceDuration;
    if (bounceDuration > panel.releaseMax) {
      panel.releaseMax = bounceDuration;
    }
    if (panel.calibrationCount == calibrationTgt) {
      panel.finishedTest = true;
    } 
    panel.tempReleaseChanges += panel.tempChanges;
  }
}

void showResults(Panel &panel) {
  float avgPressBounce =
    (float)panel.pressTotal / panel.calibrationCount / 1000.0;

  float avgReleaseBounce =
    (float)panel.releaseTotal / panel.calibrationCount / 1000.0;
  
  float avgPressBounces =
    (float)panel.tempPressChanges / panel.calibrationCount;
  
  float avgReleaseBounces =
    (float)panel.tempReleaseChanges / panel.calibrationCount;

  // Press results
  Serial.println();
  Serial.println("Press:");

  Serial.print("  Avg bounce: ");
  Serial.print(avgPressBounce, 1);
  Serial.println(" ms");

  Serial.print("  Max bounce: ");
  Serial.print(panel.pressMax / 1000.0, 1);
  Serial.println(" ms");

  Serial.print("  Avg raw state transitions: ");
  Serial.println(avgPressBounces, 1);

  // Release results
  Serial.println();
  Serial.println("Release:");

  Serial.print("  Avg bounce: ");
  Serial.print(avgReleaseBounce, 1);
  Serial.println(" ms");

  Serial.print("  Max bounce: ");
  Serial.print(panel.releaseMax / 1000.0, 1);
  Serial.println(" ms");

  Serial.print("  Avg raw state transitions: ");
  Serial.println(avgReleaseBounces, 1);

  Serial.println();
  Serial.print("Number of Glitch Events: ");
  Serial.println(panel.glitchEvents);

  Serial.println();
}


// ==========================================
// Serial Input Handling
// ==========================================

void readSerialInput() {
  while (Serial.available() > 0) {
    char incoming = Serial.read();

    // Command is complete on enter
    if (incoming == '\n') {
      cmd.trim();

      cmd.toUpperCase();

      if (cmd == "MODE TEST") {
        Keyboard.releaseAll();
        curMode = TEST;
        calibrationActive = false;
        awaitingCalibration = true;
        awaitingGameplay = false;
        Serial.println();
        Serial.println("TEST MODE");
        Serial.println();
        visualizePanels();
      }
      else if (cmd == "MODE GAME") {
        curMode = GAMEPLAY;
        calibrationActive = false;
        awaitingCalibration = false;
        awaitingGameplay = false;        
        Serial.println();
        Serial.println("GAMEPLAY MODE");
        Serial.println();
      }
      else if (cmd == "Y" && awaitingCalibration) {
        if(topLeft.stableState == HIGH &&
          topRight.stableState == HIGH &&
          center.stableState == HIGH &&
          bottomLeft.stableState == HIGH &&
          bottomRight.stableState == HIGH) {

          calibrationActive = true;
          awaitingCalibration = false;
          startCalibration();
        } else {
          Serial.println("Release all panels before starting calibration.");
          return;
        }
      }
      else if (cmd == "Y" && awaitingGameplay) {
        awaitingGameplay = false;
        curMode = GAMEPLAY;
        Serial.println();
        Serial.println("GAMEPLAY MODE");
        Serial.println();
      }
      else {
        Serial.print("Unknown command: ");
        Serial.println(cmd);
      }

      cmd = "";
    }

    else if (incoming != '\r') {
      cmd += incoming;
    }
  }
}