#include <Arduino.h>
#include <Wire.h>

String gsm_send_serial(String command, int delay);

#define TINY_GSM_MODEM_SIM7600
#define SerialMon Serial
#define SerialAT Serial1
#define GSM_PIN ""

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

unsigned long prevMillis = 0;
const unsigned long interval = 5000; // Interval for sending messages

void Init(void);                  // Function prototype for network and GPRS initialization
void connectToGPRS(void);         // Function prototype for GPRS connection
void connectToMQTT(void);         // Function prototype for MQTT connection
bool isNetworkConnected();        // Function prototype to check if network is connected
bool isGPRSConnected();           // Function prototype to check if GPRS is connected

void setup() {
    // Set console baud rate
    Serial.begin(115200);
    delay(10);
    SerialAT.begin(UART_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);

    delay(2000);
    pinMode(GSM_RESET, OUTPUT);
    digitalWrite(GSM_RESET, HIGH); // RS-485 
    delay(2000);

    Init();
    connectToGPRS();
    connectToMQTT();
}

void loop() {
    if (millis() - prevMillis >= interval) {
        prevMillis = millis();

        // Send a periodic "ping" message to verify MQTT connection
        String payload=(String)(millis()/1000L);
        String response = gsm_send_serial("AT+CMQTTTOPIC=0,4", 1000);
        response = gsm_send_serial("ping\x1A", 1000);
        response = gsm_send_serial("AT+CMQTTPAYLOAD=0," + String(payload.length()), 1000);
        response = gsm_send_serial(payload + "\x1A", 1000);
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
    gsm_send_serial("AT+CMQTTCONNECT=0,\"tcp://mqtt2.sensoper.net:8883\",60,1,\"Administrator\",\"Sens1234Oper\"", 10000);
    delay(2000);
    gsm_send_serial("AT+CMQTTSUB=0,4,1", 10000);
    gsm_send_serial("ping\x1A", 1000);
    delay(2000);
    gsm_send_serial("AT+CMQTTTOPIC=0,4", 10000);
    gsm_send_serial("ping\x1A", 10000);
    delay(2000);
    gsm_send_serial("AT+CMQTTPAYLOAD=0,4", 10000);
    gsm_send_serial("ping\x1A", 10000);
    delay(2000);
    gsm_send_serial("AT+CMQTTPUB=0,1,60", 10000);
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
