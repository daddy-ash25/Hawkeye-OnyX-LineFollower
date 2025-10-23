//..................................................................................................................Button Logic variables and pins
#define NUM_BUTTONS 3
const unsigned long longPressThreshold = 600;  // ms
const unsigned long debounceDelay = 50;        // ms

//..................................................................................................................Sensor definations and declarations
#include <algorithm>

#define s0 41
#define s1 42
#define s2 2
#define s3 1
#define sensorPin 19  // Analog input
// --- Optimized multiplexer channel selector ---
const uint8_t muxPins[4] = {s0, s1, s2, s3};
int lastMuxChannel = -1; // remember the last selected channel



//Lawaris
const uint8_t sensorCount = 16;


//Calibration Struct
struct SensorCalibration {
  float minCollector[sensorCount] = {0};     // Dark baseline (max ADC to start)
  float maxCollector[sensorCount] = {4095};        // Bright baseline
  float calibratedValues[sensorCount] = {0};    // Normalized values 0-1000
  int midPoint = 2048;                           // Center threshold
  float alpha = 0.1;                             // EMA smoothing factor
  uint16_t calibrationDuration = 6000;
};

SensorCalibration sensorCal;


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
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);

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
  printCalibratedSensorValues();
  if(currentMode == 0)
    pressControl(&emberLite);
  else if(currentMode == 1)
    pressControl(&onyxSpeed);

  delay(50);
}

