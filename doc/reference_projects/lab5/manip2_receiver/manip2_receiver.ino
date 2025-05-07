/*
  LoRa register dump

  This examples shows how to inspect and output the LoRa radio's
  registers on the Serial interface
*/
#include <SPI.h>  // include libraries
#include <LoRa.h>
#include <Ecran.h>


Ecran monEcran;

//Paramètre de communication ESP32 et module PMOD-RFM95 V2:
// À vous, étudiant de compléter. Voir le schématique pour trouver.
#define RFM95_ss 5
#define RFM95_rst 14
#define RFM95_dio0 2

//Paramètres de la communication LoRa:
//Consulter: https://www.baranidesign.com/faq-articles/2019/4/23/lorawan-usa-frequencies-channels-and-sub-bands-for-iot-devices
const uint32_t BAND = 905400000;  //905.4MHz, Channel 3
const uint8_t LoRasyncWord = 0xF8;
const uint8_t LoRaSF = 7;
const uint32_t LoRaSB = 125E3;
const uint8_t LoRaCR = 5;

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
  monEcran.begin();

  Serial.println("LoRa Dump Registers");

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

  //The spreading factor (SF) impacts the communication performance of LoRa, which uses an SF between 7 and 12. A larger SF increases the time on air, which increases energy consumption, reduces the data rate, and improves communication range. For successful communication, as determined by the SF, the modulation method must correspond between a transmitter and a receiver for a given packet.
  Serial.println("> RFM95: SF");
  LoRa.setSpreadingFactor(LoRaSF);

  Serial.println("> RFM95: BAND");
  LoRa.setSignalBandwidth(LoRaSB);

  Serial.println("> RFM95: CR");
  LoRa.setCodingRate4(LoRaCR);

  Serial.println("> Terminé");
}

// constexpr SEND_DELAY;

void loop() {
  monEcran.refresh();

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet of size(" + String(packetSize) + ") '");

    // read packet
    while (LoRa.available()) {
      packet_union p_u;
      LoRa.readBytes(p_u.raw, sizeof(p_u.raw));

      Serial.print("did: " + String(p_u.packet.device_id) + ", ");
      Serial.print("mid: " + String(p_u.packet.message_id) + ", ");
      Serial.print("ttl: " + String(p_u.packet.ttl) + ", ");
      Serial.print("lig: " + String(p_u.packet.light_level) + ", ");
      Serial.print("hum: " + String(p_u.packet.humidity) + ", ");
      Serial.print("tem: " + String(p_u.packet.temperature) + ", ");

      monEcran.effacer();
      monEcran.ecrire("did: " + String(p_u.packet.device_id) + ", ", 1);
      monEcran.ecrire("mid: " + String(p_u.packet.message_id) + ", ", 2);
      monEcran.ecrire("ttl: " + String(p_u.packet.ttl) + ", ", 3);
      monEcran.ecrire("lig: " + String(p_u.packet.light_level) + ", ", 4);
      monEcran.ecrire("hum: " + String(p_u.packet.humidity) + ", ", 5);
      monEcran.ecrire("tem: " + String(p_u.packet.temperature) + ", ", 6);
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
}
