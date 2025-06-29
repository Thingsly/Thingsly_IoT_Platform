/**
 * Thingsly IoT Platform Library
 *
 * This is an example of how to use the Thingsly IoT Platform Library to connect to the server and control a Stepper Motor.
 * Example for ESP32 with 28BYJ-48 Stepper Motor and ULN2003 Motor Driver.
 *
 * Wiring:
 * ULN2003 IN1 → ESP32 GPIO19
 * ULN2003 IN2 → ESP32 GPIO18
 * ULN2003 IN3 → ESP32 GPIO5
 * ULN2003 IN4 → ESP32 GPIO17
 * ULN2003 VCC → ESP32 5V
 * ULN2003 GND → ESP32 GND
 * 28BYJ-48 → ULN2003 OUT1, OUT2, OUT3, OUT4
 *
 * JSON Format Usage:
 * To control the stepper motor, send JSON messages to the MQTT control topic:
 *
 * Stepper Control:
 * {"stepper": 1}     - Move clockwise
 * {"stepper": 2}     - Move counterclockwise
 * {"stepper": 3}     - Stop motor
 *
 * Steps Control:
 * {"steps": 2048}    - Set number of steps (default: 2048 for one revolution)
 *
 * Speed Control:
 * {"speed": 10}      - Set speed in RPM (1-20 recommended)
 *
 * Combined Control:
 * {"stepper": 1, "steps": 1024, "speed": 5}  - Move clockwise 1024 steps at 5 RPM
 *
 * Response Format:
 * The device will publish its current state to the telemetry topic:
 * {"stepper": 1, "stepper_status": "CLOCKWISE", "steps": 2048, "speed": 5}
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
#include <Stepper.h>

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

// ULN2003 Motor Driver Pins
#define IN1 19
#define IN2 18
#define IN3 5
#define IN4 17

// Stepper motor properties
const int stepsPerRevolution = 2048; // 28BYJ-48 has 2048 steps per revolution
Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

// Stepper states
enum StepperState
{
    STOP,
    CLOCKWISE,
    COUNTERCLOCKWISE
};

StepperState stepperState = STOP;
int stepperSpeed = 5;                  // RPM
int currentSteps = stepsPerRevolution; // Default steps for one revolution

WiFiClient espClient;
PubSubClient client(espClient);

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("Initializing...");

    // Initialize stepper motor
    myStepper.setSpeed(stepperSpeed);
    Serial.println("Stepper Motor initialized");

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

    // Check for speed control first
    if (doc.containsKey("speed"))
    {
        int newSpeed = doc["speed"].as<int>();
        if (newSpeed >= 1 && newSpeed <= 20)
        {
            stepperSpeed = newSpeed;
            myStepper.setSpeed(stepperSpeed);
            Serial.print("Stepper speed set to: ");
            Serial.println(stepperSpeed);
        }
    }

    // Check for steps control
    if (doc.containsKey("steps"))
    {
        int newSteps = doc["steps"].as<int>();
        if (newSteps > 0 && newSteps <= 4096) // Reasonable limit
        {
            currentSteps = newSteps;
            Serial.print("Steps set to: ");
            Serial.println(currentSteps);
        }
    }

    // Check if the message contains stepper control
    if (doc.containsKey("stepper"))
    {
        int stepperCommand = doc["stepper"].as<int>();

        switch (stepperCommand)
        {
        case 1: // Clockwise
            moveStepperClockwise();
            break;
        case 2: // Counterclockwise
            moveStepperCounterclockwise();
            break;
        case 3: // Stop
            stopStepper();
            break;
        default:
            Serial.println("Invalid stepper command");
            break;
        }

        // Publish the new state
        publishStepperState();
    }
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    // Publish stepper state periodically
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > SEND_INTERVAL)
    {
        publishStepperState();
        lastPublish = millis();
    }
}

void moveStepperClockwise()
{
    stepperState = CLOCKWISE;
    Serial.print("Stepper moving clockwise ");
    Serial.print(currentSteps);
    Serial.println(" steps");

    myStepper.step(currentSteps);

    // Return to stop state after movement
    stepperState = STOP;
    Serial.println("Stepper movement completed");
}

void moveStepperCounterclockwise()
{
    stepperState = COUNTERCLOCKWISE;
    Serial.print("Stepper moving counterclockwise ");
    Serial.print(currentSteps);
    Serial.println(" steps");

    myStepper.step(-currentSteps);

    // Return to stop state after movement
    stepperState = STOP;
    Serial.println("Stepper movement completed");
}

void stopStepper()
{
    stepperState = STOP;
    Serial.println("Stepper stopped");
}

void publishStepperState()
{
    StaticJsonDocument<200> doc;

    // Add stepper state as numeric value
    switch (stepperState)
    {
    case CLOCKWISE:
        doc["stepper"] = 1;
        doc["stepper_status"] = "CLOCKWISE";
        break;
    case COUNTERCLOCKWISE:
        doc["stepper"] = 2;
        doc["stepper_status"] = "COUNTERCLOCKWISE";
        break;
    case STOP:
        doc["stepper"] = 3;
        doc["stepper_status"] = "STOPPED";
        break;
    }

    doc["steps"] = currentSteps;
    doc["speed"] = stepperSpeed;

    char msg[200];
    serializeJson(doc, msg);

    if (client.publish(mqtt_publish_topic, msg))
    {
        Serial.println("Stepper state published successfully");
    }
    else
    {
        Serial.println("Failed to publish stepper state");
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
