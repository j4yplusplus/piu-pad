#include <Keyboard.h>


String cmd = "";

enum Mode {GAMEPLAY, TEST};
enum TestStage {VISUALIZATION, CALIBRATION, RESULTS};
// Default mode and test stage
Mode curMode = GAMEPLAY;
TestStage curStage = VISUALIZATION;

// Amount of time needed for reading to be accepted as stable
const unsigned long debounceDelay = 20;

// Amount of samples needed per panel for calibration
int calibrationTgt = 10;


// ==========================================
// Panels
// ==========================================

struct Stats {
  unsigned long bounceSum = 0;
  unsigned long longestBounce = 0;
  int totalChanges = 0;
  unsigned long longestSilence = 0;
  int rejectedChanges = 0;
};

struct Panel {
  String name;
  int pin;
  char key;

  // Current accepted state (LOW/HIGH)
  int stableState = HIGH;
  // Previous raw electrical reading
  int prevReading = HIGH;
  // Measuring time for debounce delay
  unsigned long debounceTimer = 0;

  // Current amount of samples collected for calibration
  int calibrationCount = 0;

  // This is true from when the panel is pressed to when it's released as long 
  // as calibrationCount < calibrationTgt
  bool activeTest = false;

  // First bounce before stable reading
  unsigned long bounceStart = 0;
  // Latest bounce
  unsigned long lastBounce = 0;

  // Number of state changes before stable
  int tempChanges = 0;
  // Current max quiet period between temporary state changes
  unsigned long tempQuiet = 0;

  Stats pressStats;
  Stats releaseStats;

  Panel(String n, int p, char k) : name(n), pin(p), key(k) {}
};


Panel topLeft = {"TOP LEFT", 2, 'w'};
Panel center = {"CENTER", 3, 's'};
Panel topRight = {"TOP RIGHT", 4, 'd'};
Panel bottomLeft = {"BOTTOM LEFT", 5, 'a'};
Panel bottomRight = {"BOTTOM RIGHT", 6, 'x'};


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

    if (curStage == CALIBRATION && panel.calibrationCount < calibrationTgt) {
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
        if (curStage == VISUALIZATION) {
          visualizePanels();
        } 
        else if (curStage == CALIBRATION && panel.calibrationCount < calibrationTgt) {
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
        panel.pressStats.rejectedChanges++;
      }
      else {
        // Looked like a press, stayed at release
        panel.releaseStats.rejectedChanges++;
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
  panel.tempChanges = 0;
  panel.tempQuiet = 0;

  panel.pressStats = Stats();
  panel.releaseStats = Stats();
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

    curStage = RESULTS;

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
    panel.pressStats.bounceSum += bounceDuration;
    if (bounceDuration > panel.pressStats.longestBounce) {
      panel.pressStats.longestBounce = bounceDuration;
    }
    panel.pressStats.totalChanges += panel.tempChanges;
    if (panel.tempQuiet > panel.pressStats.longestSilence) {
      panel.pressStats.longestSilence = panel.tempQuiet;
    }
  }
  else {
    panel.releaseStats.bounceSum += bounceDuration;
    if (bounceDuration > panel.releaseStats.longestBounce) {
      panel.releaseStats.longestBounce = bounceDuration;
    }
    panel.releaseStats.totalChanges += panel.tempChanges;
    if (panel.tempQuiet > panel.releaseStats.longestSilence) {
      panel.releaseStats.longestSilence = panel.tempQuiet;
    }
  }
}

void showResults(Panel &panel) {
  float avgPressBounce =
    (float)panel.pressStats.bounceSum / panel.calibrationCount / 1000.0;

  float avgReleaseBounce =
    (float)panel.releaseStats.bounceSum / panel.calibrationCount / 1000.0;
  
  float avgPressBounces =
    (float)panel.pressStats.totalChanges / panel.calibrationCount;
  
  float avgReleaseBounces =
    (float)panel.releaseStats.totalChanges / panel.calibrationCount;

  // Press results
  Serial.println();
  Serial.println("Press:");

  Serial.print("  Avg bounce: ");
  Serial.print(avgPressBounce, 1);
  Serial.println(" ms");

  Serial.print("  Max bounce: ");
  Serial.print(panel.pressStats.longestBounce / 1000.0, 1);
  Serial.println(" ms");

  Serial.print("  Longest quiet period: ");
  Serial.print(panel.pressStats.longestSilence / 1000.0, 1);
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
  Serial.print(panel.releaseStats.longestBounce / 1000.0, 1);
  Serial.println(" ms");

  Serial.print("  Longest quiet period: ");
  Serial.print(panel.releaseStats.longestSilence / 1000.0, 1);
  Serial.println(" ms");

  Serial.print("  Avg raw state transitions: ");
  Serial.println(avgReleaseBounces, 1);

  Serial.println();
  Serial.print("Number of Rejected Releases: ");
  Serial.println(panel.pressStats.rejectedChanges);
  Serial.print("Number of Rejected Presses: ");
  Serial.println(panel.releaseStats.rejectedChanges);

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
        curStage = VISUALIZATION;
        Serial.println();
        Serial.println("TEST MODE");
        Serial.println();
        visualizePanels();
      }
      else if (cmd == "MODE GAME") {
        curMode = GAMEPLAY;
        curStage = VISUALIZATION;       
        Serial.println();
        Serial.println("GAMEPLAY MODE");
        Serial.println();
      }
      else if (cmd == "Y" && curMode == TEST && curStage == VISUALIZATION) {
        if(topLeft.stableState == HIGH &&
          topRight.stableState == HIGH &&
          center.stableState == HIGH &&
          bottomLeft.stableState == HIGH &&
          bottomRight.stableState == HIGH) {

          curStage = CALIBRATION;
          startCalibration();
        } else {
          Serial.println("Release all panels before starting calibration.");
        }
      }
      else if (cmd == "Y" && curStage == RESULTS) {
        curMode = GAMEPLAY;
        curStage = VISUALIZATION;
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