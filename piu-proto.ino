#include <Keyboard.h>

bool testMode = false;

// Will be utilized if debouncing is needed. For now, redundant.
int topLeft = HIGH;
int center = HIGH;
int topRight = HIGH;
int bottomLeft = HIGH;
int bottomRight = HIGH;

// Prev states
int topLeftPrev = HIGH;
int centerPrev = HIGH;
int topRightPrev = HIGH;
int bottomLeftPrev = HIGH;
int bottomRightPrev = HIGH;

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

  detectPress(2, 'w', topLeft, topLeftPrev);
  detectPress(3, 's', center, centerPrev);
  detectPress(4, 'd', topRight, topRightPrev);
  detectPress(5, 'a', bottomLeft, bottomLeftPrev);
  detectPress(6, 'x', bottomRight, bottomRightPrev);
}

void detectPress (int pin, char key, int &state, int &statePrev) {
  state = digitalRead(pin);

  if (state != statePrev) {

    if (testMode == true) {

      Serial.print("Pin ");
      Serial.print(pin);
      Serial.print(" changed to: ");
      Serial.print(state == LOW ? "PRESSED" : "RELEASED");
      Serial.print(" at ");
      Serial.print(micros());
      Serial.println(" us");

    } else {

      if (state == LOW) {
        Keyboard.press(key);
      } else {
        Keyboard.release(key);
      }

    }

    statePrev = state;
  }
}