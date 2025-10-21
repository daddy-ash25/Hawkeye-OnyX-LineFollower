//..................................................................................................................Button Logic variables and pins
#define NUM_BUTTONS 3
const unsigned long longPressThreshold = 600;  // ms
const unsigned long debounceDelay = 50;        // ms

// --- Struct for button data ---
struct Button {
  int pin;
  bool lastState;
  bool pressed;
  bool longPressFired;
  unsigned long pressStartTime;
};

// --- Button array ---
Button buttons[NUM_BUTTONS] = {
  {9, HIGH, false, false, 0},
  {10, HIGH, false, false, 0},
  {11, HIGH, false, false, 0}
};


//............................................................................................................Display Variables and fonts
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/Font5x7Fixed.h>
#include <Fonts/Font5x7FixedMono.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>



#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// I2C Pins for ESP32
#define OLED_SDA 20
#define OLED_SCL 21


// Modes and Ui
int8_t currentMode = -1;



//........................................................................................................................................................SETUP
void setup() {
  Serial.begin(115200);

  // --- Setup buttons ---
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }

  // --- Start I2C and OLED ---
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  oled.clearDisplay();
  oled.display();\


  // add your lock screen image or text here
  printLockScreen();

  while(buttonCheck() == -1){
    delay(50);
  }


  // Mode select layout design
  auto displayLayout = []() {
      oled.clearDisplay();                    // Clear screen before drawing
      oled.drawRect(0, 0, 128, 64, WHITE);   // Outline
      oled.drawRoundRect(0, 0, 128, 64, 4, WHITE); // Rounded outline
      oled.setTextColor(SSD1306_WHITE);
      oled.setFont(&Font5x7FixedMono);
      oled.setCursor(25, 10);
      oled.print("Select Mode :");
      oled.drawLine(24, 12, 100, 12, SSD1306_WHITE);
      oled.fillRoundRect(3, 16, 39, 45, 3, SSD1306_WHITE);
      oled.fillRoundRect(44, 16, 40, 45, 3, SSD1306_WHITE);
      oled.fillRoundRect(86, 16, 39, 45, 3, SSD1306_WHITE);
      oled.fillRoundRect(5, 18, 35, 28, 2, SSD1306_BLACK);
      oled.fillRoundRect(46, 18, 36, 28, 2, SSD1306_BLACK);
      oled.fillRoundRect(88, 18, 35, 28, 2, SSD1306_BLACK);
      oled.setTextColor(SSD1306_BLACK);
      oled.setCursor(10, 56);
      oled.print("Lite");
      oled.setCursor(49, 56);
      oled.print("Speed");
      oled.setCursor(95, 56);
      oled.print("TBM");
      oled.display();
  };
  // Setting up the initial mode
  while (true) {
      displayLayout();             // Call the lambda

      uint8_t temp = buttonCheck();
      if (temp != -1) {
          if (temp == 0 || temp == 3) {
              currentMode = 0;
              break;
          }
          else if (temp == 1 || temp == 4) {
              currentMode = 1;
              break;
          }
          else continue; 
      }

      delay(50);
  }
  delay(200);
}



//.........................................................................................................................................................LOOP
void loop() {
  int result = buttonCheck();
  printButton(result);
  if(currentMode == 0)
    litePressControl();
  else if(currentMode == 1)
    speedPressControl();

  delay(10);
}



//..................................................................................................................Button Checking Function
int buttonCheck() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool currentState = digitalRead(buttons[i].pin);

    // --- Button Pressed ---
    if (currentState == LOW && buttons[i].lastState == HIGH) {
      delay(debounceDelay);
      if (digitalRead(buttons[i].pin) == LOW) {
        buttons[i].pressStartTime = millis();
        buttons[i].pressed = true;
        buttons[i].longPressFired = false;
      }
    }

    // --- While Holding ---
    if (buttons[i].pressed && !buttons[i].longPressFired) {
      unsigned long heldTime = millis() - buttons[i].pressStartTime;
      if (heldTime >= longPressThreshold) {
        buttons[i].longPressFired = true;
        return 3 + i; // long press event immediately
      }
    }

    // --- Button Released ---
    if (currentState == HIGH && buttons[i].lastState == LOW) {
      delay(debounceDelay);
      if (digitalRead(buttons[i].pin) == HIGH && buttons[i].pressed) {
        unsigned long pressDuration = millis() - buttons[i].pressStartTime;
        buttons[i].pressed = false;

        if (!buttons[i].longPressFired && pressDuration < longPressThreshold)
          return i; // short press
      }
    }

    buttons[i].lastState = currentState;
  }

  return -1; // no event
}



//..................................................................................................................Button Printing Function
void printButton(int result) {
  if (result != -1) {
    int btn = result % NUM_BUTTONS;
    bool isLong = result >= 3;
    Serial.print("Button ");
    Serial.print(btn);
    Serial.print(" -> ");
    Serial.println(isLong ? "Long Press" : "Short Press");
  }
}



//..................................................................................................................Lock Screen Display Function
void printLockScreen() {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  oled.setTextSize(2);
  oled.setCursor(10, 5);
  oled.println("HELLO!");

  oled.setTextSize(1);
  oled.setCursor(15, 28);
  oled.println("Welcome to the");
  oled.setCursor(15, 40);
  oled.println("JUNGLE :)");

  oled.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  oled.display();
}
