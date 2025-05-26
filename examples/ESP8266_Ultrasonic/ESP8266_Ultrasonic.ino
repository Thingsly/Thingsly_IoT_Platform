/**
 * Thingsly IoT Platform Library
 *
 * This is an example of how to use the Thingsly IoT Platform Library to connect to the server and send data to the server.
 * Example for ESP8266 with HC-SR04 ultrasonic sensor to measure distance.
 *
 * Wiring:
 * HC-SR04 VCC → ESP8266 3.3V
 * HC-SR04 GND → ESP8266 GND
 * HC-SR04 TRIG → ESP8266 D15
 * HC-SR04 ECHO → ESP8266 D2
 * 
 * Wowki: https://wokwi.com/projects/432042876480256001
 *
 * @author Nguyen Thanh Ha 20210298 <ha.nt210298@sis.hust.edu.vn>
 * @version 1.0.0
 * @date 2025-05-27
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Setup WiFi and MQTT parameters
const char *ssid = "Wokwi-GUEST"; // Replace with your network SSID
const char *password = "";        // Replace with your network password

const char *mqtt_server = "103.124.93.210";
const int mqtt_port = 1883;
const char *mqtt_user = "319e111e-ae21-7695-ee8";
const char *mqtt_password = "872e27d";
const char *mqtt_topic = "devices/telemetry";
const char *mqtt_client_id = "mqtt_23a69dda-a90";

// Set the interval to send data to the server
const unsigned long SEND_INTERVAL = 5000;

// Ultrasonic sensor pins
const int TRIG_PIN = 15;
const int ECHO_PIN = 2;

// Speed of sound in cm/s
const float SOUND_SPEED = 0.034;

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing...");

    // Initialize ultrasonic sensor pins
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);
    Serial.println("Ultrasonic sensor initialized");

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

float measureDistance()
{
    // Clear the TRIG_PIN
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    // Set the TRIG_PIN HIGH for 10 microseconds
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Read the ECHO_PIN, return the sound wave travel time in microseconds
    long duration = pulseIn(ECHO_PIN, HIGH);

    // Calculate the distance
    float distance = duration * SOUND_SPEED / 2;

    return distance;
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    float distance = measureDistance();

    if (distance > 0)
    {
        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" cm");

        StaticJsonDocument<200> doc;
        doc["distance"] = distance;

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
    }
    else
    {
        Serial.println("Failed to read from ultrasonic sensor! Retrying...");
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