#include <Keyboard.h>

bool testMode = true;

// Amount of time needed for reading to be accepted as stable
const unsigned long debounceDelay = 20;
// For easier testing
int count = 0;

// Stable debounced states
int topLeft = HIGH;
int center = HIGH;
int topRight = HIGH;
int bottomLeft = HIGH;
int bottomRight = HIGH;

// Previous raw electrical readings
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

  Serial.begin(9600);
  Keyboard.begin();
}


void loop() {

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

      if (testMode) {

        Serial.print("Pin ");
        Serial.print(pin);
        Serial.print(" changed to: ");
        Serial.print(stableState == LOW ? "PRESSED" : "RELEASED");
        Serial.print(" at ");
        Serial.print(micros());
        Serial.println(" us ");
        Serial.println(count);

        if (stableState == LOW) {count++;}

      } else {

        if (stableState == LOW) {
          Keyboard.press(key);
        } else {
          Keyboard.release(key);
        }

      }
    }
  }
}