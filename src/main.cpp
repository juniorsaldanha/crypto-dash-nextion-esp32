#include <Arduino.h>

#include <Preferences.h>

#include <WiFi.h>
#include <HTTPClient.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include <ArduinoJson.h>

#include <SPI.h>
#include <SD.h>
#include <Nextion.h>
#include <NexUpload.h>

AsyncWebServer server(80);

Preferences preferences;
String ssid;
String password;

String coins_ids;
String coins_ids_default = "bitcoin,ethereum,bomber-coin";
String coins_currencies;
String coins_currencies_default = "usd,eur,brl";

String username_firmware = "esp32";
String password_firmware = "esp32@2ti";
String pathURL = "https://api.coingecko.com/api/v3/simple/price?ids=IDS_TO_REPLACE&vs_currencies=CURRENCIES_TO_REPLACE&include_24hr_change=true";

unsigned long lastTime = 0;
unsigned long timerDelay = 60 * 1000;

void connectWIFI()
{
    preferences.begin("credentials", false);

    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    coins_ids = preferences.getString("coins_ids", coins_ids_default);
    coins_currencies = preferences.getString("coins_currencies", coins_currencies_default);
    if (ssid == "" || password == "")
    {
        Serial.println("No values saved for ssid or password");
    }
    else
    {
        Serial.println("SSID: " + ssid + "\nPASS: " + password);
        // Connect to Wi-Fi
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.print("Connecting to WiFi ..");
        while (WiFi.status() != WL_CONNECTED)
        {
            Serial.print('.');
            delay(1000);
        }
        Serial.print("\nIP: " + WiFi.localIP().toString() + "\n");
    }
}

void startOTA()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Hi! I am your CryptoDash."); });

    AsyncElegantOTA.begin(
        &server,
        username_firmware.c_str(),
        password_firmware.c_str()); // Start ElegantOTA
    server.begin();
    Serial.println("HTTP Server started");
}

DynamicJsonDocument getDateFromCoinGecko()
{
    DynamicJsonDocument responseObject(2048);
    if ((millis() - lastTime) > timerDelay)
    {
        // Check WiFi connection status
        if (WiFi.status() == WL_CONNECTED)
        {
            // String serverPath = pathURL1 + "usd" + pathURL2 + "bomber-coin" + pathURL3;
            // String serverPath = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin,ethereum,bomber-coin&vs_currencies=usd,eur,brl&include_24hr_change=true";
            pathURL.replace("IDS_TO_REPLACE", coins_ids);
            pathURL.replace("CURRENCIES_TO_REPLACE", coins_currencies);
            Serial.println("URL API: " + pathURL);
            String serverPath = pathURL;

            HTTPClient http;

            http.begin(serverPath);
            int httpCode = http.GET();

            // httpCode will be negative on error
            if (httpCode > 0)
            {
                if (httpCode == HTTP_CODE_OK)
                {
                    const char *json = http.getString().c_str();

                    // Parse response
                    deserializeJson(responseObject, json);
                    // serializeJsonPretty(responseObject, Serial);

                    // JsonObject bitcoin = doc["bitcoin"];
                    // float bitcoin_eur = bitcoin["eur"];                       // 9473.3
                    // float bitcoin_eur_24h_change = bitcoin["eur_24h_change"]; // 11.379516678954898
                    // float bitcoin_gbp = bitcoin["gbp"];                       // 8642.89
                    // float bitcoin_gbp_24h_change = bitcoin["gbp_24h_change"]; // 11.58637677393075
                    // float bitcoin_usd = bitcoin["usd"];                       // 11140.6
                    // float bitcoin_usd_24h_change = bitcoin["usd_24h_change"]; // 12.464050392648252
                }
            }
            else
            {
                Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }
            http.end();

            JsonObject bitcoin = responseObject["bitcoin"];
            JsonObject bomber_coin = responseObject["bomber-coin"];
            JsonObject ethereum = responseObject["ethereum"];

            serializeJsonPretty(bitcoin, Serial);
            serializeJsonPretty(bomber_coin, Serial);
            serializeJsonPretty(ethereum, Serial);
        }
        else
        {
            Serial.println("WiFi Disconnected");
        }
        lastTime = millis();
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println();

    connectWIFI();
    startOTA();

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);

    getDateFromCoinGecko();

    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
}