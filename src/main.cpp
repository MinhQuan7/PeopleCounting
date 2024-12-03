#define ERA_DEBUG
#define ERA_LOCATION_VN
#define ERA_AUTH_TOKEN "7d47f69f-b84e-4b72-af1d-84f1306caf53"

#include <Arduino.h>
#include <ERa.hpp>
#include <WiFi.h>

#define LED_PIN  2
#define resetButton 25
const char ssid[] = "eoh.io";
const char pass[] = "Eoh@2020";

void logStudentCount(String event);

// Define pins for buttons
const int enterButtonPin = 34;
const int exitButtonPin = 35;

// Variables to track counts and button states
int enterCount = 0;
int exitCount = 0;
int lastEnterButtonState = HIGH;
int lastExitButtonState = HIGH;
int lastResetButtonState = HIGH;

//___________Boolean check________________
boolean checkDeparture = false; // V2: allow counting up
boolean checkStop = false;      // V3: disallow any counting
boolean checkEnd = false;       // V4: allow counting down

ERA_WRITE(V2) {
    uint8_t value_go = param.getInt();
    checkDeparture = (value_go == 1);
    checkStop = false;
    checkEnd = false;
}

ERA_WRITE(V3) {
    uint8_t value_stop = param.getInt();
    checkStop = (value_stop == 1);
    checkDeparture = false;
    checkEnd = false; 
}

ERA_WRITE(V4) {
    uint8_t value_end = param.getInt();
    checkEnd = (value_end == 1);
    checkDeparture = false;
    checkStop = false;
}

// Hàm lấy thời gian định dạng từ ERa
String getFormattedTime() {
    unsigned long timeInSeconds = ERaMillis() / 1000;
    int hours = (timeInSeconds / 3600) % 24;
    int minutes = (timeInSeconds / 60) % 60;
    int seconds = timeInSeconds % 60;
    
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
    return String(timeStr);
}

void timerEvent() {
    ERA_LOG("Timer", "Uptime: %d", ERaMillis() / 1000L);
}

void setup() {
    Serial.begin(115200);
    pinMode(enterButtonPin, INPUT_PULLUP);
    pinMode(exitButtonPin, INPUT_PULLUP);
    pinMode(resetButton, INPUT_PULLUP);
    ERa.begin(ssid, pass);
}

void loop() {
    ERa.run();

    // Read button states
    int enterButtonState = digitalRead(enterButtonPin);
    int exitButtonState = digitalRead(exitButtonPin);
    int resetButtonState = digitalRead(resetButton);

    // Kiểm tra nút nhấn lên và điều kiện V2, V3
    if (checkDeparture && !checkStop) {  // V2 == 1 và V3 == 0
        if (lastEnterButtonState == HIGH && enterButtonState == LOW) {
            enterCount++;
            ERa.virtualWrite(V0, enterCount);
            Serial.println("Student entered bus.");
            logStudentCount("Enter");
        }
    }
    lastEnterButtonState = enterButtonState;

    // Kiểm tra nút nhấn xuống và điều kiện V4, V3
    if (checkEnd && !checkStop) {  // V4 == 1 và V3 == 0
        if (lastExitButtonState == HIGH && exitButtonState == LOW) {
            exitCount++;
            ERa.virtualWrite(V1, exitCount);
            Serial.println("Student exited bus.");
            logStudentCount("Exit");
        }
    }
    lastExitButtonState = exitButtonState;

    // Detect rising edge for reset button
    if (lastResetButtonState == HIGH && resetButtonState == LOW) {
        enterCount = 0;
        exitCount = 0;
        ERa.virtualWrite(V0, enterCount);
        ERa.virtualWrite(V1, exitCount);
        Serial.println("Reset counting people!!");
    }
    lastResetButtonState = resetButtonState;
}

// Function to log student count with timestamp
void logStudentCount(String event) {
    String currentTime = getFormattedTime();
    Serial.print(event + " Time: ");
    Serial.println(currentTime);
    Serial.print("Enter count: ");
    Serial.println(enterCount);
    Serial.print("Exit count: ");
    Serial.println(exitCount);
}