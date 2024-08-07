

#define TINY_GSM_MODEM_SIM7600

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
//#ifndef __AVR_ATmega328P__
#define SerialAT Serial1

#define MODEM_TX 23
#define MODEM_RX 18
#define GSM_RESET 34

#define D0 36
#define D1 35
#define D2 32
#define D3 33
#define R0 4

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Range to attempt to autobaud
// NOTE: DO NOT AUTOBAUD in production code. Once you've established
// communication, set a fixed baud rate using modem.setBaud(#).
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

// Define how you're planning to connect to the internet.
// This is only needed for this example, not in other code.
#define TINY_GSM_USE_GPRS true

// set GSM PIN, if any
#define GSM_PIN ""


// Your GPRS credentials, if any
const char apn[] = "dialogbb";
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT details
const char* broker = "mqtt.sensoper.net";

const char* topic = "Values";
String str_macAddress;

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>  // Include the WiFi library for MAC address
#include "Secret.h" // Include the file to get the username and pasasword of MQTT server

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif


#define MAC_ADDRESS_SIZE 18 // Assuming MAC address is in format "XX:XX:XX:XX:XX:XX"
byte mac[6]; 

#define PUBLISH_INTERVAL 60000  // Publish interval in milliseconds (1 minute)

unsigned long lastPublishTime = 0;
unsigned long int mqtt_interval = 30000;

// Device-specific details
const char* deviceSerial = "34865D461830";  // Replace with your device serial

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif
TinyGsmClient client(modem);
PubSubClient mqtt(client);

uint32_t lastReconnectAttempt = 0;


void mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();

  // Extract serial number from the topic
  String topicStr = String(topic);
  int firstSlash = topicStr.indexOf('/');
  int lastSlash = topicStr.lastIndexOf('/');
  String MAC_ID = topicStr.substring(firstSlash + 1, lastSlash);

  SerialMon.print("MAC ID: ");
  SerialMon.println(MAC_ID);

  if (MAC_ID == deviceSerial) {
    // Decode the received message
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload, len);

    if (error) {
      SerialMon.print("deserializeJson() failed: ");
      SerialMon.println(error.c_str());
      return;
    }

    // Extract the payload
     bool state = doc["state"];

   if(state == 0){
      digitalWrite(R0,LOW);
   }else if(state == 1){
     digitalWrite(R0,HIGH);
   }
     

  } else {
    SerialMon.println("Received message for a different serial number");
  }
}

boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);

  // Connect to MQTT Broker
  boolean status = mqtt.connect("GsmClientTest", username, password);

  if (status == false) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" success");
  String downlinkTopic = "NORVI/+/OUTPUT";
  mqtt.subscribe(downlinkTopic.c_str());  // Subscribe to the specific downlink topic
  return mqtt.connected();
}

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);
  SerialAT.begin(GSM_AUTOBAUD_MAX, SERIAL_8N1, MODEM_RX, MODEM_TX);

  delay(2000);
  
  pinMode(GSM_RESET, OUTPUT);

  pinMode(D0, INPUT);
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(R0, OUTPUT);
  
  digitalWrite(GSM_RESET, HIGH);   // RS-485 

  SerialMon.println("Wait...");

  // Set GSM module baud rate
  TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

#if TINY_GSM_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
#endif

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) { SerialMon.println("Network connected"); }

#if TINY_GSM_USE_GPRS
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }
#endif

  // MQTT Broker setup
  mqtt.setServer(broker, 1881);
  mqtt.setKeepAlive (mqtt_interval/1000);
  mqtt.setSocketTimeout (mqtt_interval/1000);
  mqtt.setCallback(mqttCallback);
  delay(1000);
  String downlinkTopic = "NORVI/+/OUTPUT";
  mqtt.subscribe(downlinkTopic.c_str());  // Subscribe to the specific downlink topic
}

void loop() {
  // Make sure we're still registered on the network
  if (!modem.isNetworkConnected()) {
    SerialMon.println("Network disconnected");
    if (!modem.waitForNetwork(180000L, true)) {
      SerialMon.println(" fail");
      delay(10000);
      return;
    }
    if (modem.isNetworkConnected()) {
      SerialMon.println("Network re-connected");
    }

#if TINY_GSM_USE_GPRS
    // and make sure GPRS/EPS is still connected
    if (!modem.isGprsConnected()) {
      SerialMon.println("GPRS disconnected!");
      SerialMon.print(F("Connecting to "));
      SerialMon.print(apn);
      if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        SerialMon.println(" fail");
        delay(10000);
        return;
      }
      if (modem.isGprsConnected()) { SerialMon.println("GPRS reconnected"); }
    }
#endif
  }

  if (!mqtt.connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) { lastReconnectAttempt = 0; }
    }
    delay(100);
    return;
  }

  mqtt.loop();

  unsigned long now = millis();
  if (now - lastPublishTime > PUBLISH_INTERVAL) {
    lastPublishTime = now;


    bool IN1 = digitalRead(D0);
    bool IN2 = digitalRead(D1);
    bool IN3 = digitalRead(D2);
    bool IN4 = digitalRead(D3);

  

    // Create JSON object
    StaticJsonDocument<200> doc;
    doc["D0"] = IN1 ? 1 : 0;
    doc["D1"] = IN2 ? 1 : 0;
    doc["D2"] = IN3 ? 1 : 0;
    doc["D3"] = IN4 ? 1 : 0;

    String jsonString;
    //char jsonBuffer[512];
    serializeJson(doc, jsonString);

    WiFi.macAddress(mac);
    
    str_macAddress = (String(mac[0] >> 4,  HEX) + String(mac[0] & 0x0F, HEX)) +
                     (String(mac[1] >> 4,  HEX) + String(mac[1] & 0x0F, HEX)) +
                     (String(mac[2] >> 4,  HEX) + String(mac[2] & 0x0F, HEX)) +
                     (String(mac[3] >> 4,  HEX) + String(mac[3] & 0x0F, HEX)) +
                     (String(mac[4] >> 4,  HEX) + String(mac[4] & 0x0F, HEX)) +
                     (String(mac[5] >> 4,  HEX) + String(mac[5] & 0x0F, HEX));
   str_macAddress.toUpperCase();
   String Digital_input = "NORVI/INPUTS/" +  str_macAddress;

    

    // Publish the JSON object
    mqtt.publish(Digital_input.c_str(), jsonString.c_str());
    SerialMon.print("Published: ");
    SerialMon.println(jsonString);
 } 
}