void pressControl(modeProfile* profile){
  // Serial.println("came to the mode select");
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
  else if(buttonStatus == 1){
    if(profile->activeSelect == 0){
      if(profile->modeNo == 0);
        speedModeStartPage();
    }
    else if(profile->activeSelect == 2)
      modeSelectPage();

    profile->currentSelect = 0;
    profile->activeSelect = 0;
    profile->currentSlide = 0;
    return;
  }
  else{
    calibrationPage();
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


void sensorCalibrate(){
  for (int i = 0; i < sensorCount; i++) {
      int temp = sensorRead(i);  // Your raw sensor reading function

      if (temp > sensorCal.midPoint) {
        sensorCal.maxCollector[i] = (1 - sensorCal.alpha) * sensorCal.maxCollector[i] + sensorCal.alpha * temp;
      } else {
        sensorCal.minCollector[i] = (1 - sensorCal.alpha) * sensorCal.minCollector[i] + sensorCal.alpha * temp;
      }
    }
}

void calibrationPage(){
  // Reinitialize min and max collectors to defaults for a fresh calibration
  for (int i = 0; i < sensorCount; i++) {
    sensorCal.maxCollector[i] = 4095;      // Bright baseline starts at 0
    sensorCal.minCollector[i] = 0;   // Dark baseline starts at max ADC
  }
  auto PrintCurrentCalibrationStatus = [](float multiplyer){
    oled.fillRoundRect(15, 10, 98, 44, 3, BLACK);
    oled.drawRoundRect(16, 11, 96, 42, 3, WHITE);
    oled.setTextColor(WHITE);              // Default text color
    oled.setFont(&Font5x7FixedMono);
    oled.setCursor(25, 23);
    oled.print("Calibrating!!");
    oled.drawLine(24, 25, 100, 25, WHITE);
    oled.fillRoundRect(18, 39, 92, 12, 2, BLACK);
    oled.drawRoundRect(18, 39, 92, 12, 2, WHITE);

    //VISUALISING HOW MUCH CALIBRATION IS DONE
    oled.fillRoundRect(20, 41, (int)(88 * multiplyer), 8, 1, WHITE);
  };
  unsigned long startTime = millis();
  unsigned long lastPrintedTime = millis();
    while ((millis() - startTime) < sensorCal.calibrationDuration){
      sensorCalibrate();
      if(millis()-lastPrintedTime > 50){
        lastPrintedTime = millis();
        PrintCurrentCalibrationStatus((float)(millis() - startTime) / sensorCal.calibrationDuration);
        oled.display();
      }
    }
  // --- Post-calibration correction for untouched sensors ---
  float avgMax = 0, avgMin = 0;
  int validMaxCount = 0, validMinCount = 0;

  // Step 1: Calculate averages for valid entries
  for (int i = 0; i < sensorCount; i++) {
    if (sensorCal.maxCollector[i] < 4094) {  // skip untouched (still 4095)
      avgMax += sensorCal.maxCollector[i];
      validMaxCount++;
    }
    if (sensorCal.minCollector[i] > 1) {  // skip untouched (still 0)
      avgMin += sensorCal.minCollector[i];
      validMinCount++;
    }
  }

  if (validMaxCount > 0) avgMax /= validMaxCount;
  if (validMinCount > 0) avgMin /= validMinCount;

  // Step 2: Replace untouched sensors with average
  for (int i = 0; i < sensorCount; i++) {
    if (sensorCal.maxCollector[i] >= 4094) {  // still default
      sensorCal.maxCollector[i] = avgMax;
    }
    if (sensorCal.minCollector[i] <= 1) {  // still default
      sensorCal.minCollector[i] = avgMin;
    }
  }



  // Debug print
  Serial.println("Calibration Done");
  Serial.println("Min Values:");
  for (int i = 0; i < sensorCount; i++) {
    Serial.print((int)sensorCal.minCollector[i]); Serial.print("\t");
  }
  Serial.println("\nMax Values:");
  for (int i = 0; i < sensorCount; i++) {
    Serial.print((int)sensorCal.maxCollector[i]); Serial.print("\t");
  }
  Serial.println();
  delay(200);
}

void speedModeStartPage(){
  int8_t buttonStatus = -1;
  while((buttonStatus != 1)&&(buttonStatus != 4)){
    buttonStatus = buttonCheck();
    oled.clearDisplay();
    oled.drawRect(0, 0, 128, 64, WHITE);
    printGraph();
    oled.display();
    delay(50);
  }
  Serial.println("speedModeStartPage");
}


void printGraph(){
  updateIRValues();
  uint8_t individualHeight = 10;
  uint8_t startYposition = 38;
  uint8_t startXposition = 5;
  // auto uint8_t Xposition = [](uint8_t input, uint8_t height){
    
  // }
  for (uint8_t i = 0; i < sensorCount; i++) {
    // uint8_t Xposition = getXposition(sensorCal.calibratedValues[i], individualHeight)
    int8_t Yposition = individualHeight * (1 - (sensorCal.calibratedValues[i] / 1000.0f));
    int8_t Xposition = (i > 7) ? (i - 1) * 8 : i * 8;
    int8_t finalHeight = individualHeight - Yposition;
    if(i==7)
      Yposition -= (individualHeight/2)+2;
    if(i==8)
      Yposition += (individualHeight/2)+3;
    oled.fillRoundRect(startXposition+Xposition, startYposition+Yposition, 6, finalHeight + 3, 2, WHITE); 
  }
}

void updateIRValues(){
  for (int i = 0; i < sensorCount; i++) {
    sensorCal.calibratedValues[i] = readCalibrated(i);
  }
}


int sensorRead(int i) {
  selectMuxChannel(i);        // Optimized multiplexer switching
  return analogRead(sensorPin);
}

void selectMuxChannel(uint8_t channel) {
  if (channel == lastMuxChannel) return;  // no change needed

  uint8_t changedBits = lastMuxChannel ^ channel;  // find which bits changed

  for (uint8_t bit = 0; bit < 4; bit++) {
    if (changedBits & (1 << bit)) {
      digitalWrite(muxPins[bit], (channel >> bit) & 1);
    }
  }

  lastMuxChannel = channel;
}

void printCalibratedSensorValues() {
  for (int i = 0; i < sensorCount; i++) {
    Serial.print(readCalibrated(i));
    Serial.print("\t");
  }
  Serial.println();
}

int readCalibrated(int i){
  int tempHolder = sensorRead(i);
  tempHolder = map(tempHolder, sensorCal.minCollector[i], sensorCal.maxCollector[i], 0, 1000);
  
  return std::clamp(tempHolder, 0, 1000);  // Clamp the result
}