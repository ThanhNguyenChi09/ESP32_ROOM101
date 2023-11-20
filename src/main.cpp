#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <LiquidCrystal_I2C.h>

// UART Enable
#define RX 16
#define TX 17

HardwareSerial mySerial(2);

// LIGHT
#define LIGHT_11 14
#define LIGHT_12 33

// DHT22
#define DHT_PIN 13
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// TEMT6000
#define TEMT_PIN 32

float lux;
bool autoModeActive = false;

// Wi-Fi Credentials
// const char* ssid = "HCMUT02";
const char *ssid = "P904";
const char *password = "904ktxbk";
// const char *ssid = "ThanhNguyenChi";
// const char *password = "chithanh";

// MQTT Broker Details
String device_id = "ROOM101";
const char *mqtt_clientId = "ROOM101";

// const char* mqtt_server = "10.130.42.129"; //HCMUT02
const char *mqtt_server = "192.168.1.6"; // ROOM 904
// const char *mqtt_server = "172.20.10.3"; // ROOM 904

// MQTT Port
const int mqtt_port = 1883;

// Publish Topic
const char *topic_publish = "SensorData";

WiFiClient esp_client;
void callback(char *topic, byte *payload, unsigned int length);
PubSubClient mqtt_client(mqtt_server, mqtt_port, callback, esp_client);

// Data Sending Times
unsigned long CurrentMillis, PreviousMillis, DataSendingTime = (unsigned long)150 * 10;

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
      mqtt_subscribe("Auto_Mode");
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
      mySerial.print("11");
      // mqtt_client.publish("device/light11", "1"); // Gửi tín hiệu bật đèn 11 tới STM32F103
    }
    else if (value == 0)
    {
      mySerial.print("10");
    }
  }
  else if (topicStr.equals("device/light12"))
  {
    if (value == 1)
    {
      mySerial.print("21");
    }
    else if (value == 0)
    {
      mySerial.print("20");
    }
  }
  else if (topicStr.equals("device/fan1"))
  {
    if (value == 1)
    {
      mySerial.print("31");
    }
    else if (value == 0)
    {
      mySerial.print("30");
    }
  }
  if (topicStr.equals("Auto_Mode"))
  {
    if (value == 1)
    {
      Serial.println("Auto Mode ON");
      Serial.print("Lux value: ");
      Serial.println(lux);

      if (lux <= 50.0)
      {
        Serial.println("Turning on lights");
        mySerial.print("11");
        autoModeActive = true;
        // mySerial.print("21");
      }
      else if (lux > 50)
      {
        Serial.println("Turning on lights");
        mySerial.print("10");
      }
    }
    else if (value == 0)
    {
      Serial.println("Auto Mode OFF");
      autoModeActive = false;
    }
  }
}
// } else if (topicStr.equals("device/fan1")) {
//   if (value == 1) {
//     mqtt_client.publish("device/fan1", "1"); // Gửi tín hiệu bật quạt 1 tới STM32F103
//   } else if (value == 0) {
//     mqtt_client.publish("device/fan1", "0"); // Gửi tín hiệu tắt quạt 1 tới STM32F103
//   }
// }

// if (topicStr.equals("device/light11"))
// {
//   if (value == 1)
//   {
//     digitalWrite(LIGHT_11, HIGH);
//     light_11_Status = 1;
//   }
//   else if (value == 0)
//   {
//     digitalWrite(LIGHT_11, LOW);
//     light_11_Status = 0;
//   }
// }
// else if (topicStr.equals("device/light12"))
// {
//   if (value == 1)
//   {
//     digitalWrite(LIGHT_12, HIGH);
//     light_12_Status = 1;
//   }
//   else if (value == 0)
//   {
//     digitalWrite(LIGHT_12, LOW);
//     light_12_Status = 0;
//   }
// }
// else if (topicStr.equals("device/fan1"))
// {
//   if (value == 1)
//   {
//     digitalWrite(LED_BUILTIN, HIGH);
//     fan_1_Status = 1;
//   }
//   else if (value == 0)
//   {
//     digitalWrite(LED_BUILTIN, LOW);
//     fan_1_Status = 0;
//   }
// }

void setup()
{
  Serial.begin(115200);
  mySerial.begin(500000, SERIAL_8N1, 16, 17);

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
  lux = analogRead(TEMT_PIN) * 0.244200; // 1000 / 4095 = 0.244200

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

    String recived = mySerial.readStringUntil('\n');
    if (recived == "11on")
    {
      light_11_Status = 1;
    }
    if (recived == "11off")
    {
      light_11_Status = 0;
    }

    if (recived == "12on")
    {
      light_12_Status = 1;
    }
    if (recived == "12off")
    {
      light_12_Status = 0;
    }

    if (recived == "fanOn")
    {
      fan_1_Status = 1;
    }
    if (recived == "fanOff")
    {
      fan_1_Status = 0;
    }
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

      String pkt4 = "{";
      pkt4 += "\"device_id\": \"ROOM101\", ";
      pkt4 += "\"type\": \"Light11\", ";
      pkt4 += "\"value\": " + String(light_11_Status) + "";
      pkt4 += "}";
      mqtt_publish((char *)pkt4.c_str());

      String pkt5 = "{";
      pkt5 += "\"device_id\": \"ROOM101\", ";
      pkt5 += "\"type\": \"Light12\", ";
      pkt5 += "\"value\": " + String(light_12_Status) + "";
      pkt5 += "}";
      mqtt_publish((char *)pkt5.c_str());

      String pkt6 = "{";
      pkt6 += "\"device_id\": \"ROOM101\", ";
      pkt6 += "\"type\": \"Fan1\", ";
      pkt6 += "\"value\": " + String(fan_1_Status) + "";
      pkt6 += "}";
      mqtt_publish((char *)pkt6.c_str());
    }
  }
  if (!mqtt_client.loop())
    mqtt_connect();
}
