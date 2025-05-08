#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HTTPSRedirect.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 4);

const char *GScriptId = "AKfycbwzWNAxzCs1j3ZqyovQWvhT0t_4dUkMLX4D5JeA84crBJpckww9MzdoHi4yTeXZLgDb";

const char* ssid = "coleen";
const char* password = "colaine08";

String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";

const char* host = "script.google.com";
const int httpsPort = 443;
const char* fingerprint = ""; // Optional: Add correct fingerprint

String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client = nullptr;

String student_id;
String first_name;
String last_name;
String program_year;
String course_code;

int blocks[] = {4, 5, 6, 8, 9};
#define total_blocks  (sizeof(blocks) / sizeof(blocks[0]))

#define RST_PIN  0  //D3
#define SS_PIN   2  //D4
#define BUZZER   15 //D2

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;

int blockNum = 2;  
byte bufferLen = 18;
byte readBlockData[18];

void setup() {
  Serial.begin(9600);        
  delay(10);
  Serial.println('\n');

  SPI.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to the");
  lcd.setCursor(0, 1);
  lcd.print("Internet...");

  WiFi.begin(ssid, password);            
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println(" ...");
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println('\n');
  Serial.println("WiFi Connected!");
  Serial.println(WiFi.localIP());

  pinMode(BUZZER, OUTPUT);

  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print("Google Sheets...");
  delay(2000);  // Reduced delay

  Serial.print("Connecting to ");
  Serial.println(host);

  bool flag = false;
  for(int i = 0; i < 5; i++){
    int retval = client->connect(host, httpsPort);
    if (retval == 1){
      flag = true;
      String msg = "Connected!";
      Serial.println(msg);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(msg);
      delay(2000);
      break;
    } else {
      Serial.println("Connection failed. Retrying...");
    }
  }
 
  if (!flag){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connection fail");
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    delay(5000);
    return;
  }

  delete client;
  client = nullptr;
}

void loop() {
  static bool flag = false;
  if (!flag){
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (client != nullptr){
    if (!client->connected()){
      int retval = client->connect(host, httpsPort);
      if (retval != 1){
        Serial.println("Disconnected. Retrying...");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Disconnected.");
        lcd.setCursor(0, 1);
        lcd.print("Retrying...");
        return;
      }
    }
  } else {
    Serial.println("Error creating client object!");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Please Scan your ID");

  mfrc522.PCD_Init();
  if (!mfrc522.PICC_IsNewCardPresent()) { return; }
  if (!mfrc522.PICC_ReadCardSerial()) { return; }

  Serial.println();
  Serial.println(F("Reading last data from RFID..."));  

  String values = "", data;
  bool readSuccess = true;

  // Authorized RFID - single beep
  authorizedBeep();

  for (byte i = 0; i < total_blocks; i++) {
    if (!ReadDataFromBlock(blocks[i], readBlockData)) {
      readSuccess = false;
      break;
    }
    if (i == 0){
      data = String((char*)readBlockData);
      data.trim();
      student_id = data;
      values = "\"" + data + ",";
    } else if (i == 1) {
      data = String((char*)readBlockData);
      data.trim();
      first_name = data;
    } else if (i == 2) {
      data = String((char*)readBlockData);
      data.trim();
      last_name = data;
      values += first_name + " " + last_name + ",";
    } else if (i == 3) {
      data = String((char*)readBlockData);
      data.trim();
      program_year = data;
    } else if (i == 4) {
      data = String((char*)readBlockData);
      data.trim();
      course_code = data;
    } else {
      data
      = String((char*)readBlockData);
      data.trim();
      values += data + ",";
    }
  }

  if (readSuccess) {
    values += data + ",";
   
    payload = payload_base + "\"" + student_id + "," + first_name + "," + last_name + "," + course_code + "," + program_year + "\"}";

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Processing Data");
    lcd.setCursor(0, 1);
    lcd.print("Please Wait...");

    Serial.println("Publishing data...");
    Serial.println(payload);

    if(client->POST(url, host, payload)){
      Serial.println("[OK] Data published.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(first_name + " " + last_name);
      lcd.setCursor(0, 1);
      lcd.print(student_id);
      lcd.setCursor(0, 2);
      lcd.print(program_year);
      lcd.setCursor(0, 3);
      lcd.print(course_code);
      
      // Make buzzer sound after displaying the data on the LCD
      authorizedBeep();
      
    } else {
      // Error during scan - long beep
      errorBeep();
      Serial.println("Error while connecting");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Error!");
      lcd.setCursor(0, 1);
      lcd.print("Please Try Again");
    }
  } else {
    // Error reading data
    errorBeep();
    Serial.println("Error reading RFID data");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error!");
    lcd.setCursor(0, 1);
    lcd.print("Please Try Again");
  }
 
  delay(5000);
}

bool ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK){
    Serial.print("Authentication failed for Read: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error!");
    lcd.setCursor(0, 1);
    lcd.print("Please Try Again");
    return false;
  } else {
    Serial.println("Authentication success");
  }

  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error!");
    lcd.setCursor(0, 1);
    lcd.print("Please Try Again");
    return false;
  } else {
    readBlockData[16] = ' ';
    readBlockData[17] = ' ';
    Serial.println("Block was read successfully");  
  }

  return true;
}

void authorizedBeep() {
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);
  delay(200);
}

void errorBeep() {
  digitalWrite(BUZZER, HIGH);
  delay(1000);
  digitalWrite(BUZZER, LOW);
  delay(1000);
}
