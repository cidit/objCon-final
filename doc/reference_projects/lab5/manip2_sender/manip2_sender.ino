/*
  LoRa register dump
 
  This examples shows how to inspect and output the LoRa radio's
  registers on the Serial interface
*/
#include <SPI.h>  // include libraries
#include <LoRa.h>
 
#include <Ecran.h>
Ecran monEcran;
 
#include "DHT.h"
DHT dht(19, DHT11);
float temperatureC;
 
//Paramètre de communication ESP32 et module PMOD-RFM95 V2:
// À vous, étudiant de compléter. Voir le schématique pour trouver.
#define RFM95_ss 5
#define RFM95_rst 14
#define RFM95_dio0 2
 
#define LDR 35
 
//Paramètres de la communication LoRa:
//Consulter: https://www.baranidesign.com/faq-articles/2019/4/23/lorawan-usa-frequencies-channels-and-sub-bands-for-iot-devices
const uint32_t BAND = 905400000;  //906.9MHz, Channel 3
const uint8_t LoRasyncWord = 0xF8;
const uint8_t LoRaSF = 7;
const uint32_t LoRaSB = 125E3;
const uint8_t LoRaCR = 5;
 
 
int counter = 0;
 
struct packet_frame {
  float light_level, humidity, temperature;
  uint32_t ttl, message_id, device_id;
};
 
union packet_union {
  packet_frame packet;
  uint8_t raw[sizeof(packet_frame)];
};
 
void setup() {
  Serial.begin(115200);  // initialize serial
  while (!Serial)
    ;
 
  Serial.println("LoRa Dump Registers");
 
  dht.begin();
  pinMode(LDR, INPUT);
 
  //setup LoRa transceiver module
  Serial.println("> RFM95: Init pins");
  LoRa.setPins(RFM95_ss, RFM95_rst, RFM95_dio0);
 
  Serial.println("> RFM95: Initializing");
  while (!LoRa.begin(BAND)) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("> RFM95: configuration réussie");
 
  Serial.println("> RFM95: dumping registers:");
  LoRa.dumpRegisters(Serial);
  Serial.println("> RFM95: dump complété.");
 
  Serial.println("> RFM95: set registers:");
  Serial.println("> RFM95: LoRaSyncWord");
  LoRa.setSyncWord(LoRasyncWord);
 
  Serial.println("> RFM95: set CRC");
  LoRa.enableCrc();
 
  //The spreading factor (SF) impacts the communication performance of LoRa, which uses an SF between 7 and 12.A larger SF increases the time on air, which increases energy consumption, reduces the data rate, and improves communication range. For successful communication, as determined by the SF, the modulation method must correspond between a transmitter and a receiver for a given packet.
  Serial.println("> RFM95: SF");
  LoRa.setSpreadingFactor(LoRaSF);
 
  Serial.println("> RFM95: BAND");
  LoRa.setSignalBandwidth(LoRaSB);
 
  Serial.println("> RFM95: CR");
  LoRa.setCodingRate4(LoRaCR);
 
  Serial.println("> Terminé");
}
 
 
void loop() {
 
  packet_union packet_u;
 
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
 
  packet_u.packet.device_id = 777;
  packet_u.packet.humidity = humidity;
  packet_u.packet.light_level = analogRead(LDR);
  packet_u.packet.message_id = counter;
  packet_u.packet.temperature = temperature;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
  packet_u.packet.ttl = millis();
 
  Serial.println("Sending packet: ");
  Serial.print("Packet [( Device ID = ");
  Serial.print(packet_u.packet.device_id);
  Serial.print("),( Humidité = ");
  Serial.print(packet_u.packet.humidity);
  Serial.print("),( Ligh level = ");
  Serial.print(packet_u.packet.light_level);
  Serial.print("),( message ID = ");
  Serial.print(packet_u.packet.message_id);
  Serial.print("),( Température = ");
  Serial.print(packet_u.packet.temperature);
  Serial.print("),( TTL = ");
  Serial.print(packet_u.packet.ttl);
  Serial.print(")]");
  Serial.println();
  Serial.println(counter);
 
  int packet_size = sizeof(packet_frame);
  //Send LoRa packet to receiver
 
  LoRa.beginPacket();
  LoRa.write(packet_u.raw, packet_size);
  LoRa.endPacket();
 
  counter++;
  delay(30000);
}