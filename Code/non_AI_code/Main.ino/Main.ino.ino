#include "WiFiEsp.h"
#include "secrets.h"
#include "ThingSpeak.h"

int S1 = 0;
int S2 = 0;
int S3 = 0;
int S4 = 0;
int tp = 0;

char ssid[] = SECRET_SSID;   // your network SSID (name)
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
WiFiEspClient client;

// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#define ESP_BAUDRATE 19200
#else
#define ESP_BAUDRATE 115200
#endif

// Counting channel details
unsigned long counterChannelNumber = SECRET_CH_ID_COUNTER;
const char * myCounterReadAPIKey = SECRET_READ_APIKEY_COUNTER;
unsigned int counterFieldNumber1 = 1;
unsigned int counterFieldNumber2 = 2;
unsigned int counterFieldNumber3 = 3;
unsigned int counterFieldNumber4 = 4;

#define ADC_VREF_mV 5000.0 // in millivolt
#define ADC_RESOLUTION 1024.0
#define PIN_LM35 A0 // pin connected to LM35 temperature sensor

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x20,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const int Relay0 = 2, Relay1 = 3, Relay2 = 4, Relay3 = 5;
const int Rf0 = 8, Rf1 = 9, Rf2 = 10, Rf3 = 11;
const int motion_sensor = 12;

int x = 0, y = 0, z = 0, j = 0, k = 0, fon = 0, foff = 0;
int motion_data, Rf0_Read, Rf1_Read, Rf2_Read, Rf3_Read;
int RL0_Status, RL1_Status, RL2_Status, RL3_Status;
float tempC;

// Function to update LCD with a message on a specific row
void updateLCD(String message, int row) {
    lcd.setCursor(0, row);
    lcd.print(message);
}

