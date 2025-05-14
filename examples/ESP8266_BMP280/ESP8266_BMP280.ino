/**
 * Thingsly IoT Platform Library
 *
 * This is an example of how to use the Thingsly IoT Platform Library to connect to the server and send data to the server.
 * Example for ESP8266 with BMP280 sensor to read temperature and pressure.
 *
 * Wiring:
 * BMP280 VCC → ESP8266 3.3V
 * BMP280 GND → ESP8266 GND
 * BMP280 SDA → ESP8266 D21
 * BMP280 SCL → ESP8266 D22
 *
 * Wowki: https://wokwi.com/projects/430759237006047233
 * If you want to test this example on Wokwi, you need to config mqtt_server and mqtt_port using Ngrok to setup tunnel to the server locally.
 *
 * @author Nguyen Thanh Ha 20210298 <ha.nt210298@sis.hust.edu.vn>
 * @version 1.0.0
 * @date 2025-05-12
 *
 */

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Set the WiFi details
const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

// Set the MQTT server details
const char *mqtt_server = "YOUR_MQTT_SERVER";
const int mqtt_port = 1883;
const char *mqtt_user = "YOUR_MQTT_USER";
const char *mqtt_password = "YOUR_MQTT_PASSWORD";
const char *mqtt_topic = "devices/telemetry";
const char *mqtt_client_id = "YOUR_MQTT_CLIENT_ID";

// Set the interval to send data to the server
const unsigned long SEND_INTERVAL = 5000;

// BMP280 sensor setup
#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_BMP280 bmp;

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing...");

    // Start I2C communication
    Wire.begin(SDA_PIN, SCL_PIN);

    if (!bmp.begin(0x76))
    {
        Serial.println("Could not find a valid BMP280 sensor, check wiring!");
        while (1)
            ;
    }
    Serial.println("BMP280 sensor initialized");

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    float temperature = bmp.readTemperature();
    float pressure = bmp.readPressure() / 100.0F;

    if (isnan(temperature) || isnan(pressure))
    {
        Serial.println("Failed to read from BMP280 sensor! Retrying...");
        delay(SEND_INTERVAL);
        return;
    }

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" *C");

    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");

    StaticJsonDocument<200> doc;
    doc["temperature"] = temperature;
    doc["pressure"] = pressure;

    char msg[200];
    serializeJson(doc, msg);

    if (client.publish(mqtt_topic, msg))
    {
        Serial.println("Message published successfully");
    }
    else
    {
        Serial.println("Failed to publish message");
    }

    delay(SEND_INTERVAL);
}

void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (client.connect(mqtt_client_id, mqtt_user, mqtt_password))
        {
            Serial.println("connected");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}
