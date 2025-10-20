

//.....................................................................................................................................Button Logic variables and pins
#define NUM_BUTTONS 3
const int buttonPins[NUM_BUTTONS] = {9, 10, 11};
const unsigned long longPressThreshold = 600;  // ms
const unsigned long debounceDelay = 50;        // ms
bool lastButtonState[NUM_BUTTONS];
unsigned long pressStartTime[NUM_BUTTONS];
bool buttonPressed[NUM_BUTTONS];
bool longPressFired[NUM_BUTTONS];



void setup() {
  Serial.begin(115200);
  // Seting up button testing initials
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonState[i] = HIGH;
    buttonPressed[i] = false;
    longPressFired[i] = false;
  }
  delay(200);
}

void loop() {
  printButton(buttonCheck());
  delay(10);
}


int buttonCheck() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool currentState = digitalRead(buttonPins[i]);

    // --- Button Pressed ---
    if (currentState == LOW && lastButtonState[i] == HIGH) {
      delay(debounceDelay);
      if (digitalRead(buttonPins[i]) == LOW) {
        pressStartTime[i] = millis();
        buttonPressed[i] = true;
        longPressFired[i] = false;
      }
    }

    // --- While Holding ---
    if (buttonPressed[i] && !longPressFired[i]) {
      unsigned long heldTime = millis() - pressStartTime[i];
      if (heldTime >= longPressThreshold) {
        longPressFired[i] = true;
        return 3 + i; // long press event immediately
      }
    }

    // --- Button Released ---
    if (currentState == HIGH && lastButtonState[i] == LOW) {
      delay(debounceDelay);
      if (digitalRead(buttonPins[i]) == HIGH && buttonPressed[i]) {
        unsigned long pressDuration = millis() - pressStartTime[i];
        buttonPressed[i] = false;

        if (!longPressFired[i] && pressDuration < longPressThreshold)
          return i; // short press
      }
    }

    lastButtonState[i] = currentState;
  }

  return -1; // no event
}


void printButton(int result){
  // int result = buttonCheck();
  if (result != -1) {
    int btn = result % 3;
    bool isLong = result >= 3;
    Serial.print("Button ");
    Serial.print(btn);
    Serial.print(" -> ");
    Serial.println(isLong ? "Long Press" : "Short Press");
  }
}
