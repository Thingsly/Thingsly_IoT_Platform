/**
 * Thingsly IoT Platform Library
 *
 * This is an example of how to use the Thingsly IoT Platform Library to connect to the server and control an LED.
 * Example for ESP8266 with LED module.
 *
 * Wiring:
 * LED VCC → ESP8266 3.3V
 * LED GND → ESP8266 GND
 * LED IN → ESP8266 D2 (GPIO2)
 *
 * JSON Format Usage:
 * To control the LED, send JSON messages to the MQTT control topic:
 *
 * LED Control:
 * {"led": 1}     - Turn LED ON
 * {"led": 0}     - Turn LED OFF
 *
 * Response Format:
 * The device will publish its current state to the telemetry topic:
 * {"led": 1, "led_status": "ON"}
 * {"led": 0, "led_status": "OFF"}
 *
 * Wowki: https://wokwi.com/projects/435088150847360001
 *
 * @author Nguyen Thanh Ha 20210298 <ha.nt210298@sis.hust.edu.vn>
 * @version 1.0.6
 * @date 2025-06-29
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

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

// LED pin
#define LED 2

WiFiClient espClient;
PubSubClient client(espClient);

// LED state
bool ledState = false;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing...");

    // Initialize LED pin
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
    Serial.println("LED initialized");

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

    // Check if the message contains LED state
    if (doc.containsKey("led"))
    {
        bool newState = doc["led"] == 1;
        if (newState != ledState)
        {
            ledState = newState;
            digitalWrite(LED, ledState ? HIGH : LOW);
            Serial.print("LED state changed to: ");
            Serial.println(ledState ? "ON" : "OFF");

            // Publish the new state
            publishLedState();
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

    // Publish LED state periodically
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > SEND_INTERVAL)
    {
        publishLedState();
        lastPublish = millis();
    }
}

void publishLedState()
{
    StaticJsonDocument<200> doc;
    doc["led"] = ledState ? 1 : 0;
    doc["led_status"] = ledState ? "ON" : "OFF";

    char msg[200];
    serializeJson(doc, msg);

    if (client.publish(mqtt_publish_topic, msg))
    {
        Serial.println("LED state published successfully");
    }
    else
    {
        Serial.println("Failed to publish LED state");
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