#include <Wire.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <LoRa.h>
#include "LoRaBoards.h"
#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

// Constants for LoRa
#ifndef CONFIG_RADIO_FREQ
#define CONFIG_RADIO_FREQ           915.0
#endif
#ifndef CONFIG_RADIO_OUTPUT_POWER
#define CONFIG_RADIO_OUTPUT_POWER   17
#endif
#ifndef CONFIG_RADIO_BW
#define CONFIG_RADIO_BW             125.0
#endif

#if !defined(USING_SX1276) && !defined(USING_SX1278)
#error "LoRa example is only allowed to run SX1276/78. For other RF models, please run examples/RadioLibExamples"
#endif

// GPS object
TinyGPSPlus gps;
HardwareSerial GPS(1);

// Define AWS IoT topic
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

// WiFi and MQTT client setup
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// Variables
String gpsData[4]; // [0] = "gt1", [1] = Latitude, [2] = Longitude, [3] = Time (HH:MM:SS)

void setup() {
    setupBoards();
    delay(1500);

    Serial.begin(115200);
    Serial.println("LoRa Receiver");

    // Initialize Wi-Fi and AWS IoT connection
    connectAWS();

    // Initialize LoRa
#ifdef RADIO_TCXO_ENABLE
    pinMode(RADIO_TCXO_ENABLE, OUTPUT);
    digitalWrite(RADIO_TCXO_ENABLE, HIGH);
#endif

    LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);
    if (!LoRa.begin(CONFIG_RADIO_FREQ * 1000000)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }

    LoRa.setTxPower(CONFIG_RADIO_OUTPUT_POWER);
    LoRa.setSignalBandwidth(CONFIG_RADIO_BW * 1000);
    LoRa.setSpreadingFactor(10);
    LoRa.setPreambleLength(16);
    LoRa.setSyncWord(0xAB);
    LoRa.disableCrc();
    LoRa.disableInvertIQ();
    LoRa.setCodingRate4(7);

    // Start listening for incoming packets
    LoRa.receive();

    // Initialize GPS
    GPS.begin(9600, SERIAL_8N1, 34, 12); // GPS uses TX=12, RX=34
    delay(15000);  // Wait for GPS to stabilize
}

void connectToWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.println("Connecting to Wi-Fi");

    int retries = 0;
    int maxRetries = 20; // Wait for 10 seconds (20 * 500ms)

    while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
        delay(500);
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi connected successfully!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect to Wi-Fi, restarting...");
        ESP.restart(); // Restart ESP32 if Wi-Fi connection fails
    }
}

void connectAWS() {
    connectToWiFi(); // Ensure Wi-Fi is connected before proceeding

    // Configure WiFiClientSecure to use the AWS IoT device credentials
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);

    // Connect to the MQTT broker on the AWS endpoint
    client.setServer(AWS_IOT_ENDPOINT, 8883);

    Serial.println("Connecting to AWS IOT");

    while (!client.connect(THINGNAME)) {
        Serial.print(".");
        delay(100);
    }

    if (!client.connected()) {
        Serial.println("AWS IoT Timeout!");
        return;
    }

    // Subscribe to a topic
    client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
    Serial.println("AWS IoT Connected!");
}

void storeGPSData() {
    // Store "gt1" in the first element
    gpsData[0] = "gt1";
    
    // Store GPS data into the array as a single string
    gpsData[1] = String(gps.location.lat(), 6);  // Latitude
    gpsData[2] = String(gps.location.lng(), 6);  // Longitude

    // Print received GPS coordinates
    Serial.print("Storing Latitude: "); Serial.println(gpsData[1]);
    Serial.print("Storing Longitude: "); Serial.println(gpsData[2]);


    // Print received time
    if (gps.time.isValid()) {
        // Format time as "HH:MM:SS"
        String timeString = String(gps.time.hour() + 8) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());
        gpsData[3] = timeString; // Store formatted time
        Serial.print("Storing Time (HH:MM:SS): ");
        Serial.println(gpsData[3]);
    } else {
        gpsData[3] = "00:00:00"; // Set to default if time is not valid
        Serial.println("Invalid GPS time.");
    }
}

void publishMessage(const String& combinedArray) {
    if (client.connected()) {
        StaticJsonDocument<512> doc;

        JsonArray array = doc.to<JsonArray>();

        // Split the combined array into components
        int index = 0;
        String part;
        String tempCombinedArray = combinedArray;
        while (tempCombinedArray.indexOf(',') != -1) {
            part = tempCombinedArray.substring(0, tempCombinedArray.indexOf(','));
            array.add(part);
            tempCombinedArray.remove(0, tempCombinedArray.indexOf(',') + 1);
        }
        array.add(tempCombinedArray); // Add the last part

        // Serialize the JSON document to a String
        String jsonBuffer;
        serializeJson(doc, jsonBuffer); // Serialize into the String

        delay(1000); // Allow a moment before attempting to publish

        Serial.print("Publishing message: ");
        Serial.println(jsonBuffer);

        if (client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer.c_str())) { // Use c_str() to get a C-style string
            Serial.println("Message successfully published.");
        } else {
            Serial.print("Failed to publish message, error code: ");
            Serial.println(client.state());
        }
    } else {
        Serial.println("Client is not connected to AWS IoT.");
    }
}

void loop() {
    // Ensure the MQTT client runs in the loop
    client.loop(); 

    // Check Wi-Fi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi disconnected, attempting to reconnect...");
        connectToWiFi();
    }

    // Check MQTT connection
    if (!client.connected()) {
        Serial.println("AWS IoT disconnected, attempting to reconnect...");
        connectAWS();
    }

    // Try to receive GPS data
    while (GPS.available()) {
        gps.encode(GPS.read());
    }

    // Check the validity of GPS data
    if (gps.location.isValid() && gps.time.isValid()) {

        // Read LoRa data packet
        int packetSize = LoRa.parsePacket();
        
        // If a LoRa data packet is received
        if (packetSize) {
            String recv = ""; // Initialize the received string
            Serial.print("Received packet: ");
            while (LoRa.available()) {
                recv += (char)LoRa.read();
            }

            // Print the received array
            Serial.print("Received array: ");
            Serial.println(recv);

            // Store GPS data
            storeGPSData();
  
            // Build a new combined array
            String combinedArray = recv + "," + gpsData[0] + "," + gpsData[1] + "," + gpsData[2]+ "," + gpsData[3]+ "," + LoRa.packetRssi()+ "," + LoRa.packetSnr()+ "," + LoRa.parsePacket();
            Serial.println("Combined array: " + combinedArray);

            // Publish the combined array to AWS IoT
            publishMessage(combinedArray);

            // Continue listening for the next LoRa data packet
            LoRa.receive();
        }
    } else {
        Serial.println("Waiting for valid GPS data...");
    }
     delay(1000);
}