void setup() {
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Home Automation");
    lcd.setCursor(2, 1);
    lcd.print("System By");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Samson Ajadalu");
    lcd.setCursor(0, 1);
    lcd.print("ENG1704075");
    delay(2000);

    pinMode(Relay0, OUTPUT);
    pinMode(Relay1, OUTPUT);
    pinMode(Relay2, OUTPUT);
    pinMode(Relay3, OUTPUT);
    pinMode(Rf0, INPUT);
    pinMode(Rf1, INPUT);
    pinMode(Rf2, INPUT);
    pinMode(Rf3, INPUT);
    pinMode(motion_sensor, INPUT);

    digitalWrite(Relay0, HIGH);
    digitalWrite(Relay1, HIGH);
    digitalWrite(Relay2, HIGH);
    digitalWrite(Relay3, HIGH);

    lcd.clear();
    updateLCD("Initialing...", 0);
    Serial.begin(115200);
/* 
   Note: 
  Remove the comment markers if you want to use this code with a real physical device like the ATmega328P.
  This section initializes the WiFi module and the serial communication, which is needed for real-world applications.
  The physical version of this project was created using the ATmega328P chip.
  
  // Initialize Serial for Leonardo native USB port, or if using Serial1
  while(!Serial){
    ; // Wait for serial port to connect. Needed for Leonardo native USB port only
 }

  // Set the baud rate for the ESP8266 module, which communicates with the ATmega328P
  setEspBaudRate(ESP_BAUDRATE);

  // Wait for the serial connection to be available
  while (!Serial ) {
    ; // Wait for serial port to connect. This is critical for Leonardo or native USB port devices
  }

  Serial.print("Searching for ESP8266..."); 
  // Initialize the ESP8266 WiFi module
  WiFi.init(&Serial1);

  // Check if the ESP8266 WiFi shield/module is present
  if (WiFi.status() == WL_NO_SHIELD )
  {
    Serial.println("WiFi shield not present");
    k = 1;  // Set error flag
  }
  else
  {
    Serial.println("found it!");  
    ThingSpeak.begin(client);  // Initialize ThingSpeak communication using the WiFi client
  }
  */


    lcd.setCursor(0, 1);
    lcd.print(k);
    delay(500);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RoomTemp:     `C");
    tp = 10;
}

void loop() {
    z++;
    j++;
    fon++;
    foff++;

    if (tp == 10) {
        int adcVal = analogRead(PIN_LM35);
        float milliVolt = adcVal * (ADC_VREF_mV / ADC_RESOLUTION);
        tempC = milliVolt / 10;
        lcd.setCursor(9, 0);
        lcd.print(tempC);
        tp = 0;
    }

    motion_data = digitalRead(motion_sensor);
    Rf0_Read = digitalRead(Rf0);
    Rf1_Read = digitalRead(Rf1);
    Rf2_Read = digitalRead(Rf2);
    Rf3_Read = digitalRead(Rf3);

    RL0_Status = digitalRead(Relay0);
    RL1_Status = digitalRead(Relay1);
    RL2_Status = digitalRead(Relay2);
    RL3_Status = digitalRead(Relay3);

    // Motion sensor logic
    if (motion_data == HIGH && z >= 100) {
        updateLCD("MOTION DETECTED", 1);
        digitalWrite(Relay0, LOW);
        delay(500);
        z = 0;
    }

    if (motion_data == LOW && j >= 500) {
        updateLCD("INACTIVITY", 1);
        digitalWrite(Relay0, HIGH);
        delay(500);
        j = 0;
    }

    // Temperature-based fan control
    if (tempC <= 20 && foff >= 1000) {
        updateLCD("TEMP TOO LOW", 1);
        delay(500);
        updateLCD("FAN OFF", 1);
        digitalWrite(Relay1, HIGH);
        foff = 0;
    }

    if (tempC >= 35 && fon >= 1000) {
        updateLCD("TEMP TOO HIGH", 1);
        delay(500);
        updateLCD("FAN ON", 1);
        digitalWrite(Relay1, LOW);
        fon = 0;
    }

    // RF Control Logic
    handleRelayControl(Rf0_Read, Relay0, "LIGHTS ON", "LIGHTS OFF");
    handleRelayControl(Rf1_Read, Relay1, "FAN ON", "FAN OFF");
    handleRelayControl(Rf2_Read, Relay2, "LOAD1 ON", "LOAD1 OFF");
    handleRelayControl(Rf3_Read, Relay3, "LOAD2 ON", "LOAD2 OFF");

    if (x == 4000 && k == 0) {
        updateLCD("Updating...", 1);
        x = 0;
        /* 
    Note: 
  Remove the comment markers to enable this code for real-world applications.
  This code connects to WiFi and retrieves data from ThingSpeak. It's designed to work with physical devices, 
  such as the ATmega328P chip used in the physical version of this project.
    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(SECRET_SSID);  // Replace with your network SSID
      while(WiFi.status() != WL_CONNECTED){
        // Connect to the network using the SSID and password. 
        // If using open or WEP networks, modify the WiFi.begin() line accordingly.
        WiFi.begin(ssid, pass); 
        Serial.print(".");
        delay(5000);  // Delay to allow connection attempts
      } 
      Serial.println("\nConnected");
    }

    // Read data from ThingSpeak: Field 1 (Assume this is a counter channel)
    long count1 = ThingSpeak.readLongField(counterChannelNumber, counterFieldNumber1, myCounterReadAPIKey);  
    statusCode = ThingSpeak.getLastReadStatus();
    if(statusCode == 200){
      Serial.println("Counter 1: " + String(count1));
      S1 = count1;  // Save the count to S1
    } else {
      Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
    }

    // Repeat for additional fields (Field 2, Field 3, Field 4) as needed
    long count2 = ThingSpeak.readLongField(counterChannelNumber, counterFieldNumber2, myCounterReadAPIKey);  
    statusCode = ThingSpeak.getLastReadStatus();
    if(statusCode == 200){
      Serial.println("Counter 2: " + String(count2));
      S2 = count2;
    } else {
      Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
    }

    long count3 = ThingSpeak.readLongField(counterChannelNumber, counterFieldNumber3, myCounterReadAPIKey);  
    statusCode = ThingSpeak.getLastReadStatus();
    if(statusCode == 200){
      Serial.println("Counter 3: " + String(count3));
      S3 = count3;
    } else {
      Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
    }

    long count4 = ThingSpeak.readLongField(counterChannelNumber, counterFieldNumber4, myCounterReadAPIKey);  
    statusCode = ThingSpeak.getLastReadStatus();
    if(statusCode == 200){
      Serial.println("Counter 4: " + String(count4));
      S4 = count4;
    } else {
      Serial.println("Problem reading channel. HTTP error code " + String(statusCode)); 
    }
    */

    }

    tp++;
    x++;
    delay(800);
    updateLCD("                ", 1);
}

// Function to handle relay toggle and LCD message update
void handleRelayControl(int rfRead, int relayPin, String onMsg, String offMsg) {
    if (rfRead == HIGH) {
        digitalWrite(relayPin, !digitalRead(relayPin));
        if (digitalRead(relayPin) == LOW) {
            updateLCD(onMsg, 1);
        } else {
            updateLCD(offMsg, 1);
        }
    }
}

void setEspBaudRate(unsigned long baudrate) {
    long rates[6] = {115200, 74880, 57600, 38400, 19200, 9600};

    Serial.print("Setting ESP8266 baudrate to ");
    Serial.print(baudrate);
    Serial.println("...");

    for (int i = 0; i < 6; i++) {
        Serial1.begin(rates[i]);
        delay(100);
        Serial1.print("AT+UART_DEF=");
        Serial1.print(baudrate);
        Serial1.print(",8,1,0,0\r\n");
        delay(100);
    }

    Serial1.begin(baudrate);
}
