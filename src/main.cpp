#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// LIGHT
#define LIGHT_11 32
#define LIGHT_12 14

// DHT22
#define DHT_PIN 13
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// TEMT6000
#define TEMT_PIN 34

// Wi-Fi Credentials
// const char* ssid = "HCMUT02";
// const char* ssid ="ThanhNguyenChi";
// const char* password ="chithanh";
const char *ssid = "P904";
const char *password = "904ktxbk";

// MQTT Broker Details
String device_id = "ROOM101";
const char *mqtt_clientId = "ROOM101";

// const char* mqtt_server = "10.130.42.129"; //HCMUT02
const char *mqtt_server = "192.168.1.11"; // ROOM 904
const int mqtt_port = 1883;
const char *topic_publish = "SensorData";


WiFiClient esp_client;
void callback(char *topic, byte *payload, unsigned int length);
PubSubClient mqtt_client(mqtt_server, mqtt_port, callback, esp_client);

// Data Sending Time
unsigned long CurrentMillis, PreviousMillis, DataSendingTime = (unsigned long)500 * 10;

// Variable
byte light_11_Status;
byte light_12_Status;
byte fan_1_Status;

void mqtt_connect();

void setup_wifi()
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println("\"" + String(ssid) + "\"");

  WiFi.begin(ssid, password);
  // WiFi.begin(ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqtt_publish(char *data)
{
  mqtt_connect();
  if (mqtt_client.publish(topic_publish, data))
    Serial.println("Publish \"" + String(data) + "\" Sent");
  else
    Serial.println("Publish \"" + String(data) + "\" failed");
}
void mqtt_subscribe(const char *topic)
{
  if (mqtt_client.subscribe(topic))
    Serial.println("Subscribe \"" + String(topic) + "\" Sent");
  else
    Serial.println("Subscribe \"" + String(topic) + "\" failed");
}

void mqtt_connect()
{
  // Loop until we're reconnected
  while (!mqtt_client.connected())
  {
    Serial.println("Attempting MQTT connection...");
    if (mqtt_client.connect(mqtt_clientId))
    {
      Serial.println("MQTT Client Connected");
      mqtt_publish((char *)("Data from" + device_id).c_str());
      // Subscribe
      mqtt_subscribe("device/light11");
      mqtt_subscribe("device/light12");
      mqtt_subscribe("device/fan1");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String command;
  Serial.print("\n\nMessage arrived [");
  Serial.print(topic);
  Serial.println("] ");

  String topicStr = topic;
  int value = payload[0] - '0';

  if (topicStr.equals("device/light11"))
  {
    if (value == 1)
    {
      digitalWrite(LIGHT_11, HIGH);
      light_11_Status = 1;
    }
    else if (value == 0)
    {
      digitalWrite(LIGHT_11, LOW);
      light_11_Status = 0;
    }
  }
  else if (topicStr.equals("device/light12"))
  {
    if (value == 1)
    {
      digitalWrite(LIGHT_12, HIGH);
      light_12_Status = 1;
    }
    else if (value == 0)
    {
      digitalWrite(LIGHT_12, LOW);
      light_12_Status = 0;
    }
  }
  else if (topicStr.equals("device/fan1"))
  {
    if (value == 1)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      fan_1_Status = 1;
    }
    else if (value == 0)
    {
      digitalWrite(LED_BUILTIN, LOW);
      fan_1_Status = 0;
    }
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(LIGHT_11, OUTPUT);
  pinMode(LIGHT_12, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  dht.begin();
  pinMode(TEMT_PIN, INPUT);

  delay(1000);
  setup_wifi();
  mqtt_connect();
}

void loop()
{
  // TemtGettingData
  float lux = analogRead(TEMT_PIN) * 0.244200; // 1000 / 4095 = 0.244200

  // DHT11GettingData
  float DHT_Temperature = dht.readTemperature();
  float DHT_Humidity = dht.readHumidity();

  if (isnan(DHT_Temperature) || isnan(DHT_Humidity))
  {
    Serial.println("\n\nFailed to read from DHT22 sensor!");
    delay(1000);
  }
  else
  {
    Serial.println("\n\nDHT Temperature: " + String(DHT_Temperature) + " °C");
    Serial.println("DHT Humidity: " + String(DHT_Humidity) + " %");
    Serial.println("Light Intensity: " + String(lux) + " lux");
    Serial.println("Light_11: " + String(light_11_Status) + " " + String(light_11_Status == 1 ? "ON" : "OFF"));
    Serial.println("Light_12: " + String(light_12_Status) + " " + String(light_12_Status == 1 ? "ON" : "OFF"));
    Serial.println("Fan_1: " + String(fan_1_Status) + " " + String(fan_1_Status == 1 ? "ON" : "OFF"));

    delay(1000);

    // Devices State Sync Request
    CurrentMillis = millis();
    if (CurrentMillis - PreviousMillis > DataSendingTime)
    {
      PreviousMillis = CurrentMillis;

      // Publish Temperature Data
      String pkt = "{";
      pkt += "\"device_id\": \"ROOM101\", ";
      pkt += "\"type\": \"Temperature\", ";
      pkt += "\"value\": " + String(DHT_Temperature) + "";
      pkt += "}";
      mqtt_publish((char *)pkt.c_str());

      // Publish Humidity Data
      String pkt2 = "{";
      pkt2 += "\"device_id\": \"ROOM101\", ";
      pkt2 += "\"type\": \"Humidity\", ";
      pkt2 += "\"value\": " + String(DHT_Humidity) + "";
      pkt2 += "}";
      mqtt_publish((char *)pkt2.c_str());

      // Publish Light Intensity Data
      String pkt3 = "{";
      pkt3 += "\"device_id\": \"ROOM101\", ";
      pkt3 += "\"type\": \"Light_Intensity\", ";
      pkt3 += "\"value\": " + String(lux) + "";
      pkt3 += "}";
      mqtt_publish((char *)pkt3.c_str());
    }
  }

  if (!mqtt_client.loop())
    mqtt_connect();
}
