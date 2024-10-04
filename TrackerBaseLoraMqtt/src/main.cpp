#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>

const char* ssid = "NETGEAREA758B-g026";
const char* password = "boulerouge62219!";

const char* mqtt_server = "188.165.52.200";

WiFiClient espClient;
PubSubClient client(espClient);

BLEScan* pBLEScan;
int scanTime = 5;

unsigned long lastMsg = 0;
const long interval = 10000;  

String formatBSSID(uint8_t *bssid) {
    String bssidStr = "";
    for (int i = 0; i < 6; i++) {
        if (i > 0) {
            bssidStr += ":";  
        }
        bssidStr += String(bssid[i], HEX);  
    }
    return bssidStr;
}

void setup_wifi() {
    delay(5000);
    Serial.println();
    Serial.print("Connexion à ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connecté");
    Serial.println("Adresse IP : ");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.macAddress());
    Serial.print("Wi-Fi Channel: ");
    Serial.println(WiFi.channel());
}

void mqttConnect() {
    while (!client.connected()) {
        Serial.print("Tentative de connexion au broker MQTT...");
        if (client.connect("ESP32Client")) {
            Serial.println("connecté");
        } else {
            Serial.print("Échec, code erreur = ");
            Serial.print(client.state());
            Serial.println(" nouvelle tentative dans 5 secondes");
            delay(5000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message reçu [");
    Serial.print(topic);
    Serial.print("] : ");
    
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    setup_wifi();

    BLEDevice::init("");

    pBLEScan = BLEDevice::getScan();
    pBLEScan->setInterval(100); 
    pBLEScan->setWindow(99); 
    pBLEScan->setActiveScan(true); 

    client.setBufferSize(512);
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    mqttConnect();

    client.subscribe("test/topic");
}

void loop() {
    if (!client.connected()) {
        mqttConnect();
    }

    client.loop(); 

    unsigned long now = millis();
    if (now - lastMsg > interval) {
        lastMsg = now;

        Serial.println("Recherche des réseaux Wi-Fi...");
        int n = WiFi.scanNetworks();
        Serial.println("Scan Wi-Fi terminé.");

        if (n == 0) {
            Serial.println("Aucun réseau Wi-Fi trouvé");
        } else {
            String wifiResults = String(n) + " réseau(x) trouvé(s) :\n";
            for (int i = 0; i < n; ++i) {
                wifiResults += WiFi.SSID(i);
                wifiResults += " (RSSI : ";
                wifiResults += WiFi.RSSI(i);
                wifiResults += " dBm, BSSID : ";
                
                uint8_t* bssid = WiFi.BSSID(i); 
                wifiResults += formatBSSID(bssid); 
                wifiResults += ")\n";
            }

            Serial.println("Envoi des résultats Wi-Fi via MQTT...");
            bool success = client.publish("test/wifi_scan", wifiResults.c_str());

            if (success) {
                Serial.println("Résultats du scan Wi-Fi publiés avec succès :\n" + wifiResults);
            } else {
                Serial.println("Échec de la publication MQTT !");
            }
        }
    }
}
