// Required Libraries
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Preferences.h>

extern "C" {
#include <lwip/lwip_napt.h>
#include <lwip/err.h>
#include <lwip/ip_addr.h>
}

WebServer server(80);
Preferences prefs;

// NAT Configuration
void startNAT() {
    ip_napt_init(512, 128);
    ip_napt_enable(WiFi.localIP(), 1);
    Serial.println("NAT Logic: ACTIVE");
}

// Web Server
void handleFileRead(String path, String contentType) {
    if (LittleFS.exists(path)) {
        File file = LittleFS.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
    } else {
        server.send(404, "text/plain", "File Not Found");
    }
}

// WiFi Scan
void handleScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; ++i) {
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        if (i < n - 1) json += ",";
    }
    json += "]";
    server.send(200, "application/json", json);
}

void handleGetConfig() {
    prefs.begin("router-settings", true);
    String json = "{";
    json += "\"ap_ssid\":\"" + prefs.getString("ap_ssid", "ESP32_NAT_Router") + "\",";
    json += "\"sta_ssid\":\"" + prefs.getString("sta_ssid", "") + "\"";
    json += "}";
    server.send(200, "application/json", json);
    prefs.end();
}

void handleSave() {
    if (server.hasArg("ap_ssid")) {
        prefs.begin("router-settings", false);
        prefs.putString("ap_ssid", server.arg("ap_ssid"));
        
        if (server.arg("ap_pass").length() >= 8) {
            prefs.putString("ap_pass", server.arg("ap_pass"));
        }
        
        prefs.putString("sta_ssid", server.arg("sta_ssid"));
        prefs.putString("sta_pass", server.arg("sta_pass"));
        prefs.end();
        
        server.send(200, "text/plain", "OK");
        Serial.println("Settings saved. Rebooting...");
        delay(2000);
        ESP.restart();
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

// Core Setup
void setup() {
    Serial.begin(115200);
    delay(1000);

    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed!");
        return;
    }

    // Saved Credentials
    prefs.begin("router-settings", true);
    String apSSID = prefs.getString("ap_ssid", "ESP32_NAT_Router");
    String apPass = prefs.getString("ap_pass", "12345678");
    String staSSID = prefs.getString("sta_ssid", "");
    String staPass = prefs.getString("sta_pass", "");
    prefs.end();

    // Initialising Access Point
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apSSID.c_str(), apPass.c_str());
    Serial.println("Access Point Started: " + apSSID);

    // Connecting to Internet
    if (staSSID != "") {
        WiFi.begin(staSSID.c_str(), staPass.c_str());
        Serial.print("Connecting to: " + staSSID);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(1000);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\nInternet Connected! IP: " + WiFi.localIP().toString());
            startNAT();
        } else {
            Serial.println("\nUplink Connection Failed.");
        }
    }

    // Web Routes
    server.on("/", []() { handleFileRead("/index.html", "text/html"); });
    server.on("/style.css", []() { handleFileRead("/style.css", "text/css"); });
    server.on("/script.js", []() { handleFileRead("/script.js", "application/javascript"); });
    
    server.on("/scan", HTTP_GET, handleScan);
    server.on("/get_config", HTTP_GET, handleGetConfig);
    server.on("/save", HTTP_POST, handleSave);

    server.begin();
    Serial.println("Web Server Online.");
}

void loop() {
    server.handleClient();
}