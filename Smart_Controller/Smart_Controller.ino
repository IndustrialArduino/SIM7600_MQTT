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
String broker = "mqtt.sensoper.net";
String MQTTport="1881";

#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 60          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD 115200

#define MODEM_TX 23
#define MODEM_RX 18
#define GSM_RESET 34

void Init(void){                                   // Connecting with the newtwork and GPRS
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

void getMQTTS(){
    gsm_send_serial("AT+CMQTTSTART", 1000);
    gsm_send_serial("AT+CMQTTACCQ=0,\"client test0\",0", 1000);
    gsm_send_serial("AT+CMQTTWILLTOPIC=0,2", 1000);
    gsm_send_serial("01\x1A", 1000);
    gsm_send_serial("AT+CMQTTWILLMSG=0,6,1", 1000);
    gsm_send_serial("qwerty\x1A", 1000);
    gsm_send_serial("AT+CMQTTCONNECT=0,\"tcp://mqtt.sensoper.net:1881\",60,1,\"Administrator\",\"Sens1234Oper\"", 10000); 
    delay(2000);
    gsm_send_serial("AT+CMQTTSUB=0,2,1", 10000); 
    gsm_send_serial("AB\x1A", 1000);
    delay(2000);
    gsm_send_serial("AT+CMQTTTOPIC=0,2", 10000);
    gsm_send_serial("AB\x1A", 10000);
    delay(2000);
    gsm_send_serial("AT+CMQTTPAYLOAD=0,2", 10000);
    gsm_send_serial("mq\x1A", 10000);
    delay(2000);
    gsm_send_serial("AT+CMQTTPUB=0,1,60", 10000);

}

String gsm_send_serial(String command, int delay){
    String buff_resp = "";
    Serial.println("Send ->: " + command);
    SerialAT.println(command);
    long wtimer = millis();
    while (wtimer + delay > millis())
    {
        while (SerialAT.available())
        {
            buff_resp = SerialAT.readString();
            Serial.println(buff_resp);
        }
    }
    Serial.println();
    return buff_resp;
}

void setup(){
    // Set console baud rate
    Serial.begin(115200);
    delay(10);
    SerialAT.begin(UART_BAUD, SERIAL_8N1, MODEM_RX, MODEM_TX);

    delay(2000);
    pinMode(GSM_RESET, OUTPUT);
    digitalWrite(GSM_RESET,HIGH );   // RS-485 
    delay(2000);
    
    Init();
    getMQTTS();
}

void loop(){
  
}    
