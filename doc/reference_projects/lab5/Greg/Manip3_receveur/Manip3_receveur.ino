/*  --- Entête principale -- information sur le programme
 *  Programme:        Récepteur laboratoire LoRa
 *  Date:             9/04/2025
 *  Auteur:           TRIVALLE et St-Gelais
 *  Pltform matériel: (plateforme matérielle et version - ESP32 DOIT KIT V1 - protoTPhys)
 *  Description:      Recevoir un message via wifi, l'affichier sur le moniteur série et l'envoyer sur un serveur infonuagique
*/
/* --- Materiel et composants -------------------------------------------
 * - CONSULTER le fichier de l'envoyeur
*/

//--- Déclaration des librairies (en ordre alpha) -----------------------

#include <ArduinoHttpClient.h>  //Librairie wrapper HTTP: arduinoHttpClient v0.6.1
#include <Ecran.h>
#include <esp_wifi.h>  //requis pour changer la MAC du Wifi
#include <LoRa.h>
#include <SPI.h>
#include "ThingSpeak.h"
#include <WiFi.h>

//--- Definitions -------------------------------------------------------

//Paramètre de communication ESP32 et module PMOD-RFM95 V2:
#define RFM95_ss 5
#define RFM95_rst 14
#define RFM95_dio0 2


//--- Constantes --------------------------------------------------------

// NOTE: dans un scénario réel, ces données devraient etre dans un fichier sécurisé.
byte mac[] = { 0x24, 0x6f, 0x28, 0x01, 0x01, 0xB0 };
const char* myWriteAPIKey = "8ID5AO60YD6V94W9";

Ecran monEcran;

WiFiClient client;

//Paramètres de la communication LoRa:
const uint32_t BAND = 908.1E6;  // Hz
const uint8_t LoRaSyncWord = 0xF8;
const uint8_t LoRaSpreadFactor = 7;
const uint32_t LoRaSB = 125E3;
const uint8_t LoRaCodingRate = 5;

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
  LoRa.setSyncWord(LoRaSyncWord);

  Serial.println("> RFM95: set CRC");
  LoRa.enableCrc();

  //The spreading factor (SF) impacts the communication performance of LoRa, which uses an SF between 7 and 12. A larger SF increases the time on air, which increases energy consumption, reduces the data rate, and improves communication range. For successful communication, as determined by the SF, the modulation method must correspond between a transmitter and a receiver for a given packet.
  Serial.println("> RFM95: SF");
  LoRa.setSpreadingFactor(LoRaSpreadFactor);

  Serial.println("> RFM95: BAND");
  LoRa.setSignalBandwidth(LoRaSB);

  Serial.println("> RFM95: CR");
  LoRa.setCodingRate4(LoRaCodingRate);
  // --- FIN INITIALISATION LORA

  // --- DEBUT INITIALISATION WIFI
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  esp_wifi_set_mac(WIFI_IF_STA, mac);  //requis pour changer la MAC du Wifi
  WiFi.begin("MQTT", "");

  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  // --- FIN INITIALISATION WIFI

  ThingSpeak.begin(client);  // Initialize ThingSpeak

  Serial.println("> Terminé");
}  //Fin de setup()

//--- Loop -----------------------------------------------------

void loop() {
  monEcran.refresh();

  // Connect ou reconnect à la WiFi. 
  // Bloque jusqu'a ce qu'une connection valide soit établie.
  // Réessaye chaques 5 secondes une nouvelle tentative de connection.
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect");
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin("MQTT", "");
      Serial.print("Connecting to WiFi ..");
      int retry_left = 5;
      while (WiFi.status() != WL_CONNECTED && retry_left > 0) {
        Serial.print('.');
        delay(1000);
        retry_left--;
      }
      if (WiFi.status() != WL_CONNECTED) {
        Serial.print('failed');
      }
    }
    Serial.println("\nConnected.");
  }

  // lecture du packet LoRa
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

      // Envoie des valeurs au serveur infonuagiaque ThingSpeak
      ThingSpeak.setField(1, int(p_u.packet.device_id));
      ThingSpeak.setField(2, p_u.packet.humidity);
      ThingSpeak.setField(3, p_u.packet.temperature);
      ThingSpeak.setField(4, p_u.packet.light_level);
      ThingSpeak.setField(5, int(p_u.packet.message_id));
      ThingSpeak.setField(6, int(p_u.packet.ttl));
      int x = ThingSpeak.writeFields(1, myWriteAPIKey);

      if (x == 200) {
        Serial.println("Channel update successful.");
      } else {
        Serial.println("Problem updating channel. HTTP error code " + String(x));
      }
    }

    // Écrire RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
}  //Fin de loop()