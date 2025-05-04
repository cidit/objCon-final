/*  --- Entête principale -- information sur le programme
 *  Programme:        Récepteur laboratoire LoRa
 *  Date:             9/04/2025
 *  Auteur:           TRIVALLE et St-Gelais
 *  Pltform matériel: (plateforme matérielle et version - ESP32 DOIT KIT V1 - protoTPhys)
 *  Description:      Recevoir un message via wifi et l'affichier sur le moniteur série
 */

/* --- Materiel et composants -------------------------------------------
 * - DHT11: capteur de température et de d'humidité
 * - LDR: capteur de de luminosié
*/

//--- Déclaration des librairies (en ordre alpha) -----------------------

#include "DHT.h"
#include <SPI.h>
#include <LoRa.h>

//--- Definitions -------------------------------------------------------

// Paramètre de communication ESP32 et module PMOD-RFM95 V2:
#define RFM95_ss 5
#define RFM95_rst 14
#define RFM95_dio0 2

#define LDR 35  // Pin pour le photo-transistor

// Paramétre du DHT
DHT dht(19, DHT11);  // Définition de la pin du DHT11
float temperatureC;  // Obtenir la température en degré Celsius

//--- Constantes --------------------------------------------------------

//Paramètres de la communication LoRa:
const uint32_t BAND = 908.1E6;  // Hz
const uint8_t LoRasyncWord = 0xF8;
const uint8_t LoRaSF = 7;
const uint32_t LoRaSB = 125E3;
const uint8_t LoRaCR = 5;

// Déclaration des minuteries
const uint32_t msgInterval = 30000;
uint32_t lastMillis = 0;
const uint32_t retry_TIMER = 5000;
uint32_t lastMillis_retry_timer = 0;

// Déclaration du compteur
int counter = 0;

// Permettre d'envoyer un message directement et gestion d'erreur si le DHT ne renvoi pas de valeur
bool retry = true;
bool retry_MSG = false;
bool first_pass = true;

// représentation d'un message LoRa
struct packet_frame {
  float light_level,
    humidity,
    temperature;
  uint32_t ttl,  // time-to-live, equivalent au uptime.
    message_id,
    device_id;
};

// utilitaire de conversion de packet_frame en tableau de bytes.
union packet_union {
  packet_frame packet;
  uint8_t raw[sizeof(packet_frame)];
};

//--- Setup -----------------------------------------------------

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("LoRa Dump Registers");

  dht.begin();
  pinMode(LDR, INPUT);

  // --- DEBUT INITIALISATION LORA
  // Paramètre LoRa envoyeur module
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
  bool is_time = millis() - lastMillis > msgInterval

                 // Noter: la condition requiert soit que la minterie s'ecoule ou que le flag de reaissait soit monté.
                 if (is_time || retry) {
    lastMillis = millis();

    packet_union packet_u;

    // Lecteur des valeurs du DHT11
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Boucle permettant de savoir si la valeur du DHT11 est valide, sinon refait recommence jusqu'à obtenir une valeur valide
    if (isnan(humidity) || isnan(temperature)) {

      if (retry_MSG == false) {
        Serial.println("Failed to read from DHT sensor!");
      }

      if (millis() - lastMillis_retry_timer > retry_TIMER || first_pass) {
        Serial.println("retry...");
        lastMillis_retry_timer = millis();
        first_pass = false;
      }

      retry = true;
      retry_MSG = true;
      return;
    }

    retry = false;
    retry_MSG = false;

    // Rentrer les valeurs dans l'UNION
    packet_u.packet.device_id = 777;
    packet_u.packet.humidity = humidity;
    packet_u.packet.light_level = analogRead(LDR);
    packet_u.packet.message_id = counter;
    packet_u.packet.temperature = temperature;
    packet_u.packet.ttl = millis();

    // Message sur le moniteur afin de voir les valeurs QUI sont envoyées
    Serial.println("Sending packet: ");
    Serial.print("Packet = [( Device ID = ");
    Serial.print(packet_u.packet.device_id);
    Serial.print("),( Humidity = ");
    Serial.print(packet_u.packet.humidity);
    Serial.print("),( Temperature = ");
    Serial.print(packet_u.packet.temperature);
    Serial.print("),( Ligh level = ");
    Serial.print(packet_u.packet.light_level);
    Serial.print("),( Message ID = ");
    Serial.print(packet_u.packet.message_id);
    Serial.print("),( TTL = ");
    Serial.print(packet_u.packet.ttl);
    Serial.print(")]");
    Serial.println();
    Serial.print("Count = ");
    Serial.println(counter);
    Serial.println("");

    // emission du packet LoRa
    int packet_size = sizeof(packet_frame);
    LoRa.beginPacket();
    LoRa.write(packet_u.raw, packet_size);
    LoRa.endPacket();

    counter++;
  }
}  //Fin de loop()
   //-----------------------------------------------------------------------