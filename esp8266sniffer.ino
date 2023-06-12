#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <SoftwareSerial.h>

// CONNECTIONS
// ESP8266 WEMOS LOLIN              HW-519         N1-CT
// 3V-------------------------------VCC
// GND------------------------------GND
// D4-------------------------------RXD
//                                   A+ ------------ D+
//                                   B- ------------ D-
//
// note: power the ESP8266 with a USB power supply. Do not connect Vin to the wallbox.

// Define the MQTT broker credentials
const char* mqtt_server = "192.168.1.220";
const char* mqtt_username = "your_MQTT_username";
const char* mqtt_password = "your_MQTT_password";
const char* mqtt_topic_val = " wbx98302884/n1ct/current/value/";
const char* mqtt_topic_raw = " wbx98302884/n1ct/current/rawhex/";
const char* mqtt_topic_error = " wbx98302884/n1ct/current/error/";

// Define the rs485serial port pins
int rx_pin = D4;
int tx_pin = D5; // not really needed, we are receiving only

//modbus response should start with  0x01 0x03 0x04 
//address 1, function 3, 4 bytes
const byte responseheader[4] = {0x01, 0x03, 0x04};

// Create a WiFiManager object
WiFiManager wifiManager;

// Create a SoftwareSerial object
SoftwareSerial rs485serial(rx_pin, tx_pin);

// Create a PubSubClient object
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600); // for monitoring and debug
  rs485serial.begin(9600);   
  delay(1000);

  // Connect to the WiFi network
  wifiManager.autoConnect();

  // Connect to the MQTT broker
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnect();
}

void loop() {
// Check if the MQTT client is connected
  if (!client.connected()) {
    reconnect();
  }

  // Read the rs485serial data, 36 bytes  (guess that's enough)
  if (rs485serial.available()) {
    byte modbusbuffer[36];
    rs485serial.readBytes(modbusbuffer, 36);
    for( byte i=0; i<35; i++ ) {
      Serial.printf("%02X", modbusbuffer[i]); // print for inspection
      Serial.print(" ");
    }
    Serial.println("");

    byte modbusvalue[4]; // our value is 4 bytes
   
    memcpy(modbusvalue, &modbusbuffer[3], 4); // only copy bytes 3 to 6 to the new array, which hold the value of the register

    // check if header is ok 
    int ok = 1;    
    for( byte i=0; i<3; i++ ) {
      Serial.printf("%02X", modbusbuffer[i]); // print for inspection
      Serial.printf("%02X", responseheader[i]); // print for inspection
      if (modbusbuffer[i] != responseheader[i] ) {
        Serial.println("Invalid header, discarding input");
        ok = 0;
      }
    }


// push raw hex first

   String rawmodbusstring = byteArrayToString(modbusbuffer, sizeof(modbusbuffer));
   Serial.println(rawmodbusstring);
   char rawmodbuschararray[rawmodbusstring.length() + 1];
   rawmodbusstring.toCharArray(rawmodbuschararray, rawmodbusstring.length() + 1);
   client.publish(mqtt_topic_raw, rawmodbuschararray); //publish the value

  if (ok == 0 ){
    Serial.println("no match, skipping");
    client.publish(mqtt_topic_error, "no match, skipping"); //publish the error
//    client.publish(mqtt_topic_val, ""); //clear value
        return; // discard any reading, start from the beginning
  } else {
        client.publish(mqtt_topic_error, ""); //clear error
  }

    // if we got this far, we are good to decode
    reverseArray(modbusvalue); // needed for arduino...
    float floatValue = hexToFloat(modbusvalue); 
    Serial.println(floatValue); 

    // mqtt sends a string
    char msg_out[20];
    dtostrf(floatValue,2,2,msg_out); // 99.99 Amps will do
    client.publish(mqtt_topic_val, msg_out); //publish the value

    Serial.println("-------------------"); 

  }

  // Handle the MQTT client events
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Handle the MQTT messages received
  // we don't listen
}

void reconnect() {
  // Loop until the MQTT client is connected
  while (!client.connected()) {
    // Connect to the MQTT broker
    if (client.connect("ESP8266Client")) {
      // Subscribe to the MQTT topic
      client.subscribe(mqtt_topic_val);
    } else {
      // Wait before retrying
      delay(5000);
    }
  }
}


float hexToFloat(byte* hexArray) {
  float floatValue;
  memcpy(&floatValue, hexArray, sizeof(float));
  return floatValue;
}


void reverseArray(byte arr[]) {
  int size = sizeof(arr) / sizeof(arr[0]);
  for (int i = 0; i < size / 2; i++) {
    int temp = arr[i];
    arr[i] = arr[size - i - 1];
    arr[size - i - 1] = temp;
  }
}

String byteArrayToString(byte* byteArray, int length) {
  String result = "";
  for (int i = 0; i < length; i++) {
    if (byteArray[i] < 0x10) {
      result += "0";
    }
    result += String(byteArray[i], HEX);
    if (i < length - 1) {
      result += " ";
    }
  }
  return result;
}
