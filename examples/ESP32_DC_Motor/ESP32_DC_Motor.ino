/**
 * Thingsly IoT Platform Library
 *
 * This is an example of how to use the Thingsly IoT Platform Library to connect to the server and control a DC Motor.
 * Example for ESP32 with DC Motor and L298N Motor Driver.
 *
 * Wiring:
 * L298N ENA → ESP32 GPIO14
 * L298N IN1 → ESP32 GPIO27
 * L298N IN2 → ESP32 GPIO26
 * L298N VCC → ESP32 5V
 * L298N GND → ESP32 GND
 * Motor → L298N OUT1, OUT2
 *
 * JSON Format Usage:
 * To control the motor, send JSON messages to the MQTT control topic:
 *
 * Motor Control:
 * {"motor": 1}     - Move forward
 * {"motor": 2}     - Move backward
 * {"motor": 3}     - Stop motor
 *
 * Speed Control:
 * {"speed": 150}   - Set speed (0-255)
 *
 * Combined Control:
 * {"motor": 1, "speed": 200}  - Move forward at speed 200
 * {"motor": 2, "speed": 100}  - Move backward at speed 100
 *
 * Response Format:
 * The device will publish its current state to the telemetry topic:
 * {"motor": 1, "motor_status": "FORWARD", "speed": 200}
 * {"motor": 2, "motor_status": "BACKWARD", "speed": 100}
 * {"motor": 3, "motor_status": "STOPPED", "speed": 0}
 *
 * Wowki: update soon
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

// Motor A pins
int motor1Pin1 = 27;
int motor1Pin2 = 26;
int enable1Pin = 14;

// Setting PWM properties
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;

// Motor states
enum MotorState
{
    STOP,
    FORWARD,
    BACKWARD
};

MotorState motorState = STOP;
int motorSpeed = 200; // 0-255

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing...");

    // Initialize motor pins
    pinMode(motor1Pin1, OUTPUT);
    pinMode(motor1Pin2, OUTPUT);
    pinMode(enable1Pin, OUTPUT);

    // Configure LEDC PWM
    ledcAttachChannel(enable1Pin, freq, resolution, pwmChannel);

    // Stop motor initially
    stopMotor();
    Serial.println("DC Motor initialized");

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

    // Check if the message contains motor control
    if (doc.containsKey("motor"))
    {
        int motorCommand = doc["motor"].as<int>();

        switch (motorCommand)
        {
        case 1: // Forward
            moveMotorForward();
            break;
        case 2: // Backward
            moveMotorBackward();
            break;
        case 3: // Stop
            stopMotor();
            break;
        default:
            Serial.println("Invalid motor command");
            break;
        }

        // Check for speed control
        if (doc.containsKey("speed"))
        {
            int newSpeed = doc["speed"].as<int>();
            if (newSpeed >= 0 && newSpeed <= 255)
            {
                motorSpeed = newSpeed;
                setMotorSpeed(motorSpeed);
            }
        }

        // Publish the new state
        publishMotorState();
    }
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    // Publish motor state periodically
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > SEND_INTERVAL)
    {
        publishMotorState();
        lastPublish = millis();
    }
}

void moveMotorForward()
{
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, HIGH);
    setMotorSpeed(motorSpeed);
    motorState = FORWARD;
    Serial.println("Motor moving forward");
}

void moveMotorBackward()
{
    digitalWrite(motor1Pin1, HIGH);
    digitalWrite(motor1Pin2, LOW);
    setMotorSpeed(motorSpeed);
    motorState = BACKWARD;
    Serial.println("Motor moving backward");
}

void stopMotor()
{
    digitalWrite(motor1Pin1, LOW);
    digitalWrite(motor1Pin2, LOW);
    ledcWrite(pwmChannel, 0);
    motorState = STOP;
    Serial.println("Motor stopped");
}

void setMotorSpeed(int speed)
{
    if (speed >= 0 && speed <= 255)
    {
        ledcWrite(pwmChannel, speed);
        motorSpeed = speed;
        Serial.print("Motor speed set to: ");
        Serial.println(speed);
    }
}

void publishMotorState()
{
    StaticJsonDocument<200> doc;

    // Add motor state as numeric value
    switch (motorState)
    {
    case FORWARD:
        doc["motor"] = 1;
        doc["motor_status"] = "FORWARD";
        break;
    case BACKWARD:
        doc["motor"] = 2;
        doc["motor_status"] = "BACKWARD";
        break;
    case STOP:
        doc["motor"] = 3;
        doc["motor_status"] = "STOPPED";
        break;
    }

    doc["speed"] = motorSpeed;

    char msg[200];
    serializeJson(doc, msg);

    if (client.publish(mqtt_publish_topic, msg))
    {
        Serial.println("Motor state published successfully");
    }
    else
    {
        Serial.println("Failed to publish motor state");
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
