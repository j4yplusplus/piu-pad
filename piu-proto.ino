#include <Keyboard.h>


enum Mode {
  GAMEPLAY,
  CONTROLLED_TEST
}
// Gameplay is default mode
Mode curMode = GAMEPLAY;

String cmd = "";


// Amount of time needed for reading to be accepted as stable
const unsigned long debounceDelay = 20;

// Stable debounced states
int topLeft = HIGH;
int center = HIGH;
int topRight = HIGH;
int bottomLeft = HIGH;
int bottomRight = HIGH;

// Previous raw readings
int topLeftPrev = HIGH;
int centerPrev = HIGH;
int topRightPrev = HIGH;
int bottomLeftPrev = HIGH;
int bottomRightPrev = HIGH;

// Time when each reading was last changed
unsigned long timerTL = 0;
unsigned long timerC = 0;
unsigned long timerTR = 0;
unsigned long timerBL = 0;
unsigned long timerBR = 0;


void setup() {
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);

  Serial.begin(115200);
  Keyboard.begin();
}


void loop() {

  readSerialInput();

  handlePanel(2, 'w', topLeft, topLeftPrev, timerTL);
  handlePanel(3, 's', center, centerPrev, timerC);
  handlePanel(4, 'd', topRight, topRightPrev, timerTR);
  handlePanel(5, 'a', bottomLeft, bottomLeftPrev, timerBL);
  handlePanel(6, 'x', bottomRight, bottomRightPrev, timerBR);
}


void handlePanel (int pin, char key, int &stableState, int &prevReading, unsigned long &debounceTimer) {
  int curReading = digitalRead(pin);

  // If the current raw electrical reading is different from the previous raw reading
  if (curReading != prevReading) {
    debounceTimer = millis();
    prevReading = curReading;
  }

  // If the current raw reading has been stable for longer than the debounce delay
  if (millis() - debounceTimer >= debounceDelay) {

    // Check if the current stable state is different from the last stable state
    if (curReading != stableState) {
      stableState = curReading;

      if (curMode == GAMEPLAY) {
        if (stableState == LOW) {
          Keyboard.press(key);
        } else {
          Keyboard.release(key);
        }
      }

      else if (curMode == CONTROLLED_TEST) {
        Serial.print("Pin ");
        Serial.print(pin);
        Serial.print(": ");

        if (stableState == LOW) {
          Serial.println("PRESSED");
        }
        else {
          Serial.println("RELEASED");
        }
      }

    }
  }
}

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

    }

    else if (incoming != '\r') {
      cmd += incoming;
    }
  }
}