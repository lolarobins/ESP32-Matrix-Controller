#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <JPEGDecoder.h>

#define R1_PIN 25
#define G1_PIN 27
#define B1_PIN 26
#define R2_PIN 14
#define G2_PIN 13
#define B2_PIN 12
#define A_PIN 23
#define B_PIN 22
#define C_PIN 5
#define D_PIN 17
#define E_PIN 21
#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16

HUB75_I2S_CFG::i2s_pins pins = {R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};
HUB75_I2S_CFG mxconfig(64, 64, 1, pins);
MatrixPanel_I2S_DMA *display = nullptr;

Preferences preferences;

AsyncWebServer server(80);

uint8_t *buffer;
size_t cursor;

void setup()
{
    Serial.begin(115200);

    // display initialization
    mxconfig.clkphase = false;
    mxconfig.latch_blanking = 4;
    mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;
    display = new MatrixPanel_I2S_DMA(mxconfig);
    display->begin();
    display->setBrightness8(40);
    display->clearScreen();

    // load flash
    preferences.begin("display", false);

    // connect to network
    preferences.getString("password").length() == 0 ? WiFi.begin(preferences.getString("ssid").c_str()) : WiFi.begin(preferences.getString("ssid").c_str(), preferences.getString("password").c_str());

    display->setCursor(2, 24);
    display->print("Connecting");

    for (int i = 0; i < 30; i++)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            delay(500);                                       // 15s to connect
            display->fillRect(2 + (i * 2), 34, 2, 2, 0xFFFF); // progress bar
        }
    }

    // allow user to connect to wifi
    if (WiFi.status() != WL_CONNECTED)
    {
        display->clearScreen();
        display->setCursor(0, 4);
        display->print("Connect to\nMATRIX32\nand visit\n192.168.4.1\nfor WiFi\nsetup");

        WiFi.disconnect();
        WiFi.softAP("MATRIX32");

        server.on("/", [](AsyncWebServerRequest *request)
                  {
            if (request->hasArg("ssid"))
                preferences.putString("ssid", request->arg("ssid"));
            if (request->hasArg("password"))
                preferences.putString("password", request->arg("password"));
    
            request->send(200, "text/html", "<html><body><form><input name=\"ssid\" type=\"text\" placeholder=\"SSID\" value=\"" +
                preferences.getString("ssid") + "\"><input name=\"password\" placeholder=\"Password\" type=\"text\"><input type=\"submit\"></form></body></html>");

            if (request->args() > 0)
                ESP.restart(); });

        server.begin();

        while (true)
            ;
    }

    // show ip address to allow user to control ESP32 online
    IPAddress addr = WiFi.localIP();
    WiFi.setHostname("MATRIX32");

    // image upload & test setting
    server.on(
        "/upload", HTTP_POST, [](AsyncWebServerRequest *request)
        { request->send(200); },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            if (!index)
            {
                if (buffer != NULL)
                    free(buffer);
                buffer = (uint8_t *)malloc(request->contentLength());
            }

            memcpy(buffer + index, data, len);

            if (final)
            {
                display->drawRGBBitmap(0, 0, (uint16_t *) buffer, 64, 64);
                request->send(200);
                
                free(buffer);
                buffer = NULL;
            }
        });

    server.on("/clear", [](AsyncWebServerRequest *request){
        display->clearScreen();
        request->send(200);
    });

    server.begin();

    display->clearScreen();
    display->setCursor(2, 27);
    display->printf("%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
}

void loop()
{
}
