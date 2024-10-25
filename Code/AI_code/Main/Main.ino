#include "WiFiEsp.h"
#include "secrets.h"
#include "ThingSpeak.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoHttpClient.h>

// Network credentials
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// ESP8266 communication
WiFiEspClient espClient;
HttpClient client = HttpClient(espClient, "192.168.x.x", 5000);  // Replace with Flask server IP and port

#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7);  // RX, TX for SoftwareSerial
#define ESP_BAUDRATE 19200
#else
#define ESP_BAUDRATE 115200
#endif

// ThingSpeak credentials
unsigned long counterChannelNumber = SECRET_CH_ID_COUNTER;
const char *myCounterReadAPIKey = SECRET_READ_APIKEY_COUNTER;
unsigned int counterFieldNumber1 = 1;
unsigned int counterFieldNumber2 = 2;
unsigned int counterFieldNumber3 = 3;
unsigned int counterFieldNumber4 = 4;

#define ADC_VREF_mV 5000.0
#define ADC_RESOLUTION 1024.0
#define PIN_LM35 A0  // Temperature sensor (LM35) pin

// LCD initialization
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Relays and RF pins
const int Relay0 = 2;  // For light
const int FanPin = 3;  // For fan (using PWM)
const int Relay2 = 4, Relay3 = 5;
const int Rf0 = 8, Rf1 = 9, Rf2 = 10, Rf3 = 11;
const int motion_sensor = 12;

// Variables
int motion_data, Rf0_Read, Rf1_Read, Rf2_Read, Rf3_Read;
int lightState;  // 0 = off, 1 = on
int fanSpeed = 0;  // This will track the current fan speed
bool manualOverride = false;
float tempC;
int tp = 0;
float lastTempC = 0;
int lastMotionData = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Home Automation");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Samson Ajadalu");
  lcd.setCursor(0, 1);
  lcd.print("ENG1704075");
  delay(2000);

  pinMode(Relay0, OUTPUT);
  pinMode(FanPin, OUTPUT);  // Fan is controlled by PWM on pin 3
  pinMode(Rf0, INPUT);
  pinMode(Rf1, INPUT);
  pinMode(Rf2, INPUT);
  pinMode(Rf3, INPUT);
  pinMode(motion_sensor, INPUT);

  digitalWrite(Relay0, HIGH);  // Turn off light initially
  analogWrite(FanPin, 0);      // Turn off fan initially (PWM = 0)

  Serial.begin(115200);

  WiFi.init(&Serial1);
  if (WiFi.status() == WL_NO_SHIELD) {
    lcd.setCursor(0, 1);
    lcd.print("No WiFi Shield");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("WiFi Found");
    ThingSpeak.begin(espClient);
  }
  delay(2000);
  lcd.clear();
}

void loop() {
  // Read temperature from LM35 sensor
  if (tp == 10) {
    int adcVal = analogRead(PIN_LM35);
    float milliVolt = adcVal * (ADC_VREF_mV / ADC_RESOLUTION);
    tempC = milliVolt / 10;
    lcd.setCursor(9, 0);
    lcd.print(tempC);
    tp = 0;
  }

  // Read RF inputs and motion sensor
  motion_data = digitalRead(motion_sensor);
  Rf0_Read = digitalRead(Rf0);
  Rf1_Read = digitalRead(Rf1);
  Rf2_Read = digitalRead(Rf2);
  Rf3_Read = digitalRead(Rf3);

  // Handle manual override from RF remote or app
  handleRemoteControl();

  // Check ThingSpeak for manual override commands
  checkThingSpeakManualOverride();  // Check for any manual override from ThingSpeak

  // Read actual states from the relays (fan speed from PWM and light state from digital pin)
  lightState = (digitalRead(Relay0) == LOW) ? 1 : 0; // 1 if light is on, 0 if off

  // Check environmental changes and send to AI if manualOverride is not true
  if (!manualOverride && hasEnvironmentChanged(tempC, motion_data)) {
    sendDataToAI(tempC, motion_data, lightState, fanSpeed);
    String ai_action = getAIAction();  // Get the action from Flask/AI
    executeAIAction(ai_action);        // Perform the action
  }

  // Reset manualOverride flag after the AI takes control
  manualOverride = false;

  tp++;
  delay(100);  // Small delay for stability

  // Write data to ThingSpeak
  ThingSpeak.setField(1, lightState);
  ThingSpeak.setField(2, fanSpeed);
  ThingSpeak.setField(3, motion_data);
  ThingSpeak.setField(4, tempC);
  int responseCode = ThingSpeak.writeFields(counterChannelNumber, myCounterReadAPIKey);
  if (responseCode != 200) {
    lcd.setCursor(0, 1);
    lcd.print("ThingSpeak Error");
  }
}

