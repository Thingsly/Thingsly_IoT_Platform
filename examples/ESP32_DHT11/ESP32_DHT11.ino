/**
 * Thingsly IoT Platform Library
 *
 * This is an example of how to use the Thingsly IoT Platform Library to connect to the server and send data to the server.
 * Example for ESP32 with DHT11 sensor to read temperature and humidity.
 *
 * Wiring:
 * DHT11 VCC → ESP32 3.3V
 * DHT11 GND → ESP32 GND
 * DHT11 Data → ESP32 D4
 *
 * JSON Format Usage:
 * This sensor only publishes data (read-only). No control commands are needed.
 *
 * Data Format:
 * The device will publish temperature and humidity data to the telemetry topic:
 * {"humidity": 65.2, "temperature": 25.6}
 *
 * Data Description:
 * - humidity: Relative humidity in percentage (%)
 *   - Range: 20-90%
 *   - Accuracy: ±5%
 *   - Resolution: 1%
 *
 * - temperature: Temperature in Celsius (°C)
 *   - Range: 0-50°C
 *   - Accuracy: ±2°C
 *   - Resolution: 1°C
 *
 * Wowki: https://wokwi.com/projects/430438476155949057
 *
 * @author Nguyen Thanh Ha 20210298 <ha.nt210298@sis.hust.edu.vn>
 * @version 1.0.6
 * @date 2025-06-29
 *
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
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

// DHT sensor setup
const int pinDHT = 4;
#define DHTTYPE DHT11
DHT dht(pinDHT, DHTTYPE);

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing...");

    dht.begin();
    Serial.println("DHT sensor initialized");

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

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (isnan(humidity) || isnan(temperature))
    {
        Serial.println("Failed to read from DHT sensor! Retrying...");
        delay(SEND_INTERVAL);
        return;
    }

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" *C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    StaticJsonDocument<200> doc;
    doc["humidity"] = humidity;
    doc["temperature"] = temperature;

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
