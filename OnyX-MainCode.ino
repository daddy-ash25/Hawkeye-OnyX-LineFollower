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

struct modeProfile {
  struct PIDandStats {
    float Kp;
    float Kd;
    int maxSpeed;
    int minSpeed;
    float retention;
    bool retentionAllowed;
  } pidStats;

  int8_t modeNo;
  int8_t activeSelect;
  int8_t noOfElements;
  int8_t currentSlide;
  int8_t currentSelect;
  const char* elementNames[5]; // Store up to 5 element names
};

modeProfile emberLite = {
  {0.125, 1.25, 900, 500, 0.55, true}, // PID and Stats
  0, // modeNo
  0, // ActiveSelect
  4, // number of UI elements
  0, // currentSlide
  0, // currentSelect
  {"START", "SETTING", "LITE", "C-VALUE"} // names of UI parameters
};

modeProfile onyxSpeed = {
  {0.125, 1.25, 1000, 800, 0.55, true}, // PID and Stats
  1, // modeNo
  0, // Active Select
  4, // number of UI elements
  0, // current slide
  0, // currentSelect;
  {"START", "SETTING", "SPEED", "C-VALUE"} // names of UI parameters
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

#include <Fonts/Font5x5Fixed.h>
#include <Fonts/Font4x5Fixed.h>
#include <Fonts/Font5x7Fixed.h>
#include <Fonts/Font5x7FixedMono.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>



#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define WHITE SSD1306_WHITE
#define BLACK SSD1306_BLACK


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
  oled.display();


  // add your lock screen image or text here
  printLockScreen();

  while(buttonCheck() == -1){
    delay(50);
  }

  modeSelectPage();

  
  delay(200);
}



//.........................................................................................................................................................LOOP
void loop() {
  // int result = buttonCheck();
  // printButton(result);
  if(currentMode == 0)
    pressControl(&emberLite);
  else if(currentMode == 1)
    pressControl(&onyxSpeed);

  delay(50);
}

void pressControl(modeProfile* profile){
  Serial.println("came to the mode select");
  oled.clearDisplay();
  // oled.drawRect(0, 0, 128, 64, WHITE);
  oled.fillRect(0, 0, 128, 64, WHITE);
  // oled.drawRoundRect(3, 3, 100, 58, 2, BLACK);
  oled.fillRoundRect(2, 2, 84, 60, 3, BLACK);


  oled.setTextColor(BLACK);
  oled.setFont(&Font5x5Fixed);
  char buffer[10];  // Buffer to store formatted float values

  oled.setCursor(89, 7);
  oled.setTextColor(BLACK);
  oled.setTextSize(1);

  dtostrf(profile->pidStats.Kp, 0, 4, buffer);  // Format Kp with 3 decimal places
  oled.print("KP=");
  oled.print(buffer);

  oled.setCursor(89, 15);
  dtostrf(profile->pidStats.Kd, 0, 4, buffer);  // Format Kd with 3 decimal places
  oled.print("KD=");
  oled.print(buffer);

  oled.setCursor(89, 23);
  dtostrf(profile->pidStats.Kp, 0, 4, buffer);  // Format Ki with 3 decimal places
  oled.print("KI=");
  oled.print(buffer);

  oled.setCursor(89, 31);
  oled.print("BS=");
  oled.print(profile->pidStats.minSpeed);

  oled.setCursor(89, 39);
  oled.print("MS=");
  oled.print(profile->pidStats.maxSpeed);
   

  oled.setTextColor(WHITE);
  oled.setFont(&FreeMonoBold9pt7b);
  if(profile->currentSlide == 0){
    oled.setCursor(5, 17);
    oled.print(profile->elementNames[0]);
    oled.setCursor(5, 36);
    oled.print(profile->elementNames[1]);
    oled.setCursor(5, 55);
    oled.print(profile->elementNames[2]);
  }
  else {
    oled.setCursor(5, 17);
    oled.print(profile->elementNames[1]);
    oled.setCursor(5, 36);
    oled.print(profile->elementNames[2]);
    oled.setCursor(5, 55);
    oled.print(profile->elementNames[3]);
  }

  if(profile->currentSelect == 0)
    oled.drawRoundRect(3, 4, 81, 18, 3, WHITE);
  else if(profile->currentSelect == 1)
    oled.drawRoundRect(3, 23, 81, 18, 3, WHITE);
  else
    oled.drawRoundRect(3, 42, 81, 18, 3, WHITE);

  oled.display();


  int8_t buttonStatus = buttonCheck();
  if(buttonStatus == -1) return;
  else if(buttonStatus == 0){
    profile->activeSelect--;
    profile->currentSelect--;
  }
  else if(buttonStatus == 2){
    profile->activeSelect++;
    profile->currentSelect++;
  }
  else{
    if(profile->activeSelect == 1){
      if(profile->modeNo == 0);
        speedModeStartPage()
    }
    else if(profile->activeSelect == 2)
      modeSelectPage();

    profile->currentSelect = 0;
    profile->activeSelect = 0;
    profile->currentSlide = 0;
    return;
  }

  if(profile->currentSelect<0 && profile->activeSelect < 0){
    profile->currentSelect = 2;
    profile->activeSelect = profile->noOfElements-1;
    profile->currentSlide = profile->noOfElements-3;
  }
  if(profile->currentSelect>2 && profile->activeSelect > (profile->noOfElements-1)){
    profile->currentSelect = 0;
    profile->activeSelect = 0;
    profile->currentSlide = 0;
  }
  if(profile->currentSelect<0){
    profile->currentSelect = 0;
    // profile->activeSelect--;
    profile->currentSlide--;
  }
  if(profile->currentSelect>2){
    profile->currentSelect = 2;
    // profile->activeSelect++;
    profile->currentSlide++;
  }
  if (profile->currentSlide < 0)
    profile->currentSlide = 0;
  else if (profile->currentSlide > (profile->noOfElements - 3))
    profile->currentSlide = max(0, profile->noOfElements - 3);


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



void modeSelectPage(){
  // Mode select layout design
  auto displayLayout = []() {
    oled.clearDisplay();                   // Clear screen before drawing

    // Outline
    oled.drawRect(0, 0, 128, 64, WHITE);
    oled.drawRoundRect(0, 0, 128, 64, 4, WHITE);

    oled.setTextColor(WHITE);              // Default text color
    oled.setFont(&Font5x7FixedMono);
    oled.setCursor(25, 10);
    oled.print("Select Mode :");

    // Decorative line
    oled.drawLine(24, 12, 100, 12, WHITE);

    // Mode boxes (outer white)
    oled.fillRoundRect(3, 16, 39, 45, 3, WHITE);
    oled.fillRoundRect(44, 16, 40, 45, 3, WHITE);
    oled.fillRoundRect(86, 16, 39, 45, 3, WHITE);

    // Inner black boxes
    oled.fillRoundRect(5, 18, 35, 28, 2, BLACK);
    oled.fillRoundRect(46, 18, 36, 28, 2, BLACK);
    oled.fillRoundRect(88, 18, 35, 28, 2, BLACK);

    // Text inside boxes (black)
    oled.setTextColor(BLACK);
    oled.setCursor(10, 56);
    oled.print("Lite");
    oled.setCursor(49, 56);
    oled.print("Speed");
    oled.setCursor(95, 56);
    oled.print("TBM");

    oled.display();// Update screen
  
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
}