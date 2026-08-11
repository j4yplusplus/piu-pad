#include <Keyboard.h>

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

  Keyboard.begin();
}

void loop() {
  int topLeft = digitalRead(2);
  int center = digitalRead(3);
  int topRight = digitalRead(4);
  int bottomLeft = digitalRead(5);
  int bottomRight = digitalRead(6);

  if (topLeft != topLeftPrev) {
    if (topLeft == LOW) {
      Keyboard.press('w');
    } else {
      Keyboard.release('w');
    }
    topLeftPrev = topLeft;
  }

  if (center != centerPrev) {
    if (center == LOW) {
      Keyboard.press('s');
    } else {
      Keyboard.release('s');
    }
    centerPrev = center;
  }

  if (topRight != topRightPrev) {
    if (topRight == LOW) {
      Keyboard.press('d');
    } else {
      Keyboard.release('d');
    }
    topRightPrev = topRight;
  }

  if (bottomLeft != bottomLeftPrev) {
    if (bottomLeft == LOW) {
      Keyboard.press('a');
    } else {
      Keyboard.release('a');
    }
    bottomLeftPrev = bottomLeft;
  }

  if (bottomRight != bottomRightPrev) {
    if (bottomRight == LOW) {
      Keyboard.press('x');
    } else {
      Keyboard.release('x');
    }
    bottomRightPrev = bottomRight;
  }
}