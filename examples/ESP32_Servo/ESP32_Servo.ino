/**
 * Thingsly IoT Platform Library
 *
 * This is an example of how to use the Thingsly IoT Platform Library to connect to the server and control a Servo Motor.
 * Example for ESP32 with Servo Motor.
 *
 * Wiring:
 * Servo VCC → ESP32 5V
 * Servo GND → ESP32 GND
 * Servo Signal → ESP32 GPIO13
 *
 * JSON Format Usage:
 * To control the servo, send JSON messages to the MQTT control topic:
 *
 * Servo Control:
 * {"servo": 0}     - Move to 0 degrees
 * {"servo": 90}    - Move to 90 degrees
 * {"servo": 180}   - Move to 180 degrees
 *
 * Any value between 0-180 degrees is supported
 *
 * Response Format:
 * The device will publish its current state to the telemetry topic:
 * {"servo": 90, "servo_status": "POSITION_90"}
 *
 * Wowki: update soon
 *
 * @author Nguyen Thanh Ha 20210298 <ha.nt210298@sis.hust.edu.vn>
 * @version 1.0.0
 * @date 2025-06-29
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// Setup WiFi and MQTT parameters
const char *ssid = "Wokwi-GUEST"; // Replace with your network SSID
const char *password = "";        // Replace with your network password

const char *mqtt_server = "103.124.93.210";
const int mqtt_port = 1883;
const char *mqtt_user = "607aa904-8527-6c18-fb4";
const char *mqtt_password = "8ab80ff";
const char *mqtt_publish_topic = "devices/telemetry";
const char *mqtt_control_topic = "devices/telemetry/control/50824d7e-6b13-9a1a-8eec-6a4a8bcc7902";
const char *mqtt_client_id = "mqtt_50824d7e-6b1";

// Set the interval to send data to the server
const unsigned long SEND_INTERVAL = 5000;

// Servo pin
static const int servoPin = 13;

Servo servo1;

// Servo state
int servoPosition = 90; // Default position

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing...");

    // Initialize servo
    servo1.attach(servoPin);
    servo1.write(servoPosition);
    Serial.println("Servo initialized");

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

    // Convert payload to string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    Serial.println(message);

    // Parse JSON message
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.println("Failed to parse JSON");
        return;
    }

    // Check if the message contains servo control
    if (doc.containsKey("servo"))
    {
        int newPosition = doc["servo"].as<int>();

        // Validate position range (0-180 degrees)
        if (newPosition >= 0 && newPosition <= 180)
        {
            if (newPosition != servoPosition)
            {
                servoPosition = newPosition;
                servo1.write(servoPosition);
                Serial.print("Servo moved to position: ");
                Serial.println(servoPosition);

                // Publish the new state
                publishServoState();
            }
        }
        else
        {
            Serial.println("Invalid servo position (must be 0-180)");
        }
    }
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    // Publish servo state periodically
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > SEND_INTERVAL)
    {
        publishServoState();
        lastPublish = millis();
    }
}

void publishServoState()
{
    StaticJsonDocument<200> doc;
    doc["servo"] = servoPosition;
    doc["servo_status"] = "POSITION_" + String(servoPosition);

    char msg[200];
    serializeJson(doc, msg);

    if (client.publish(mqtt_publish_topic, msg))
    {
        Serial.println("Servo state published successfully");
    }
    else
    {
        Serial.println("Failed to publish servo state");
    }
}

void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        if (client.connect(mqtt_client_id, mqtt_user, mqtt_password))
        {
            Serial.println("connected");
            // Subscribe to control topic
            client.subscribe(mqtt_control_topic);
            Serial.print("Subscribed to: ");
            Serial.println(mqtt_control_topic);
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
