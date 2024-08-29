#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>  // Include the WiFi library for MAC address
#include <ArduinoJson.h>
#include "Secret.h" // Include the file to get the username and pasasword of MQTT server

String gsm_send_serial(String command, int delay);

#define SerialMon Serial
#define SerialAT Serial1
#define GSM_PIN ""

#define D0 36
#define D1 35
#define D2 32
#define D3 33
#define R0 4

// Your GPRS credentials, if any
const char apn[] = "dialogbb";
const char gprsUser[] = "";
const char gprsPass[] = "";

// MQTT details
String broker = "mqtt2.sensoper.net";
String MQTTport = "8883";

#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 60          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD 115200

#define MODEM_TX 23
#define MODEM_RX 18
#define GSM_RESET 34

#define MAC_ADDRESS_SIZE 18 // Assuming MAC address is in format "XX:XX:XX:XX:XX:XX"
byte mac[6]; 
String str_macAddress;

unsigned long prevMillis = 0;
const unsigned long interval = 60000; // Interval for sending messages

// Device-specific details
const char* deviceSerial = "34865D461830";  // Replace with your device serial

void Init(void);                  // Function prototype for network and GPRS initialization
void connectToGPRS(void);         // Function prototype for GPRS connection
void connectToMQTT(void);         // Function prototype for MQTT connection
bool isNetworkConnected();        // Function prototype to check if network is connected
bool isGPRSConnected();           // Function prototype to check if GPRS is connected

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

void setup() {
    // Set console baud rate
    Serial.begin(115200);
    delay(10);
    SerialAT.begin(UART_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);

    delay(2000);
    pinMode(GSM_RESET, OUTPUT);
    digitalWrite(GSM_RESET, HIGH); 
    delay(2000);

    pinMode(D0, INPUT);
    pinMode(D1, INPUT);
    pinMode(D2, INPUT);
    pinMode(D3, INPUT);
    pinMode(R0, OUTPUT);

    Init();
    connectToGPRS();
    connectToMQTT();
}

void loop() {
    if (millis() - prevMillis >= interval) {
           prevMillis = millis();

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

    SerialMon.print("Published: ");
    SerialMon.println(jsonString);
   
    // Send a periodic "ping" message to verify MQTT connection
    String payload=(String)(millis()/1000L);
    String response = gsm_send_serial("AT+CMQTTTOPIC=0," + String(Digital_input.length()), 1000);
    response = gsm_send_serial(Digital_input, 1000);
    response = gsm_send_serial("AT+CMQTTPAYLOAD=0," + String(jsonString.length()), 1000);
    response = gsm_send_serial(jsonString + "\x1A", 1000);
    response = gsm_send_serial("AT+CMQTTPUB=0,1,60", 1000);

    // Check if the publish command was successful
      if (response.indexOf("ERROR") != -1) {
            Serial.println("MQTT publish failed. Reconnecting...");
            connectToMQTT();
            if (!isGPRSConnected()) {
                Serial.println("GPRS connection lost. Reconnecting...");
                connectToGPRS();
                connectToMQTT();
            }
            if (!isNetworkConnected()) {
                Serial.println("Network connection lost. Reconnecting...");
                Init();
                connectToGPRS();
                connectToMQTT();
            }
        }
    }
    // Handle incoming MQTT messages
    handleIncomingMessages();
}

