// This #include statement was automatically added by the Particle IDE.
#include <ArduinoJson.h>

// This #include statement was automatically added by the Particle IDE.
#include <MQTT.h>

// This #include statement was automatically added by the Particle IDE.
#include <WatchDog_WCL.h>

#include <queue>

// Pins and Globals
int ledPin = D7;
int motionPin = D6;
bool motion = false;
bool DEBUG = true;

WatchDog wd;

// Reconnect tracking
int reconnectCount = 0;
const int MAX_RECONNECTS = 10; // Store the last 10 reconnect timestamps
std::queue<unsigned long> reconnectTimes;

// MQTT Setup
byte server[] = {192, 168, 200, 207};
MQTT client(server, 1883, callback);

// Timing helpers
unsigned long lastBlink = 0;
int blinkState = 0;

// Heartbeat
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 60000; // 60 seconds

// Motion state tracking
int lastMotionState = -1;

// Device metadata
const char* DEVICE_ID = "particle_argon_01";   // TODO: Set this uniquely per device
const char* LOCATION = "lab";                    // TODO: Set this per deployment
const char* FW_VERSION = "1.0.07";

// Function to calculate reconnects in time window
int getReconnectsInWindow(unsigned long window) {
    unsigned long now = Time.now();
    int count = 0;
    std::queue<unsigned long> tempQueue = reconnectTimes; // Copy the queue
    while (!tempQueue.empty()) {
        unsigned long time = tempQueue.front();
        if (now - time < window) {
            count++;
        }
        tempQueue.pop();
    }
    return count;
}

// Accept config updates via MQTT
void handleConfigUpdate(const char* payload) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        if (DEBUG) Serial.println("Config JSON parse failed");
        return;
    }
    if (doc.containsKey("DEBUG")) {
        bool newDebug = false;
        if (doc["DEBUG"].is<bool>()) {
            newDebug = doc["DEBUG"].as<bool>();
            Serial.printlnf("DEBUG (bool) received: %d", newDebug);
        } else if (doc["DEBUG"].is<const char*>()) {
            String dbgStr = String(doc["DEBUG"].as<const char*>());
            newDebug = (dbgStr.equalsIgnoreCase("true") || dbgStr == "1");
            Serial.printlnf("DEBUG (string) received: %s", dbgStr.c_str());
        } else {
            Serial.println("DEBUG key present but not bool or string");
        }
        DEBUG = newDebug;
        Serial.printlnf("DEBUG flag updated via MQTT: %d", DEBUG);
    }
    // Add more config options as needed (e.g., intervals, thresholds)
}

// Accept control commands via MQTT
void handleControlCommand(const char* payload) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        if (DEBUG) Serial.println("Control command JSON parse failed");
        return;
    }
    if (doc.containsKey("led")) {
        String color = String(doc["led"].as<const char*>());
        if (color.equalsIgnoreCase("red")) RGB.color(255, 0, 0);
        else if (color.equalsIgnoreCase("green")) RGB.color(0, 255, 0);
        else if (color.equalsIgnoreCase("blue")) RGB.color(0, 0, 255);
        else if (color.equalsIgnoreCase("off")) RGB.color(0, 0, 0);
        else RGB.color(255, 255, 255);
        if (DEBUG) Serial.printlnf("Set LED color to: %s", color.c_str());
    }
    // Add more control commands as needed
}

void callback(char* topic, byte* payload, unsigned int length) {
    payload[length] = '\0';
    String topicStr(topic);
    if (topicStr == topicFor("system", "config")) {
        handleConfigUpdate((char*)payload);
        return;
    }
    if (topicStr == topicFor("system", "cmd")) {
        handleControlCommand((char*)payload);
        return;
    }
}  

// Helper to build topic strings dynamically
String topicFor(const char* category, const char* event) {
    return String::format("agentic/%s/%s/%s/%s", LOCATION, DEVICE_ID, category, event);
    
}

// Setup
void setup() {
    wd.initialize(5000);
    wd.pet();

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);
    pinMode(motionPin, INPUT_PULLDOWN);

    RGB.control(true);

    if (DEBUG) {
        Serial.begin(9600);
        unsigned long start = millis();
        while (!Serial.isConnected() && millis() - start < 3000) {
            Particle.process();
        }
    }

    char clientId[32];
    snprintf(clientId, sizeof(clientId), "sparkclient_%lu", Time.now());
    if (DEBUG) Serial.printlnf("Connecting MQTT client with ID: %s", clientId);
    client.connect(clientId, "leowise", "LM4F120!mqtt$");

    if (!client.isConnected()) {
        MQTTReconnectClient();
        if (!client.isConnected()) panicLoop();
    }

    client.publish(topicFor("system", "state").c_str(), "Argon Motion Sensor Initialized.");
    client.subscribe(topicFor("system", "config").c_str());
    client.subscribe(topicFor("system", "cmd").c_str());

    RGB.color(0, 255, 0);
    delay(1000);
}

