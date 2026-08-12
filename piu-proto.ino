#include <Keyboard.h>



// ==========================================
// Global Variables
// ==========================================

enum Mode {
  GAMEPLAY,
  CONTROLLED_TEST
};

// Gameplay is default mode
Mode curMode = GAMEPLAY;

String cmd = "";

// Amount of time needed for reading to be accepted as stable
const unsigned long debounceDelay = 20;



// ==========================================
// Panels
// ==========================================

struct Panel {
  int pin;
  char key;
  int stableState;
  int prevReading;
  unsigned long debounceTimer;
};

Panel topLeft = {2, 'w', HIGH, HIGH, 0};
Panel center = {3, 's', HIGH, HIGH, 0};
Panel topRight = {4, 'd', HIGH, HIGH, 0};
Panel bottomLeft = {5, 'a', HIGH, HIGH, 0};
Panel bottomRight = {6, 'x', HIGH, HIGH, 0};



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

      if (curMode == CONTROLLED_TEST) {
        modeCT();
      }
    }
  }
}


// ==========================================
// Controlled Test Mode Functions
// ==========================================

void modeCT() {
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
  Serial.print("Hit pad to visualize key presses.");
  Serial.println();
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

      if (cmd == "MODE CT") {
        Keyboard.releaseAll();
        curMode = CONTROLLED_TEST;
        Serial.println();
        Serial.println("CONTROLLED TEST MODE");
        Serial.println();
        modeCT();
      }
      else if (cmd == "MODE GAME") {
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