void handleIncomingMessages() {
    while (SerialAT.available()) {
        String response = SerialAT.readStringUntil('\n');
        response.trim();

        if (response.startsWith("+CMQTTRXTOPIC")) {
            // Extract topic length
            int topicLength = response.substring(response.indexOf(",") + 1).toInt();
            SerialMon.print("Topic Length: ");
            SerialMon.println(topicLength);
            
            // Read the topic
            String topic = SerialAT.readStringUntil('\n');
            topic.trim();
            SerialMon.print("Topic: ");
            SerialMon.println(topic);

            // Confirm receipt of payload length
            response = SerialAT.readStringUntil('\n');
            response.trim();

            if (response.startsWith("+CMQTTRXPAYLOAD")) {
                int payloadLength = response.substring(response.indexOf(",") + 1).toInt();
                SerialMon.print("Payload Length: ");
                SerialMon.println(payloadLength);
                
                // Read the payload
                String payload = SerialAT.readStringUntil('\n');
                payload.trim();
                SerialMon.print("Payload: ");
                SerialMon.println(payload);

                // Ensure arrays are large enough to hold the topic and payload
                char topicArr[topicLength + 1];
                char payloadArr[payloadLength + 1];

                topic.toCharArray(topicArr, topicLength + 1);
                payload.toCharArray(payloadArr, payloadLength + 1);

                mqttCallback(topicArr, (byte*)payloadArr, payloadLength);
            }
        }
    }
}

void Init(void) {
    delay(5000);
    gsm_send_serial("AT+CFUN=1", 10000);
    gsm_send_serial("AT+CPIN?", 10000);
    gsm_send_serial("AT+CSQ", 1000);
    gsm_send_serial("AT+CREG?", 1000);
    gsm_send_serial("AT+COPS?", 1000);
    gsm_send_serial("AT+CGATT?", 1000);
    gsm_send_serial("AT+CPSI?", 500);
    gsm_send_serial("AT+CGDCONT=1,\"IP\",\"dialogbb\"", 1000);
    gsm_send_serial("AT+CGACT=1,1", 1000);
    gsm_send_serial("AT+CGATT?", 1000);
    gsm_send_serial("AT+CGPADDR=1", 500);
    gsm_send_serial("AT+NETOPEN", 500);
    gsm_send_serial("AT+NETSTATE", 500);
}

void connectToGPRS(void) {
    gsm_send_serial("AT+CGATT=1", 1000);
    gsm_send_serial("AT+CGDCONT=1,\"IP\",\"dialogbb\"", 1000);
    gsm_send_serial("AT+CGACT=1,1", 1000);
    gsm_send_serial("AT+CGPADDR=1", 500);
    gsm_send_serial("AT+NETOPEN", 500);
    gsm_send_serial("AT+NETSTATE", 500);
}

void connectToMQTT(void) {
    gsm_send_serial("AT+CMQTTSTART", 1000);
    gsm_send_serial("AT+CMQTTACCQ=0,\"client test0\",1", 1000);
    gsm_send_serial("AT+CMQTTSSLCFG=0,1", 1000);
    gsm_send_serial("AT+CMQTTWILLTOPIC=0,4", 1000);
    gsm_send_serial("ping\x1A", 1000);
    gsm_send_serial("AT+CMQTTWILLMSG=0,6,1", 1000);
    gsm_send_serial("qwerty\x1A", 1000);
    String command = "AT+CMQTTCONNECT=0,\"tcp://mqtt2.sensoper.net:8883\",60,1,\"" + username + "\",\"" + password + "\"";
    gsm_send_serial(command, 10000);
    delay(2000);
    String downlinkTopic = "NORVI/+/OUTPUT";
    gsm_send_serial("AT+CMQTTSUB=0,14,1", 1000);
    gsm_send_serial(downlinkTopic + "\x1A", 1000);
    delay(2000);
}

bool isNetworkConnected() {
    String response = gsm_send_serial("AT+CREG?", 3000);
    return (response.indexOf("+CREG: 0,1") != -1 || response.indexOf("+CREG: 0,5") != -1);
}

bool isGPRSConnected() {
    String response = gsm_send_serial("AT+CGATT?", 3000);
    return (response.indexOf("+CGATT: 1") != -1);
}

String gsm_send_serial(String command, int delay) {
    String buff_resp = "";
    Serial.println("Send ->: " + command);
    SerialAT.println(command);
    long wtimer = millis();
    while (wtimer + delay > millis()) {
        while (SerialAT.available()) {
            buff_resp += SerialAT.readString();
        }
    }
    Serial.println(buff_resp);
    return buff_resp;
}
