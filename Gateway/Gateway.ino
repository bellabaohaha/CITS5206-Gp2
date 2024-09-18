#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

// Define AWS IoT topic
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

// WiFi and MQTT client setup
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// Wi-Fi connection function with timeout
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

void connectAWS()
{
  connectToWiFi(); // Ensure Wi-Fi is connected before proceeding

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  // Create a message handler
  client.setCallback(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage()
{
  if (client.connected()) {

    // StaticJsonDocument<512> doc;

    // doc["sb"]["ID"] = "sb1";
    // doc["sb"]["date"] = "2024-09-12";
    // doc["sb"]["time"] = "12:34:56";
    // doc["sb"]["GPS"] = "51.5074, 0.1278";
    // doc["sb"]["battery"] = "75%";

    
    // doc["s1"]["depth"] = 10;
    // doc["s1"]["water"] = 10;

    // doc["s2"]["depth"] = 30;
    // doc["s2"]["water"] = 55;

    // doc["s3"]["depth"] = 50;
    // doc["s3"]["water"] = 60;

    // doc["gt"]["ID"] = "gt1";
    // doc["gt"]["date"] = "2024-09-12";
    // doc["gt"]["time"] = "12:34:56";
    // doc["gt"]["GPS"] = "51.5074, 0.1278";
    // doc["gt"]["battery"] = "80%";

    StaticJsonDocument<512> doc;

    JsonArray array = doc.to<JsonArray>();

    // Sensor board data
    array.add("sb1");             // Sensor board ID
    array.add("2024-09-12");       // Sensor board date
    array.add("12:34:56");         // Sensor board time
    array.add("51.5074, 0.1278");  // Sensor board GPS
    array.add("75%");              // Sensor board battery

    // Sensor 1 data
    array.add(10);                 // Sensor 1 depth, Could be delete
    array.add(10);                 // Sensor 1 water level

    // Sensor 2 data
    array.add(30);                 // Sensor 2 depth, Could be delete
    array.add(55);                 // Sensor 2 water level

    // Sensor 3 data
    array.add(50);                 // Sensor 3 depth, Could be delete
    array.add(60);                 // Sensor 3 water level

    // Gateway data
    array.add("gt1");              // Gateway ID
    array.add("2024-09-12");       // Gateway date
    array.add("12:34:56");         // Gateway time
    array.add("51.5074, 0.1278");  // Gateway GPS
    array.add("80%");              // Gateway battery

    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer); // Convert the JSON object to a string

    delay(1000); // Allow a moment before attempting to publish

    Serial.print("Publishing message: ");
    Serial.println(jsonBuffer);

    if (client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer)) {
      Serial.println("Message successfully published.");
    } else {
      Serial.print("Failed to publish message, error code: ");
      Serial.println(client.state());
    }
  } else {
    Serial.println("Client is not connected to AWS IoT.");
  }
}


void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void setup()
{
  Serial.begin(115200);
  
  // Connect to AWS IoT and send the message
  connectAWS();
  publishMessage();
  
  // Close the connection after sending the message
  Serial.println("Data sent once, stopping further execution.");
}

void loop()
{
  // Nothing here, no need to loop
}
