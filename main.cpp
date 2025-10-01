#include <Arduino.h>
#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include <string>
#include <WiFi.h>
#include <HTTPClient.h>
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled!
#endif
#define btcServerName "A50 BLE"
// Define the BluetoothSerial
BluetoothSerial SerialBT;
// Define a variable for the connection status
bool connected = false;
// Callback function for connection events
void deviceConnected(
    esp_spp_cb_event_t event,
    esp_spp_cb_param_t *param)
{
    if (event == ESP_SPP_SRV_OPEN_EVT)
    {
        Serial.println("Device Connected");
        connected = true;
    }
    if (event == ESP_SPP_CLOSE_EVT)
    {
        Serial.println("Device disconnected");
        connected = false;
    }
}
#include <WiFi.h>
#include <HTTPClient.h>
const long connection_timeout = 15000; // 15s
long startConnection = 0;
String Id; // <-- id echipa
void setup()
{
    Serial.begin(115200);
    int time = 0;
    SerialBT.begin(btcServerName);
    SerialBT.register_callback(deviceConnected);
}
void loop()
{
    if (SerialBT.available())
    {
        Serial.println("<--------------------------------------------->");
        String DATA = SerialBT.readString();
        String action;
        Serial.print("Comanda:");
        StaticJsonDocument<1024> doc;
        const char *json = DATA.c_str();
        DeserializationError error = deserializeJson(doc, json);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
        }
        else
        {
            action = doc["action"].as<String>();
            Serial.println(action);
            Serial.println(Id);
            if (action == "getNetworks")
            {
                Id = doc["teamId"].as<String>();
                int networksFound = WiFi.scanNetworks();
                if (networksFound == 0)
                {
                    Serial.println("No networks found");
                }
                else
                {
                    Serial.print(networksFound);
                    Serial.println(" networks found");
                    for (int i = 0; i < networksFound; i++)
                    {
                        bool open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
                        const size_t CAPACITY = JSON_OBJECT_SIZE(4) + JSON_ARRAY_SIZE(4);
                        DynamicJsonDocument doc_retele(CAPACITY);
                        doc_retele["ssid"] = WiFi.SSID(i);
                        doc_retele["strength"] = WiFi.RSSI(i);
                        if ((open) == 1)
                            doc_retele["encryption"] = "Open";
                        else
                            doc_retele["encryption"] = "Protected";
                        doc_retele["teamId"] = Id;
                        String output;
                        serializeJson(doc_retele, output);
                        Serial.println(output);
                        SerialBT.println(output);
                    }
                }
            }
            if (action == "connect")
            {
                String nume = doc["ssid"].as<String>();
                String parola = doc["password"].as<String>();
                Serial.println(nume);
                Serial.println(parola);
                WiFi.mode(WIFI_STA);
                WiFi.disconnect();
                delay(100);
                // conectare la wifi
                WiFi.begin(nume.c_str(), parola.c_str());
                Serial.println("Connecting");
                startConnection = millis();
                bool open = (WiFi.status() != WL_CONNECTED);
                const size_t CAPACITY = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(3);
                DynamicJsonDocument doc_net_conectare(CAPACITY);
                doc_net_conectare["ssid"] = nume;
                doc_net_conectare["connected"] = open;
                doc_net_conectare["teamId"] = Id;
                String output;
                serializeJson(doc_net_conectare, output);
                Serial.println(output);
                SerialBT.println(output);
            }
            if (action == "getData")
            {
                HTTPClient http;
                String site = "http://proiectia.bogdanflorea.ro/api/the-wolf-among-us/characters";
                delay(2000);
                http.begin(site); 
                delay(2000);
                http.setConnectTimeout(20000);
                http.setTimeout(20000);
                int httpResponseCode = http.GET();
                if (httpResponseCode > 0)
                {
                    Serial.print("HTTP Response code: ");
                    Serial.println(httpResponseCode);
                    String payload = http.getString();
                    StaticJsonDocument<1024> getData;
                    http.end();
                    // deserializare in jsonuri mici
                    DynamicJsonDocument minijson(15000);
                    DeserializationError error = deserializeJson(minijson, payload);
                    if (error)
                    {
                        Serial.println(error.c_str());
                    }
                    else
                    {
                        JsonArray list = minijson.as<JsonArray>();
                        int index = 1;
                        for (JsonVariant value : list)
                        {
                            JsonObject listItem = value.as<JsonObject>();
                            const size_t CAPACITY = JSON_OBJECT_SIZE(4) + JSON_ARRAY_SIZE(12);
                            DynamicJsonDocument doc_getData(CAPACITY);
                            JsonObject object = doc_getData.to<JsonObject>();
                            object.set(listItem);
                            String output;
                            doc_getData["id"] = listItem["id"].as<String>();
                            doc_getData["name"] = listItem["name"].as<String>();
                            doc_getData["image"] = listItem["imagePath"].as<String>();
                            doc_getData["teamId"] = Id;
                            serializeJson(doc_getData, output);
                            SerialBT.println(output);
                            Serial.println(output);
                            index++;
                        }
                    }
                }
            }
            if (action == "getDetails")
            {
                String id_detalii = doc["id"].as<String>();
                String details = "";
                String Site_detalii = "http://proiectia.bogdanflorea.ro/api/the-wolf-among-us/character?id=" + id_detalii;
                Serial.println(Site_detalii);
                HTTPClient http;
                http.begin(Site_detalii);
                http.setConnectTimeout(20000);
                http.setTimeout(20000);
                int httpResponseCode = http.GET();
                if (httpResponseCode > 0)
                {
                    Serial.print("HTTP Response code: ");
                    Serial.println(httpResponseCode);
                    // Get response data
                    String payload = http.getString();
                    Serial.println(payload);
                    //http.end();
                    StaticJsonDocument<1024> getDetails;
                    const char *json = payload.c_str();
                    DeserializationError error = deserializeJson(getDetails, payload);
                    if (error)
                    {
                        Serial.println(error.c_str());
                    }
                    else
                    {
                        DynamicJsonDocument detalii(10000);
                        detalii["teamId"] = Id;
                        detalii["id"] = id_detalii;
                        detalii["name"] = getDetails["name"].as<String>();
                        detalii["image"] = getDetails["imagePath"].as<String>();

                        details = details + "name: " + getDetails["name"].as<String>() + "\n";
                        details +="species: " + getDetails["species"].as<String>() + "\n";
                        details += "gender: " + getDetails["gender"].as<String>() + "\n";
                        details += "occupation: " + getDetails["occupation"].as<String>() + "\n";
                        details += "hairColour: " + getDetails["hairColour"].as<String>() + "\n";
                        details += "eyeColour: " + getDetails["eyeColour"].as<String>() + "\n";
                        details += "description: " + getDetails["description"].as<String>();

                        detalii["description"] = details;
                        
                        String output;
                        serializeJson(detalii, output);
                        Serial.println(output);
                        SerialBT.println(output);
                    }
                }
                
            }
        }
    }
}