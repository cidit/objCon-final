/*  --- Entête principale ---
 *   
 *  Programme:        EXAMEN
 *  Date:             (date initiale de création)
 *  Auteur:           FSTG
 *  Pltform matériel: ESP32 DOIT KIT V1 - protoTPhys
 *  Pltform develop : Arduino 2.0.3
 *  Description:      (brève description de ce qu'accompli ce programme/sketche (+lignes si requis)
 *  Fonctionnalités:  (fonctionnalités principales)

 *  Notes:
 *  - Ne pas oublier de rediger le fichier "secrets.h".
 *    
 *  Inspirations et credits:
 *  - laboratoire 4, manipulation 3 du cours d'objets connectés donné par Yanick Heyneman
 *  - Version de modèle: B - Yh H23
 */


//--- Déclaration des librairies (en ordre alpha) -----------------------

#include <SPI.h>
#include <Ethernet.h>      // arduino-ethernet v2.0.1
#include <PubSubClient.h>  // v2.8  https://pubsubclient.knolleary.net/
#include <ArduinoJson.h>   //Librairie de manipulation document JSON v6.21.0
#include <Decodeur.h>
#include "secrets.h"  //Façon de "protéger" les informations sensibles

//-----------------------------------------------------------------------



//--- Definitions -------------------------------------------------------

//*** Lister ici les broches qui seront utilisées dans ce programme
//*** ex: #define ONE_WIRE_BUS 4   //Ceci est un exemple
//*** ex: #define TEMPERATURE_PRECISION 9   //Ceci est un exemple

#define brocheLecture 3

//-----------------------------------------------------------------------


//--- Declaration des objets --------------------------------------------

EthernetClient ethernet;  //Moyen de transport
PubSubClient mqtt;        //objet Client__MQTT
Decodeur cmdline(&Serial);
StaticJsonDocument<250> json;

//-----------------------------------------------------------------------


//--- Constantes --------------------------------------------------------

// constexpr uint8_t SS = 5; // pas necessaire parce que definie par default par arduino
constexpr uint8_t _MQTT_RETRY_DELAY = 250;
constexpr uint8_t _MQTT_CONNECTION_TIMEOUT = 2000;
constexpr uint8_t _CHECK_ETHERNET_STATUS_DELAY = 2000;

constexpr String PUBLISH_A = "demande/liste"

//-----------------------------------------------------------------------


//--- Variables globales ------------------------------------------------

//-----------------------------------------------------------------------


//--- Section des routines specifiques ----------------------------------

//*** ENTETE DE ROUTINE/FONCTION (pour CHACUNE)

/* 
 * Nom: 
 * Fonction: 
 * Argument(s) réception: (rien)
 * Argument(s) de retour: (rien)
 * Modifie/utilise (globale):
 * Notes:  (spécial, source, amélioration, etc)
 * 
 */

void setup_ethernet() {
  Ethernet.init(SS);

  byte mac[] = _ETHERNET_MAC;
  if (Ethernet.begin(mac) == 1) {
    // référence: https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.begin/
    return;
  }

  Serial.println("> Echec de configuration DHCP");
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("> Blindage non disponible.  Désolé cette application requiert le réseau Ethernet. :(");
  } else if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("> Cable Ethernet non-connecté. SVP, vérifiez de nouveau et redémarrez (RESET)");
  }

  // etat invalide; softlock du programme.
  while (true)
    ;
}

void setup_mqtt() {
  mqtt.setClient(ethernet);

  //Indiquer à la librairie à quel serveur _MQTT se connecter ainsi que le port de service
  // seulement vraiment utile pour le port vu que le host est spécifié a la connection
  mqtt.setServer(_MQTT_BROKER, _MQTT_PORT);

  //Initialisation de la librairie: quelle fonction appeller lorsqu'un msg est recu:
  mqtt.setCallback(handle_message);
}

void handle_message(const char* topic, byte* payload, uint32_t length) {
  json.clear();
  auto error = deserializeJson(json, payload, length);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
  }

  // TODO: add things here.
}

bool connect_mqtt(const uint32_t timeout = 0) {
  const auto end = millis() + timeout;
  const auto always_retry = timeout == 0;
  bool retry, success = false;

  Serial.print("connecting.");

  do {
    Serial.print('.');
    auto now = millis();
    retry = always_retry || now < end;
    success = mqtt.connect(_MQTT_BROKER, _MQTT_USERNAME, _MQTT_TOKEN);
    delay(_MQTT_RETRY_DELAY);
  } while (!success && retry);

  if (!success) {
    Serial.println();
    Serial.println("failed to connect to the host provider: timed out.");
    return false;
  }

  do {
    Serial.print('.');
    auto now = millis();
    retry = always_retry || now < end;
    success = mqtt.subscribe(_MQTT_SUBSCRIBE_TOPIC);
    delay(_MQTT_RETRY_DELAY);
  } while (!success && retry);

  if (!success) {
    Serial.println();
    Serial.println("failed to subscribe to the topic: timed out.");
    return false;
  }

  Serial.println();
  Serial.println("connected!");
  return true;
}

bool is_comm_ok() {

  // Si on est bon, on est bon! on quitte plus tot.
  if (mqtt.connected()) {
    return true;
  }

  // Sinon, on cherche ce qui ne marche pas et on résoud si on peut.

  // on test tout périodiquement pour éviter de bouffer des resources pour riens.
  // La logique du timer est fait de sorte qu'on sort plus tot de la fonction si on n'a riens a faire
  // on sort aussi tot que possible.
  static auto checkEtherStatusTimer = 0;
  bool time_to_check_connection = (millis() - checkEtherStatusTimer) < _CHECK_ETHERNET_STATUS_DELAY;
  if (time_to_check_connection) {
    checkEtherStatusTimer = millis();
  } else {
    return false;
  }

  if (Ethernet.maintain() != 0) {
    Serial.println("> Probleme de transport");
    return false;
  }

  if (!connect_mqtt(_MQTT_CONNECTION_TIMEOUT)) {
    Serial.println("> Probleme de connection avec _MQTT");
  }

  return true;
}

bool publish(const char* topic, String payload) {
  if (mqtt.publish(topic, payload.c_str())) {
    Serial.println("> Publication au topic " + String(topic));
  } else {
    //En cas d'erreur d'envoi, on affiche les détails:
    Serial.println("> ERREUR publication");
  }
}

void handle_command() {
  String payload, command = cmdline.getCommandString();
  json.clear();

  // TODO: add things here.
  auto topic = "TODO_TOPIC";

  serializeJson(json, payload);
  publish(topic, payload);
}

//-----------------------------------------------------------------------

//--- Setup et Loop -----------------------------------------------------

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println(__FILE__);

  setup_ethernet();
  setup_mqtt();

  if (connect_mqtt()) {
    // signal de disponibilité.
    publish(_MQTT_PUBLISH_TOPIC, "Bonjour, je suis prêt");
  }
}  //Fin de setup()

void loop() {
  mqtt.loop();
  cmdline.refresh();

  if (!is_comm_ok()) {
    return;
  }

  if (cmdline.isAvailable()) {
    handle_command();
  }
}  //Fin de loop()

//-----------------------------------------------------------------------
