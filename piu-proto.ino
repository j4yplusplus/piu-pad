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
  // Other test measurments per press or release
  Stats pressStats;
  Stats releaseStats;

  Panel(String n, int p, char k) : name(n), pin(p), key(k) {}
};

Panel panels[] = {
  {"TOP LEFT", 2, 'w'},
  {"CENTER", 3, 's'},
  {"TOP RIGHT", 4, 'd'},
  {"BOTTOM LEFT", 5, 'a'},
  {"BOTTOM RIGHT", 6, 'x'}
};


// ==========================================
// Main Setup and Loop
// ==========================================

void setup() {
  for (Panel &panel : panels) {
    pinMode(panel.pin, INPUT_PULLUP);
  }

  Serial.begin(115200);
  Keyboard.begin();
}

void loop() {
  readSerialInput();

  for (Panel &panel : panels) {
    handlePanel(panel);
  }
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
      } else {
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
          visualizePad();
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
      } else {
        // Looked like a press, stayed at release
        panel.releaseStats.rejectedChanges++;
      }
    }
  }
}



// ==========================================
// Test Mode Functions
// ==========================================

void visualizePad() {
  Serial.println();
  Serial.println();

  //Top left
  displayPanel(panels[0]);
  Serial.print("          ");
  //Top right
  displayPanel(panels[2]);

  Serial.println();
  Serial.println();
  Serial.print("       ");

  // Center
  displayPanel(panels[1]);

  Serial.println();
  Serial.println();

  // Bottom Left
  displayPanel(panels[3]);
  Serial.print("          ");
  // Bottom Right
  displayPanel(panels[4]);

  Serial.println();
  Serial.println();
  Serial.print("Hit pad to visualize key presses, ");
  Serial.print("or release all pads before continuing to calibration (y)");
  Serial.println();
}

void displayPanel(Panel &panel) {
  Serial.print("[");
  if (panel.stableState == LOW) {
    Serial.print(panel.name);
  }
  Serial.print("] ");
}

void startCalibration() {
  Serial.println();
  Serial.println("Starting calibration...");
  Serial.println("Hit each pad " + String(calibrationTgt) + " times.");
  Serial.println();

  // Reset the test measurments of each panel
  for (Panel &panel : panels) {
    panel.calibrationCount = 0; 
    panel.activeTest = false;

    panel.bounceStart = 0;
    panel.lastBounce = 0;
    panel.tempChanges = 0;
    panel.tempQuiet = 0;

    panel.pressStats = Stats();
    panel.releaseStats = Stats();
  }

  showCalibration();
}

void recordCalibration(Panel &panel) {
  panel.calibrationCount++;
  showCalibration();
  Serial.println();
}

void showCalibration() {
  for (Panel &panel : panels) {
    Serial.print(panel.name + ": ");
    Serial.print(panel.calibrationCount);
    Serial.print("/");
    Serial.println(calibrationTgt);
  }
}

void checkCalibration() {
  for (Panel &panel : panels) {
    if (panel.calibrationCount < calibrationTgt) {
      return;
    }
  }

  curStage = RESULTS;
  Serial.println();
  Serial.println("Calibration presses complete.");
  Serial.println();
  // Show results for each panel
  for (Panel &panel : panels) {
    Serial.println(panel.name + ": ");
    showResults(panel);    
  }

  Serial.println();
  Serial.print("Continue to Gameplay Mode? (y)");
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
  } else {
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
        visualizePad();
      }
      else if (cmd == "MODE GAME") {
        curMode = GAMEPLAY;
        curStage = VISUALIZATION;       
        Serial.println();
        Serial.println("GAMEPLAY MODE");
        Serial.println();
      }
      else if (cmd == "Y" && curMode == TEST && curStage == VISUALIZATION) {
        bool released = true;

        for (Panel &panel : panels) {
          if (panel.stableState == LOW) {
            released = false;
            break;
          }
        }

        if (released) {
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