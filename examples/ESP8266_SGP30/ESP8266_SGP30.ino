/**
 * Thingsly IoT Platform Library
 *
 * This is an example of how to use the Thingsly IoT Platform Library to connect to the server and send data to the server.
 * Example for ESP8266 with SGP30 sensor to read CO2 and TVOC.
 * 
 * Wiring:
 * SGP30 VCC → ESP8266 3.3V
 * SGP30 GND → ESP8266 GND
 * SGP30 SDA → ESP8266 D2
 * SGP30 SCL → ESP8266 D1
 *
 * @author Nguyen Thanh Ha 20210298 <ha.nt210298@sis.hust.edu.vn>
 * @version 1.0.0
 * @date 2025-05-11
 *
 */

#include <Wire.h>
#include <Adafruit_SGP30.h>
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

WiFiClient espClient;
PubSubClient client(espClient);

// SGP30 sensor setup
Adafruit_SGP30 sgp;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing...");

    // Start I2C communication
    Wire.begin(4, 5); // SDA, SCL

    if (!sgp.begin())
    {
        Serial.println("Sensor not found!");
        while (1)
            ;
    }
    Serial.println("SGP30 sensor initialized");

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

    if (!sgp.IAQmeasure())
    {
        Serial.println("Failed to read from SGP30 sensor! Retrying...");
        delay(SEND_INTERVAL);
        return;
    }

    Serial.print("eCO2: ");
    Serial.print(sgp.eCO2);
    Serial.print(" ppm\t");
    Serial.print("TVOC: ");
    Serial.print(sgp.TVOC);
    Serial.println(" ppb");

    StaticJsonDocument<200> doc;
    doc["eco2"] = sgp.eCO2;
    doc["tvoc"] = sgp.TVOC;

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
