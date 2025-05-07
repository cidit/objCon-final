/*  --- Entête principale -- information sur le programme
 *  Programme:        Récepteur laboratoire LoRa
 *  Date:             9/04/2025
 *  Auteur:           TRIVALLE et St-Gelais
 *  Pltform matériel: (plateforme matérielle et version - ESP32 DOIT KIT V1 - protoTPhys)
 *  Description:      Recevoir un message via wifi et l'affichier sur le moniteur série
*/
/* --- Materiel et composants -------------------------------------------
 * - CONSULTER le fichier de l'envoyeur
*/


//--- Déclaration des librairies (en ordre alpha) -----------------------

#include <Ecran.h>
#include <LoRa.h>
#include <SPI.h>

//--- Definitions -------------------------------------------------------

//Paramètre de communication ESP32 et module PMOD-RFM95 V2:
#define RFM95_ss 5
#define RFM95_rst 14
#define RFM95_dio0 2


//--- Constantes --------------------------------------------------------

//Paramètres de la communication LoRa:
const uint32_t BAND = 908.1E6;  // Hz
const uint8_t LoRasyncWord = 0xF8;
const uint8_t LoRaSF = 7;
const uint32_t LoRaSB = 125E3;
const uint8_t LoRaCR = 5;

Ecran monEcran;

// Déja documenté: voir le fichier de l'envoyeur.
struct packet_frame {
  float light_level, humidity, temperature;
  uint32_t ttl, message_id, device_id;
};

// Déja documenté: voir le fichier de l'envoyeur.
union packet_union {
  packet_frame packet;
  uint8_t raw[sizeof(packet_frame)];
};

//--- Setup -----------------------------------------------------

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  monEcran.begin();

  Serial.println("LoRa Dump Registers");

  // --- DEBUT INITIALISATION LORA
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

  Serial.println("> RFM95: SF");
  LoRa.setSpreadingFactor(LoRaSF);

  Serial.println("> RFM95: BAND");
  LoRa.setSignalBandwidth(LoRaSB);

  Serial.println("> RFM95: CR");
  LoRa.setCodingRate4(LoRaCR);
  // --- FIN INITIALISATION LORA

  Serial.println("> Terminé");
}  //Fin de setup()

//--- Loop -----------------------------------------------------

void loop() {
  monEcran.refresh();

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // recevoir un packet
    Serial.print("Received packet of size(" + String(packetSize) + ") '");

    // Lire packet
    while (LoRa.available()) {
      packet_union p_u;
      LoRa.readBytes(p_u.raw, sizeof(p_u.raw));

      //Affichage des valeurs sur le moniteur
      Serial.print("did: " + String(p_u.packet.device_id) + ", ");
      Serial.print("mid: " + String(p_u.packet.message_id) + ", ");
      Serial.print("ttl: " + String(p_u.packet.ttl) + ", ");
      Serial.print("lig: " + String(p_u.packet.light_level) + ", ");
      Serial.print("hum: " + String(p_u.packet.humidity) + ", ");
      Serial.print("tem: " + String(p_u.packet.temperature) + ", ");

      //Affichage des valeurs sur l'écran
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
}  //Fin de loop()