int x, y, k, j, jt, z;

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

// Pin configuration for TFT
#define TFT_CS  10
#define TFT_RST 9 
#define TFT_DC  8

// Create TFT object for ST7735
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Pin assignments for RF buttons
const int Rf0 = 2, Rf1 = 3, Rf2 = 4, Rf3 = 5;

// Variables to store RF button states
int motion_data;
int Rf0_Read, Rf1_Read, Rf2_Read, Rf3_Read;

// Helper function to update TFT display
void updateDisplay(const char* label, uint16_t color) {
    tft.setTextWrap(false);
    tft.fillScreen(ST7735_BLACK);
    tft.setCursor(0, 30);
    tft.setTextColor(color);
    tft.setTextSize(3);
    tft.println(label);
}

void setup(void) {
    // Set pin modes for RF buttons with pull-up resistors
    pinMode(Rf0, INPUT_PULLUP);
    pinMode(Rf1, INPUT_PULLUP);
    pinMode(Rf2, INPUT_PULLUP);
    pinMode(Rf3, INPUT_PULLUP);
    
    Serial.begin(9600);
    tft.initR(INITR_BLACKTAB);  
    tft.fillScreen(ST7735_BLACK);  
    Serial.println(F("Initialized"));

    // Initial display message
    updateDisplay(""SMART ENERGY OPTIMIZATION SYSTEM BY BY SAMSON ADAJALU"", ST7735_GREEN); 
    delay(1000);

    // Initialize states
    x = LOW;
    y = LOW;
    k = LOW;
    z = LOW;
    j = LOW;
}

void loop() {
    // Read RF button states
    Rf0_Read = digitalRead(Rf0);   
    Rf1_Read = digitalRead(Rf1);
    Rf2_Read = digitalRead(Rf2);
    Rf3_Read = digitalRead(Rf3);   
    
    // Handle RF0 button for FAN control
    if (Rf0_Read == LOW) { // LOW because INPUT_PULLUP inverts logic
        x = !x;
        j = 0;
        updateDisplay("FAN", x == HIGH ? ST7735_GREEN : ST7735_RED);
        jt = 1;
    }

    // Handle RF1 button for LOAD1 control
    if (Rf1_Read == LOW) {
        y = !y;
        j = 0;
        updateDisplay("LOAD1", y == HIGH ? ST7735_GREEN : ST7735_RED);
        jt = 1;
    }

    // Handle RF2 button for LIGHTS control
    if (Rf2_Read == LOW) {
        z = !z;
        j = 0;
        updateDisplay("LIGHTS", z == HIGH ? ST7735_GREEN : ST7735_RED);
        jt = 1;
    }

    // Handle RF3 button for LOAD2 control
    if (Rf3_Read == LOW) {
        k = !k;
        j = 0;
        updateDisplay("LOAD2", k == HIGH ? ST7735_GREEN : ST7735_RED);
        jt = 1;
    }

    // Display a message after 10 loop iterations
    if (jt == 1) {
        j++;
    }

    if (j == 10) {
        tft.setTextWrap(false);
        tft.fillRect(0, 30, 128, 10, ST7735_BLACK);
        tft.setTextColor(ST7735_YELLOW); 
        tft.setTextSize(1);
        testdrawtext("SMART ENERGY OPTIMIZATION SYSTEM BY BY SAMSON ADAJALU", ST7735_YELLOW);
        delay(5000);
        tft.fillRect(0, 30, 128, 10, ST7735_BLACK);
        j = 0;
        jt = 0;
    }

    delay(100); // Small delay to prevent multiple toggles from button bounces
}

// Helper functions for drawing on the TFT
void testdrawtext(char *text, uint16_t color) {
    tft.setCursor(0, 0);
    tft.setTextColor(color);
    tft.setTextWrap(true);
    tft.print(text);
}

// Optional additional graphic effects (your original code)
void testlines(uint16_t color) {
    tft.fillRect(0, 30, 128, 10, ST7735_BLACK);
    for (int16_t x = 0; x < tft.width(); x += 6) {
        tft.drawLine(0, 0, x, tft.height() - 1, color);
        delay(0);
    }
}

void testfillcircles(uint8_t radius, uint16_t color) {
    for (int16_t x = radius; x < tft.width(); x += radius * 2) {
        for (int16_t y = radius; y < tft.height(); y += radius * 2) {
            tft.fillCircle(x, y, radius, color);
        }
    }
}

void testroundrects() {
    tft.fillScreen(ST7735_BLACK);
    uint16_t color = 100;
    int t, i;
    for (t = 0; t <= 4; t += 1) {
        int x = 0, y = 0, w = tft.width() - 2, h = tft.height() - 2;
        for (i = 0; i <= 16; i += 1) {
            tft.drawRoundRect(x, y, w, h, 5, color);
            x += 2;
            y += 3;
            w -= 4;
            h -= 6;
            color += 1100;
        }
    }
}
