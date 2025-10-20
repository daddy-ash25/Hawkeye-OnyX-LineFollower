

//.....................................................................................................................................Button Logic variables and pins
#define NUM_BUTTONS 3
const int buttonPins[NUM_BUTTONS] = {9, 10, 11};
const unsigned long longPressThreshold = 600;  // ms
const unsigned long debounceDelay = 50;        // ms
bool lastButtonState[NUM_BUTTONS];
unsigned long pressStartTime[NUM_BUTTONS];
bool buttonPressed[NUM_BUTTONS];
bool longPressFired[NUM_BUTTONS];



//..............................................................................................................................Display Variablesa and initialisatiuons
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define OLED width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// Create an SSD1306 display object
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// I2C Pins for ESP32
#define OLED_SDA 20
#define OLED_SCL 21



void setup() {
  Serial.begin(115200);
  //.................................................................................................................................Seting up button logic initials
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonState[i] = HIGH;
    buttonPressed[i] = false;
    longPressFired[i] = false;
  }

  //...................................................................................................................................Start I2C communication
  Wire.begin(OLED_SDA, OLED_SCL);
  // Initialize OLED display
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println(F("SSD1306 allocation failed"));
      for (;;); // Halt execution
  }
  // Clear the display
  oled.clearDisplay();
  printLockScreen();


  delay(200);
}

void loop() {
  printButton(buttonCheck());
  delay(10);
}



//..........................................................................................................................................Button Checking Function
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


//..........................................................................................................................................Button Printing Function
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


void printLockScreen(){
  oled.setTextColor(SSD1306_WHITE);

    // Big bold heading
    oled.setTextSize(2);
    oled.setCursor(10, 5);
    oled.println("HELLO!");

    // Smaller subheading
    oled.setTextSize(1);
    oled.setCursor(15, 28);
    oled.println("Welcome to the");

    oled.setCursor(15, 40);
    oled.println("JUNGLE :)");

    // Draw a small rectangle around text for style
    oled.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);

    oled.display();
}

