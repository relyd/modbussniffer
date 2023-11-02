#define HEADER_1 0x01
#define HEADER_2 0x03
#define HEADER_3 0x04

void setup() {
 Serial.begin(9600, SERIAL_8N1);
}

void loop() {
 if (Serial.available() >= 8) {
 byte header1 = Serial.read();
 byte header2 = Serial.read();
 byte header3 = Serial.read();
 if (header1 == HEADER_1 && header2 == HEADER_2 && header3 == HEADER_3) {
   byte payload[4];
   for (int i = 0; i < 4; i++) {
     payload[i] = Serial.read();
   }
   byte crcHigh = Serial.read();
   byte crcLow = Serial.read();
   uint16_t calculatedCRC = calculateCRC(payload, 4);
   if (crcHigh == (calculatedCRC >> 8) && crcLow == (calculatedCRC & 0xFF)) {
     float value = payloadToFloat(payload);
     if (value != 0) {
       Serial.print("Value: ");
       Serial.println(value, 6);
     }
   } else {
     Serial.println("CRC error");
   }
 }
 }
}

float payloadToFloat(byte *payload) {
 uint32_t value = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
 return *(float*)&value;
}

uint16_t calculateCRC(uint8_t *data, uint16_t length) {
 uint16_t crc = 0xFFFF;
 for (uint16_t pos = 0; pos < length; pos++) {
 crc ^= (uint16_t)data[pos]; // XOR byte into least sig. byte of crc
 for (uint8_t i = 8; i != 0; i--) { 
   if ((crc & 0x0001) != 0) { 
     crc >>= 1; 
     crc ^= 0xA001; 
   } else {
     crc >>= 1; 
   }
 }
 }
 return crc;
}
