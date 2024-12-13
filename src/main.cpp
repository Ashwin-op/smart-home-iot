#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// WiFi credentials
const char *ssid = "SETUP-DBC8";
const char *password = "create6389arrow";

// Weather API details
const String city = "Irvine";
const String apiKey = "45e5c65809df4c41b9c75836241112";
const String baseWeatherApiURL = "http://api.weatherapi.com/v1/current.json?key=";

// API base URL
const String apiBaseUrl = "https://fh5fqmsrsf.us-east-1.awsapprunner.com";

// State variables for devices
String led1State = "off";
String led2State = "off";
String fanState = "off";
String alarmTime = "00:00";

// Display pins
#define TFT_CS 5
#define TFT_DC 4
#define TFT_RST -1
#define TFT_MOSI 23
#define TFT_CLK 18
#define TFT_MISO 19

// Touchscreen pins
#define TOUCH_CS 33
#define TOUCH_IRQ 32

// GPIO pins for LEDs and motor
#define LED_ROOM_A 12
#define LED_ROOM_B 14
#define MOTOR_PIN 27

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// Function to draw buttons on the screen
void drawButton(const char *label, int x, int y, int width, int height)
{
  int radius = height / 2;

  tft.fillRoundRect(x, y, width, height, radius, ILI9341_RED);   // Filled button
  tft.drawRoundRect(x, y, width, height, radius, ILI9341_WHITE); // Border

  // Draw centered text
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  int textX = x + (width / 2) - (strlen(label) * 6);
  int textY = y + (height / 2) - 8;
  tft.setCursor(textX, textY);
  tft.print(label);
}

// Function to display alarm time on the screen
void drawAlarm(const char *time)
{
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);

  int16_t x1, y1;
  uint16_t w, h;
  String alarmText = "Alarm: " + String(time);
  tft.getTextBounds(alarmText, 0, 0, &x1, &y1, &w, &h);

  int textX = (tft.width() - w) / 2;
  int textY = (tft.height() - h) / 2 + 100;

  tft.setCursor(textX, textY + 20);
  tft.print("Alarm: ");
  tft.print(time);
}

// Function to fetch weather data
String fetchWeather(String cityName, String &temp, String &desc)
{
  String weatherUrl = baseWeatherApiURL + apiKey + "&q=" + cityName;
  Serial.println("Weather URL: " + weatherUrl);

  HTTPClient http;
  http.begin(weatherUrl);
  int httpCode = http.GET();

  if (httpCode == 200)
  {
    String payload = http.getString();

    // Extract temperature and description
    int tempStart = payload.indexOf("\"temp_c\":") + 9;
    int tempEnd = payload.indexOf(",", tempStart) - 2;
    temp = payload.substring(tempStart, tempEnd) + "\u00B0C";

    int weatherStart = payload.indexOf("\"condition\":{\"text\":\"") + 21;
    int weatherEnd = payload.indexOf("\"", weatherStart);
    desc = payload.substring(weatherStart, weatherEnd);

    http.end();
    return "OK";
  }

  Serial.println("HTTP Error: " + String(httpCode));
  http.end();
  temp = "N/A";
  desc = "Weather unavailable";
  return "Error";
}

// Function to fetch current time
String fetchTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return "Time Unavailable";
  }
  char timeBuffer[6];
  strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", &timeinfo);
  return String(timeBuffer);
}

// Function to control devices by sending commands
void controlDevice(String device, String state)
{
  Serial.println("Sending command to control " + device + " with state: " + state);
  HTTPClient http;
  String url = apiBaseUrl + "/" + device + "?state=" + state;
  http.begin(url);
  http.POST("");
  http.end();
}

void getDeviceState(String device)
{
  HTTPClient http;
  String url = apiBaseUrl + "/" + device;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200)
  {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, http.getStream());
    if (error)
    {
      Serial.println("Failed to parse JSON");
      return;
    }

    // String payload = http.getString();
    if (device == "led/led1")
    {
      led1State = doc["led1"].as<String>();
    }
    else if (device == "led/led2")
    {
      led2State = doc["led2"].as<String>();
    }
    else if (device == "fan")
    {
      fanState = doc["fan"].as<String>();
    }
    else if (device == "alarm")
    {
      alarmTime = doc["alarm"].as<String>();
    }
  }
  http.end();
}

