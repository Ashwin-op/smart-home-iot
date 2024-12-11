#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// Pin definitions for the display
#define TFT_CS 5
#define TFT_DC 4
#define TFT_RST -1
#define TFT_MOSI 23
#define TFT_CLK 18
#define TFT_MISO 19

// Pin definition for touch controller
#define TOUCH_CS 33
#define TOUCH_IRQ 32

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

void setup()
{
  Serial.begin(115200);

  // Initialize display
  tft.begin();
  tft.setRotation(3); // Landscape orientation
  tft.fillScreen(ILI9341_BLACK);

  // Initialize touch screen
  while (!ts.begin())
  {
    Serial.println("Touch screen initialization failed!");
    delay(1000);
  }

  Serial.println("Touch screen initialized.");

  // Display welcome message
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Touch Test with XPT2046!");
}

void loop()
{
  // Check if the screen is touched
  if (ts.touched())
  {
    // Get touch point
    TS_Point p = ts.getPoint();

    // Map touch coordinates to screen resolution
    int x = map(p.x, 314, 3961, 0, 320);
    int y = map(p.y, 236, 3884, 0, 240);

    // Print touch coordinates to Serial Monitor
    Serial.print("X: ");
    Serial.print(x);
    Serial.print(", Y: ");
    Serial.println(y);

    // Draw a dot on the screen at the touch point
    tft.fillCircle(x, y, 3, ILI9341_RED);
  }
}
