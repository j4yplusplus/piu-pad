#include <Keyboard.h>


String cmd = "";

enum Mode {GAMEPLAY, TEST};
// Default mode
Mode curMode = GAMEPLAY;

// Amount of time needed for reading to be accepted as stable
const unsigned long debounceDelay = 20;

// Test stages: Either visualization, calibration, or results
// When true, visualization stage
bool awaitingCalibration = false;
// When true, calibration stage
bool calibrationActive = false;
// When true, results stage. Automatically triggered when calibration is finished.
bool awaitingGameplay = false;

// Amount of samples needed per panel for calibration
int calibrationTgt = 10;


// ==========================================
// Panels
// ==========================================

struct Panel {
  int pin;
  char key;

  // Current accepted state (LOW/HIGH)
  int stableState;
  // Previous raw electrical reading
  int prevReading;
  // Measuring time for debounce delay
  unsigned long debounceTimer;

  // Current amount of samples collected for calibration
  int calibrationCount;

  // This is true from when the panel is pressed to when it's released as long 
  // as calibrationCount < calibrationTgt
  bool activeTest;

  // First bounce before stable reading
  unsigned long bounceStart;
  // Latest bounce
  unsigned long lastBounce;

  // Press bounce stats
  unsigned long pressTotal;
  unsigned long pressMax;
  int tempPressChanges;
  int pressGlitches;

  // Release bounce stats
  unsigned long releaseTotal;
  unsigned long releaseMax;
  int tempReleaseChanges;
  unsigned long releaseQuiet;
  int releaseGlitches;
  unsigned long pressQuiet;

  // Number of state changes before stable
  int tempChanges;
  // Current max quiet period between temporary state changes
  unsigned long tempQuiet;
};

Panel topLeft = {2, 'w', HIGH, HIGH, 0, 0, false, 0, 0 , 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0};
Panel center = {3, 's', HIGH, HIGH, 0, 0, false, 0, 0 , 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0};
Panel topRight = {4, 'd', HIGH, HIGH, 0, 0, false, 0, 0 , 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0};
Panel bottomLeft = {5, 'a', HIGH, HIGH, 0, 0, false, 0, 0 , 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0};
Panel bottomRight = {6, 'x', HIGH, HIGH, 0, 0, false, 0, 0 , 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0};


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

  // If the current raw reading is different from the previous raw reading
  if (curReading != panel.prevReading) {

    if (calibrationActive && panel.calibrationCount < calibrationTgt) {
      // Record time to asign to either first or last bounce
      unsigned long curTime = micros();
      // If test is not yet active, this is the first bounce.
      if (!panel.activeTest) {
        panel.activeTest = true;
        panel.bounceStart = curTime;
        panel.tempChanges = 0;
        panel.tempQuiet = 0;
      }
      else {
        // Measure quiet period between current state change and last state change.
        unsigned long quietTime = curTime - panel.lastBounce;
        // If it's larger than our current max quiet period, update.
        if (quietTime > panel.tempQuiet) {
          panel.tempQuiet = quietTime;
        }
      }
      panel.lastBounce = curTime;
      panel.tempChanges++;
    }
    panel.debounceTimer = millis();
    panel.prevReading = curReading;
  }

  // If the current raw reading has been stable for longer than the debounce delay
  if (millis() - panel.debounceTimer >= debounceDelay) {

    // If the current stable state is different from the last stable state
    if (curReading != panel.stableState) {
      // Current reading is accepted as stable state change
      panel.stableState = curReading;
      // Handle state change for gameplay (keyboard press)
      if (curMode == GAMEPLAY) {
        if (panel.stableState == LOW) {
          Keyboard.press(panel.key);
        } else {
          Keyboard.release(panel.key);
        }
      }
      // Handle state change for testing (visualization or calibration)
      if (curMode == TEST) {
        if (awaitingCalibration) {
          visualizePanels();
        } 
        else if (calibrationActive && panel.calibrationCount < calibrationTgt) {
          // Reading is now stable, so test can be ended and results recorded
          if (panel.activeTest) {
            recordBounce(panel);
            panel.activeTest = false;
          }
          // On release, increment and check to see if calibration should be ended
          if (panel.stableState == HIGH) {
            recordCalibration(panel);
            checkCalibration();
          }
        }
      }
    }

    // If the current stable state is the same as the previous stable state
    else if (panel.activeTest) {
      panel.activeTest = false;
      if (panel.stableState == LOW) {
        // Looked like a release, stayed at a press
        panel.pressGlitches++;
      }
      else {
        // Looked like a press, stayed at release
        panel.releaseGlitches++;
      }
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

  panel.bounceStart = 0;
  panel.lastBounce = 0;

  panel.pressTotal = 0;
  panel.pressMax = 0;
  panel.tempPressChanges = 0;

  panel.releaseTotal = 0;
  panel.releaseMax = 0;
  panel.tempReleaseChanges = 0;

  panel.tempChanges = 0;
  panel.pressGlitches = 0;
  panel.releaseGlitches = 0;

  panel.tempQuiet = 0;
  panel.pressQuiet = 0;
  panel.releaseQuiet= 0;
}

// TODO: Fix this later
void recordCalibration(Panel &panel) {
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
    if (topLeft.calibrationCount == calibrationTgt &&
    topRight.calibrationCount == calibrationTgt &&
    center.calibrationCount == calibrationTgt &&
    bottomLeft.calibrationCount == calibrationTgt &&
    bottomRight.calibrationCount == calibrationTgt) {

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
    panel.pressTotal += bounceDuration;
    if (bounceDuration > panel.pressMax) {
      panel.pressMax = bounceDuration;
    }
    panel.tempPressChanges += panel.tempChanges;
    if (panel.tempQuiet > panel.pressQuiet) {
      panel.pressQuiet = panel.tempQuiet;
    }
  }
  else {
    panel.releaseTotal += bounceDuration;
    if (bounceDuration > panel.releaseMax) {
      panel.releaseMax = bounceDuration;
    }
    panel.tempReleaseChanges += panel.tempChanges;
    if (panel.tempQuiet > panel.releaseQuiet) {
      panel.releaseQuiet = panel.tempQuiet;
    }
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

  Serial.print("  Longest quiet period: ");
  Serial.print(panel.pressQuiet / 1000.0, 1);
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

  Serial.print("  Longest quiet period: ");
  Serial.print(panel.releaseQuiet / 1000.0, 1);
  Serial.println(" ms");

  Serial.print("  Avg raw state transitions: ");
  Serial.println(avgReleaseBounces, 1);

  Serial.println();
  Serial.print("Number of Rejected Releases: ");
  Serial.println(panel.pressGlitches);
  Serial.print("Number of Rejected Presses: ");
  Serial.println(panel.releaseGlitches);

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