// Setup function
void setup()
{
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(2); // Landscape orientation
  tft.fillScreen(ILI9341_BLUE);

  // Initialize touchscreen
  while (!ts.begin())
  {
    Serial.println("Touch screen initialization failed!");
    delay(1000);
  }
  Serial.println("Touch screen initialized.");

  // Initialize LEDs and motor
  pinMode(LED_ROOM_A, OUTPUT);
  pinMode(LED_ROOM_B, OUTPUT);
  digitalWrite(LED_ROOM_A, LOW);
  digitalWrite(LED_ROOM_B, LOW);
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  // Synchronize time with NTP
  const long gmtOffsetSec = -8 * 3600; // PST
  const int daylightOffsetSec = 3600;  // DST
  configTime(gmtOffsetSec, daylightOffsetSec, "pool.ntp.org");

  // Fetch and display weather data
  String temperature, description;
  fetchWeather(city, temperature, description);
  tft.setTextSize(2);
  tft.setCursor(10, 60);
  tft.print(city);
  tft.setCursor(10, 90);
  tft.print(temperature);
  tft.setCursor(80, 90);
  tft.print(description);

  // Draw buttons for controlling LEDs
  drawButton("Room A", 10, 140, 100, 50);
  drawButton("Room B", 120, 140, 100, 50);
  drawButton("Fan", 65, 200, 100, 50);
  drawAlarm(alarmTime.c_str());
}

// Loop function
void loop()
{
  static String lastTime = "";
  String currentTime = fetchTime();

  // Update time on display if it changes
  if (currentTime != lastTime)
  {
    lastTime = currentTime;
    tft.fillRect(10, 10, 200, 40, ILI9341_BLUE); // Clear time area
    tft.setTextSize(4);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(10, 10);
    tft.print(currentTime);
  }

  // check for led from api every 5 secs
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 10000)
  {
    lastCheck = millis();
    getDeviceState("led/led1");
    getDeviceState("led/led2");
    getDeviceState("fan");
    getDeviceState("alarm");

    digitalWrite(LED_ROOM_A, (led1State == "on") ? HIGH : LOW);
    digitalWrite(LED_ROOM_B, (led2State == "on") ? HIGH : LOW);
    digitalWrite(MOTOR_PIN, (fanState == "on") ? HIGH : LOW);
    drawAlarm(alarmTime.c_str());
  }

  // Handle touchscreen interaction
  if (ts.touched())
  {
    TS_Point p = ts.getPoint();
    int x = map(p.x, 314, 3961, 0, 320);
    int y = map(p.y, 236, 3884, 0, 240);

    // Room A button
    if (y >= 10 && y <= 110 && x >= 140 && x <= 190)
    {
      // led1State = (led1State == "on") ? "off" : "on";
      controlDevice("led/led1", (led1State == "on") ? "off" : "on");
      // digitalWrite(LED_ROOM_A, (led1State == "on") ? HIGH : LOW);
    }

    // Room B button
    if (y >= 120 && y <= 220 && x >= 140 && x <= 190)
    {
      // led2State = (led2State == "on") ? "off" : "on";
      controlDevice("led/led2", (led2State == "on") ? "off" : "on");
      // digitalWrite(LED_ROOM_B, (led2State == "on") ? HIGH : LOW);
    }

    // Fan button
    if (y >= 65 && y <= 165 && x >= 200 && x <= 250)
    {
      // fanState = (fanState == "on") ? "off" : "on";
      controlDevice("fan", (fanState == "on") ? "off" : "on");
      // digitalWrite(MOTOR_PIN, (fanState == "on") ? HIGH : LOW);
    }
  }
  delay(50); // Small delay for responsiveness
}