// Main loop
void loop() {
    wd.pet();

    static int reconnectAttempts = 0;
    static unsigned long nextReconnectAttempt = 0;

    if (!client.isConnected()) {
        if (millis() > nextReconnectAttempt) {
            Serial.println("Attempting MQTT reconnect...");
            if (MQTTReconnectClient() == 1) {
                Serial.println("MQTT reconnected!");
                reconnectAttempts = 0; // Reset attempts on success
                lastMotionState = -1; // Reset on reconnect
            } else {
                Serial.println("MQTT reconnect failed.");
                reconnectAttempts++;
                nextReconnectAttempt = millis() + (reconnectAttempts * 10000); // Exponential backoff
                if (reconnectAttempts > 10) {
                    Serial.println("Too many failed reconnects. Resetting device.");
                    System.reset(); // Reset the device
                }
            }
        }
    } else {

        getMotion();
        // Only publish on motion state change
        if (motion != lastMotionState) {
            lastMotionState = motion;
            MQTTSendMotionInfo();
        }
    

        // Heartbeat message - TODO: check if 128 bytes is enough for all fields
        if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
            StaticJsonDocument<256> doc;
            doc["device_id"] = DEVICE_ID;
            doc["location"] = LOCATION;
            doc["status"] = "alive";
            doc["fw_version"] = FW_VERSION;
            doc["ts"] = Time.now();
            char payload[128];
            serializeJson(doc, payload, sizeof(payload));
            client.publish(topicFor("system", "heartbeat").c_str(), payload);
            if (DEBUG) Serial.printlnf("Heartbeat: %s", payload);
            lastHeartbeat = millis();
        }

        client.loop();
    }

}

// Panic LED loop with watchdog petting
void panicLoop() {
    unsigned long lastFlash = millis();
    while (1) {
        wd.pet();

        if (millis() - lastFlash > 2000) {
            RGB.color((blinkState++) % 2 == 0 ? 255 : 255, 0, 0);
            lastFlash = millis();
        }

        if (DEBUG && Serial.available()) Serial.println("MQTT not connected. System halted.");
    }
}

// Motion
void getMotion() {
    motion = digitalRead(motionPin);
}

void MQTTSendMotionInfo() {
    StaticJsonDocument<256> doc;
    doc["device_id"] = DEVICE_ID;
    doc["location"] = LOCATION;
    doc["type"] = "motion";
    doc["motion"] = motion;
    doc["ts"] = Time.now();
    char payload[256];
    size_t len = serializeJson(doc, payload, sizeof(payload));
    payload[len] = '\0'; // Ensure null-termination
    client.publish(topicFor("sensor", "motion").c_str(), payload);
    if (DEBUG) Serial.printlnf("MQTT Motion: %s", payload);

    digitalWrite(ledPin, !digitalRead(ledPin)); // toggle D7
    delay(100);                                 // Short delay to avoid rapid toggling
    digitalWrite(ledPin, !digitalRead(ledPin)); // toggle D7 again
}

// Robust reconnect logic
int MQTTReconnectClient() {
    char clientId[32];
    for (int i = 0; i < 5; i++) {
        snprintf(clientId, sizeof(clientId), "sparkclient_%lu", Time.now());
        client.connect(clientId, "leowise", "LM4F120!mqtt$");
        delay(500);
        if (client.isConnected()) {
            Serial.println("MQTT reconnected!");
            client.publish(topicFor("system", "state").c_str(), "MQTT client has reconnected");
            client.subscribe(topicFor("system", "config").c_str());
            client.subscribe(topicFor("system", "cmd").c_str());

            // Track reconnect
            reconnectCount++;
            unsigned long now = Time.now();
            reconnectTimes.push(now);
            if (reconnectTimes.size() > MAX_RECONNECTS) {
                reconnectTimes.pop(); // Remove oldest if queue is full
            }

            // Check reconnects in window and send alert
            unsigned long window = 24 * 60 * 60; // 24 hours in seconds
            int reconnectsInWindow = getReconnectsInWindow(window);
            if (reconnectsInWindow > 5) { // Threshold
                StaticJsonDocument<256> doc;
                doc["device_id"] = DEVICE_ID;
                doc["location"] = LOCATION;
                doc["type"] = "alert";
                doc["alert_type"] = "reconnects";
                doc["count"] = reconnectsInWindow;
                doc["window"] = window;
                doc["ts"] = now;

                char payload[256];
                size_t len = serializeJson(doc, payload, sizeof(payload));
                payload[len] = '\0';
                client.publish(topicFor("system", "alert").c_str(), payload);
                if (DEBUG) Serial.printlnf("Reconnect Alert: %s", payload);
            }

            return 1;
        }
    }
    return 0;
}