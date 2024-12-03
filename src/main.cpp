#define ERA_DEBUG
#define ERA_LOCATION_VN
#define ERA_AUTH_TOKEN "7d47f69f-b84e-4b72-af1d-84f1306caf53"

#include <Arduino.h>
#ifdef byte
#undef byte
#endif
using byte = uint8_t;

#include <SPI.h>
#include <MFRC522.h>
#include <ERa.hpp>
#include <WiFi.h>

// Pin definitions
#define SS_PIN 5
#define RST_PIN 22
#define LED_PIN 2
#define resetButton 25
#define LEDWELCOME 26
#define LEDREJECT 27
#define enterButtonPin 34
#define exitButtonPin 35

MFRC522 rfid(SS_PIN, RST_PIN);

const char ssid[] = "eoh.io";
const char pass[] = "Eoh@2020";

// Authorized RFID card
const uint8_t authorizedUID[] = {0xE0, 0x03, 0xA6, 0x19};

// Variables for counting
int enterCount = 0;
int exitCount = 0;
int lastEnterButtonState = HIGH;
int lastExitButtonState = HIGH;
int lastResetButtonState = HIGH;

// Control flags
boolean checkDeparture = false;
boolean checkStop = false;
boolean checkEnd = false;

// ERa virtual pin handlers
ERA_WRITE(V2)
{
  uint8_t value_go = param.getInt();
  checkDeparture = (value_go == 1);
  checkStop = false;
  checkEnd = false;
}

ERA_WRITE(V3)
{
  uint8_t value_stop = param.getInt();
  checkStop = (value_stop == 1);
  checkDeparture = false;
  checkEnd = false;
}

ERA_WRITE(V4)
{
  uint8_t value_end = param.getInt();
  checkEnd = (value_end == 1);
  checkDeparture = false;
  checkStop = false;
}

// Time formatting function
String getFormattedTime()
{
  unsigned long timeInSeconds = ERaMillis() / 1000;
  int hours = (timeInSeconds / 3600) % 24;
  int minutes = (timeInSeconds / 60) % 60;
  int seconds = timeInSeconds % 60;

  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
  return String(timeStr);
}
void logStudentCount(String event)
{
  String currentTime = getFormattedTime();
  Serial.print(event + " Time: ");
  Serial.println(currentTime);
  Serial.print("Enter count: ");
  Serial.println(enterCount);
  Serial.print("Exit count: ");
  Serial.println(exitCount);
}
void handleRFIDCount()
{
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
  {
    return;
  }

  Serial.print("UID tag: ");
  String content = "";
  for (size_t i = 0; i < rfid.uid.size; i++)
  {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
    content.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(rfid.uid.uidByte[i], HEX));
  }
  Serial.println();

  bool authorized = true;
  for (size_t i = 0; i < rfid.uid.size; i++)
  {
    if (rfid.uid.uidByte[i] != authorizedUID[i])
    {
      authorized = false;
      break;
    }
  }

  if (authorized)
  {
    if (checkDeparture && !checkStop)
    {
      enterCount++;
      ERa.virtualWrite(V0, enterCount);
      logStudentCount("Enter (RFID)");
      digitalWrite(LEDWELCOME, HIGH);
      digitalWrite(LEDREJECT, LOW);
      Serial.println("Welcome! Student entered by RFID");
    }
    else if (checkEnd && !checkStop)
    {
      exitCount++;
      ERa.virtualWrite(V1, exitCount);
      logStudentCount("Exit (RFID)");
      digitalWrite(LEDWELCOME, HIGH);
      digitalWrite(LEDREJECT, LOW);
      Serial.println("Goodbye! Student exited by RFID");
    }
  }
  else
  {
    Serial.println("Unauthorized RFID card");
    digitalWrite(LEDWELCOME, LOW);
    digitalWrite(LEDREJECT, HIGH);
  }

  delay(200);
  digitalWrite(LEDWELCOME, LOW);
  digitalWrite(LEDREJECT, LOW);
  rfid.PICC_HaltA();
}

void setup()
{
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(enterButtonPin, INPUT_PULLUP);
  pinMode(exitButtonPin, INPUT_PULLUP);
  pinMode(resetButton, INPUT_PULLUP);
  pinMode(LEDWELCOME, OUTPUT);
  pinMode(LEDREJECT, OUTPUT);

  digitalWrite(LEDWELCOME, LOW);
  digitalWrite(LEDREJECT, LOW);

  ERa.begin(ssid, pass);
}

void loop()
{
  ERa.run();

  // Handle RFID
  handleRFIDCount();

  // Handle button inputs
  int enterButtonState = digitalRead(enterButtonPin);
  int exitButtonState = digitalRead(exitButtonPin);
  int resetButtonState = digitalRead(resetButton);

  // Handle enter button
  if (checkDeparture && !checkStop)
  {
    if (lastEnterButtonState == HIGH && enterButtonState == LOW)
    {
      enterCount++;
      ERa.virtualWrite(V0, enterCount);
      logStudentCount("Enter (Button)");
      Serial.println("Student entered by button.");
    }
  }
  lastEnterButtonState = enterButtonState;

  // Handle exit button
  if (checkEnd && !checkStop)
  {
    if (lastExitButtonState == HIGH && exitButtonState == LOW)
    {
      exitCount++;
      ERa.virtualWrite(V1, exitCount);
      logStudentCount("Exit (Button)");
      Serial.println("Student exited by button.");
    }
  }
  lastExitButtonState = exitButtonState;

  // Handle reset button
  if (lastResetButtonState == HIGH && resetButtonState == LOW)
  {
    enterCount = 0;
    exitCount = 0;
    ERa.virtualWrite(V0, enterCount);
    ERa.virtualWrite(V1, exitCount);
    Serial.println("Reset counting people!!");
  }
  lastResetButtonState = resetButtonState;
}

