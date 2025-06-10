/**
 * Thingsly IoT Platform Library
 *
 * This is an example of how to use the Thingsly IoT Platform Library to connect to the server and control a relay.
 * Example for ESP32 with relay module and status LED.
 *
 * Wiring:
 * Relay VCC → ESP32 3.3V
 * Relay GND → ESP32 GND
 * Relay IN → ESP32 D14
 * LED connected to relay NC pin to show relay status
 *
 * Wowki: https://wokwi.com/projects/432039637302787073
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
const char *mqtt_publish_topic = "devices/telemetry";
const char *mqtt_control_topic = "devices/telemetry/control/23a69dda-a909-20ae-86c6-8ca84d0a4307";
const char *mqtt_client_id = "mqtt_23a69dda-a90";

// Set the interval to send data to the server
const unsigned long SEND_INTERVAL = 5000;

// Relay pin
const int relayPin = 14;

WiFiClient espClient;
PubSubClient client(espClient);

// Relay state
bool relayState = false;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing...");

    // Initialize relay pin
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
    Serial.println("Relay initialized");

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

    // Check if the message contains relay state
    if (doc.containsKey("relay"))
    {
        bool newState = doc["relay"] == 1;
        if (newState != relayState)
        {
            relayState = newState;
            digitalWrite(relayPin, relayState ? HIGH : LOW);
            Serial.print("Relay state changed to: ");
            Serial.println(relayState ? "ON" : "OFF");
            Serial.println("LED status: " + String(relayState ? "ON" : "OFF"));

            // Publish the new state
            publishRelayState();
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

    // Publish relay state periodically
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > SEND_INTERVAL)
    {
        publishRelayState();
        lastPublish = millis();
    }
}

void publishRelayState()
{
    StaticJsonDocument<200> doc;
    doc["relay"] = relayState ? 1 : 0;
    doc["led_status"] = relayState ? "ON" : "OFF";

    char msg[200];
    serializeJson(doc, msg);

    if (client.publish(mqtt_publish_topic, msg))
    {
        Serial.println("Relay state published successfully");
    }
    else
    {
        Serial.println("Failed to publish relay state");
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