// Send current data to AI (temperature, motion, and current relay states)
void sendDataToAI(float tempC, int motion_data, int lightState, int fanSpeed) {
  String postData = "{\"temperature\": " + String(tempC) + ", \"motion\": " + String(motion_data) +
                    ", \"lightState\": " + String(lightState) + ", \"fanSpeed\": " + String(fanSpeed) + "}";
  client.beginRequest();
  client.post("/api/environment");
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", postData.length());
  client.beginBody();
  client.print(postData);
  client.endRequest();
}

// Notify AI about manual override along with the current environmental data
void sendManualOverrideToAI() {
  String postData = "{\"manualOverride\": true, \"temperature\": " + String(tempC) +
                    ", \"motion\": " + String(motion_data) + ", \"lightState\": " + String(lightState) +
                    ", \"fanSpeed\": " + String(fanSpeed) + "}";
  client.beginRequest();
  client.post("/api/manual_override");
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", postData.length());
  client.beginBody();
  client.print(postData);
  client.endRequest();
}

// Get the AI's recommended action from Flask
String getAIAction() {
  client.get("/api/get_action");
  int statusCode = client.responseStatusCode();
  if (statusCode == 200) {
    String response = client.responseBody();
    return response;  // Example return, this will be AI's action
  }
  return "";
}

// Execute AI's action (turn relays on or off)
void executeAIAction(String action) {
  if (action == "turn_on_fan_low") {
    setFanSpeed(125);  // Set fan to low speed (PWM = 125)
  } else if (action == "turn_on_fan_high") {
    setFanSpeed(255);  // Set fan to high speed (PWM = 255)
  } else if (action == "turn_off_fan") {
    setFanSpeed(0);    // Turn off fan (PWM = 0)
  } else if (action == "turn_on_light") {
    digitalWrite(Relay0, LOW);  // Turn on light
  } else if (action == "turn_off_light") {
    digitalWrite(Relay0, HIGH); // Turn off light
  }

  delay(10000);  // Wait for 10 seconds to let the environment adjust

  // Record the current environmental state after action
  float currTemp = tempC;  // Update the current temperature
  int currMotion = motion_data;  // Detect motion (1 = motion, 0 = no motion)
  int currLightState = (digitalRead(Relay0) == LOW) ? 1 : 0;  // Get current light state (1 = on, 0 = off)
  int currFanSpeed = fanSpeed;  // Get current fan speed (PWM value)

  // Send the current environmental data and action back to Flask for reward calculation
  sendRewardDataToFlask(currTemp, currMotion, currLightState, currFanSpeed, action);
}

// Send previous and current states to Flask for reward calculation
void sendRewardDataToFlask(float currTemp, int currMotion, int currLightState, int currFanSpeed, String action) {
  String postData = "{\"currTemp\": " + String(currTemp) +
                    ", \"currMotion\": " + String(currMotion) +
                    ", \"currLightState\": " + String(currLightState) +
                    ", \"currFanSpeed\": " + String(currFanSpeed) +
                    ", \"action\": \"" + action + "\"}";

  client.beginRequest();
  client.post("/api/calculate_reward");
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", postData.length());
  client.beginBody();
  client.print(postData);
  client.endRequest();
}

// Set the fan speed using PWM (0 = off, 125 = low speed, 255 = high speed)
void setFanSpeed(int speed) {
  analogWrite(FanPin, speed);  // Set PWM output to control fan speed
  fanSpeed = speed;  // Update the current fan speed

  lcd.setCursor(0, 1);  // Move cursor to line 1 for displaying fan status
  if (speed == 0) {
    lcd.print("FAN OFF       ");  // Clear leftover text by adding spaces
  } else if (speed == 125) {
    lcd.print("FAN SPEED LOW ");  // Display low speed
  } else if (speed == 255) {
    lcd.print("FAN SPEED HIGH");  // Display high speed
  }

  delay(50);  // Small delay for stability
}

// Check if there are manual override commands on ThingSpeak
// Check if there are manual override commands on ThingSpeak
// Check if there are manual override commands on ThingSpeak
void checkThingSpeakManualOverride() {
  // Read all the fields from the ThingSpeak channel at once
  int lightSpeak = ThingSpeak.readFloatField(counterChannelNumber, counterFieldNumber1, myCounterReadAPIKey);
  int fanSpeak = ThingSpeak.readFloatField(counterChannelNumber, counterFieldNumber2, myCounterReadAPIKey);
  int Light2Speak = ThingSpeak.readFloatField(counterChannelNumber, counterFieldNumber3, myCounterReadAPIKey);
  int SocketSpeak = ThingSpeak.readFloatField(counterChannelNumber, counterFieldNumber4, myCounterReadAPIKey);

  // Check if the ThingSpeak read was successful (ThingSpeak returns NaN for failed reads)
  if (isnan(lightSpeak) || isnan(fanSpeak) || isnan(Light2Speak) || isnan(SocketSpeak)) {
    lcd.setCursor(0, 1);
    lcd.print("ThingSpeak Error");
    return;
  }

  // Check for light override
  if (lightSpeak == 1) {
    digitalWrite(Relay0, LOW);  // Turn on light
    manualOverride = true;      // Set manual override to true
    sendManualOverrideToAI();   // Notify AI about manual override
  } else if (lightSpeak == 0) {
    digitalWrite(Relay0, HIGH); // Turn off light
    manualOverride = true;      // Set manual override to true
    sendManualOverrideToAI();   // Notify AI about manual override
  }
  lightState = (digitalRead(Relay0) == LOW) ? 1 : 0; // 1 if light is on, 0 if off

  // Check for fan override
  if (fanSpeak == 255) {
    setFanSpeed(255);  // Set fan to high speed
    manualOverride = true;   // Set manual override to true
    sendManualOverrideToAI();   // Notify AI about manual override
  } else if (fanSpeak == 0) {
    setFanSpeed(0);    // Turn off fan
    manualOverride = true;   // Set manual override to true
    sendManualOverrideToAI();   // Notify AI about manual override
  }

  // Check for Light2Speak override
  if (Light2Speak == 1) {
    digitalWrite(Relay2, LOW);  // Turn on light 2
    manualOverride = true;
    sendManualOverrideToAI();   // Notify AI about manual override
  } else if (Light2Speak == 0) {
    digitalWrite(Relay2, HIGH);  // Turn off light 2
    manualOverride = true;
    sendManualOverrideToAI();   // Notify AI about manual override
  }

  // Check for SocketSpeak override
  if (SocketSpeak == 1) {
    digitalWrite(Relay3, LOW);  // Turn on socket
    manualOverride = true;
    sendManualOverrideToAI();   // Notify AI about manual override
  } else if (SocketSpeak == 0) {
    digitalWrite(Relay3, HIGH);  // Turn off socket
    manualOverride = true;
    sendManualOverrideToAI();   // Notify AI about manual override
  }
}



// Handle the ThingSpeak control for lights and fan
void handleThingSpeakControl(int lightSpeak, int fanSpeak, int Light2Speak, int SocketSpeak) {
  // Light control
  if (lightSpeak == 1) {
    digitalWrite(Relay0, LOW);
  } else if (lightSpeak == 0) {
    digitalWrite(Relay0, HIGH);
  }

  // Fan control
  if (fanSpeak == 255) {
    setFanSpeed(255);
  } else if (fanSpeak == 0) {
    setFanSpeed(0);
  }

  // Additional logic for Relay2 and Relay3 (if needed for your application)
  if (Light2Speak == 1) {
    digitalWrite(Relay2, LOW);
  } else if (Light2Speak == 0) {
    digitalWrite(Relay2, HIGH);
  }

  if (SocketSpeak == 1) {
    digitalWrite(Relay3, LOW);
  } else if (SocketSpeak == 0) {
    digitalWrite(Relay3, HIGH);
  }

  manualOverride = true;
  sendManualOverrideToAI();
}

// Remote control handler for light and fan
void handleRemoteControl() {
  if (Rf0_Read == HIGH) {
    digitalWrite(Relay0, !digitalRead(Relay0));
    lcd.setCursor(0, 1);
    lcd.print(digitalRead(Relay0) == LOW ? "LIGHT ON" : "LIGHT OFF");
    manualOverride = true;
    sendManualOverrideToAI();
  }

  if (Rf1_Read == HIGH) {
    setFanSpeed(fanSpeed > 0 ? 0 : 255);
    manualOverride = true;
    sendManualOverrideToAI();
  }

  if (Rf2_Read == HIGH) {
    digitalWrite(Relay2, !digitalRead(Relay2));
    lcd.setCursor(0, 1);
    lcd.print(digitalRead(Relay2) == LOW ? "LIGHT 2 ON" : "LIGHT 2 OFF");
    manualOverride = true;
    sendManualOverrideToAI();
  }

  if (Rf3_Read == HIGH) {
    digitalWrite(Relay3, !digitalRead(Relay3));
    lcd.setCursor(0, 1);
    lcd.print(digitalRead(Relay3) == LOW ? "SOCKET ON" : "SOCKET OFF");
    manualOverride = true;
    sendManualOverrideToAI();
  }

  delay(200);  // Small debounce delay for RF input
}

// Check if environmental conditions (temperature or motion) have changed
bool hasEnvironmentChanged(float tempC, int motion_data) {
  bool tempChanged = abs(tempC - lastTempC) > 0.5;  // Update if temperature changes more than 0.5°C
  bool motionChanged = motion_data != lastMotionData;

  lastTempC = tempC;
  lastMotionData = motion_data;

  return tempChanged || motionChanged;
}
