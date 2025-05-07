// 244-470-AL - Communication des Objets 4.0
// Hiver 2025 - Laboratoire 04 - Partie 2 - Simple Sketch MQTT client
/*
 *  Par: Yh - janvier 2022 - revu 27 février 2023 - révisé en mars 2024 - révisé en avril 2024 pour H25
 * 
 * Au 26 janvier 2022: changé pour envoyer un format JSON. Attention, le buffer MQTTClient est
 * limité à 128 bytes. Il y a déjà 9 ou 10 bytes réservés pour le système, ce qui laisse seulement
 * env 118 bytes de "payload". Sinon on a une erreur de type LWMQTT_BUFFER_TOO_SHORT.
 *
 * Révisé par Yh au 9 mars 2025 pour la session H2025:
 *   Retour à la librairie de base: PubSubclient au lieu de MQTT de Joel
 */

#include <SPI.h>
#include <Ethernet.h>      // arduino-ethernet v2.0.1
#include <PubSubClient.h>  // v2.8  https://pubsubclient.knolleary.net/
#include <ArduinoJson.h>   //Librairie de manipulation document JSON v6.21.0
#include "secrets.h"       //Façon de "protéger" les informations sensibles

//Version du programme :
#define VERSION "2.1.8"

// Broches pour la connectivité SPI au module Ethernet
#define SS 5  //broche 5 sur le module DFRobot (shield Ethernet) + protoTPhys

//À compléter avec l'adresse mac que l'on vous fournira
//format 6 octets séparés par une virgule, ex: [octet],[octet],...
byte mac[] = MAC;

// Lier les définitions disponibles dans secrets.h aux variables suivantes:
// Configuration de la connectivité à un service MQTT:
// *** SVP: UTILISER LES DÉFINITIONS DU FICHIER secrets.h ***//
const char mqttHostProvider[] = MQTT_BROKER;  // adresse du serveur MQTT (FQDN)
const char mqttUsername[] = MQTT_USERNAME;    // usager du service MQTT de shiftr.io
const char mqttDevice[] = MQTT_DEVICEID;      // nom du dispositif connectant au service (etudiantXX)
const char mqttUserPass[] = MQTT_TOKEN;       // token du dispositif (associé au device "etudiantXX")
const char mqttPublishTo[] = MQTT_pub;        // Nom du canal (topic) d'abonnement (peut commencer par "/")
const char mqttSubscribeTo[] = MQTT_sub;      // Nom du canal (topic) de publication (peut commencer par "/")
const uint16_t mqttPort = MQTT_PORT;          // numéro du port d'écoute TCP du broker MQTT

//Compteurs de messages envoyés et recus:
uint16_t recMsgCount = 0;
uint16_t senMsgCount = 0;

//Mettre ici la déclaration d'un objet client TCP dont le nom devra remplacer '[nom_objet_Client_MQTT]':
EthernetClient ethernet;  //Moyen de transport
PubSubClient mqtt;        //objet Client_MQTT

//Minuterie entre les publications automatiques: 20 secondes
const uint32_t msgInterval = 20000;
uint32_t lastMillis = 0;

//Minuterie de vérification de la connectivité: 10 secondes
uint32_t checkEtherStatusTimer = 0;
const uint32_t checkEtherStatusDelay = 10000;

//Objet JSON pour créer du contenu à publier
StaticJsonDocument<250> Jsondoc;

//Routine d'établissement de la connectivité au service MQTT:
bool connectMQTT() {
  bool retCode = false;

  Serial.print("connecting...");

  // Tant que la connexion avec MQTT broker n'est pas établie/confirmée
  // Synopsis: boolean connect(clientID, username, password);
  while (!mqtt.connect(mqttHostProvider, mqttUsername, mqttUserPass)) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  //Réussi! On s'abonne donc au topic (à vous de déterminer le nom du topic, selon mqttSubscribeTo):
  retCode = mqtt.subscribe(mqttSubscribeTo);

  return retCode;
}

// Routine de traitement d'un message recu, appellé de façon asynchrone, par [objet].loop()
void messageReceived(char* topic, byte* payload, unsigned int length) {
  recMsgCount++;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // TRÈS IMPORTANT:
  // Note: Do not use the clientMQTT in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `clientMQTT.loop()`.
}

//Routine d'affichage de l'adresse MAC Ethernet:
void getNPrintMAC(void) {
  byte macBuffer[6];               // create a buffer to hold the MAC address
  Ethernet.MACAddress(macBuffer);  // fill the buffer
  for (byte octet = 0; octet < 6; octet++) {
    if (macBuffer[octet] > 0x0f) Serial.print('0x');
    else Serial.print('0x0');
    Serial.print(macBuffer[octet], HEX);
    if (octet < 5) {
      Serial.print('-');
    }
  }
  Serial.println("");
}

// --- Setup et Loop ----------------------------------------------------------------

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {};  //Attendre que le port série soir fin prêt
  delay(5000);         // Pour permettre à Windows de connecter le moniteur série
  Serial.println("ESP32 client MQTT - Version du code: v" + String(VERSION));
  Serial.println("** Initialisation **");

  //Affiche le nom du programme au démarrage :
  Serial.println("> ESP32 Client Ethernet.");

  // Initialisation de l'Ethernet et validation:
  Ethernet.init(SS);
  if (Ethernet.begin(mac) == 0) {  // référence: https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.begin/
    Serial.println("> Echec de configuration DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("> Blindage non disponible.  Désolé cette application requiert le réseau Ethernet. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("> Cable Ethernet non-connecté. SVP, vérifiez de nouveau est redémarrer (RESET)");
    }
    while (true) { delay(1); }  //boucle infinie - on ne fait plus rien si on parvient ici.
  }


  //Affiche dans le moniteur série l'adresse MAC
  Serial.print("> Adresse MAC : ");
  getNPrintMAC();

  //Affiche dans le moniteur série l'adresse IP attribuée par le réseau
  Serial.print("> Adresse IP : ");
  Serial.println(Ethernet.localIP());
  Serial.print("\n> Branchement réussi avec le réseau: ");
  Serial.println("Ethernet");
  Serial.println("> Préparation de la connexion au broker MQTT: " + String(mqttHostProvider));

  //Attacher (bind) le Client_MQTT à son moyen de transport (TCP):
  mqtt.setClient(ethernet);

  //Indiquer à la librairie à quel serveur MQTT se connecter ainsi que le port de service
  mqtt.setServer(mqttHostProvider, mqttPort);

  //Initialisation de la librairie: quelle fonction appeller lorsqu'un msg est recu:
  mqtt.setCallback(messageReceived);

  // Au besoin: clientMQTT.setBufferSize([code]);  //Par défaut 128 bytes, si non spécifé

  // Connexion au service MQTT, si ca passe, on publie sur le topic un premier message.
  if (connectMQTT()) {
    mqtt.publish(mqttPublishTo, "Bonjour, je suis prêt");
  }
}  //Fin de setup

void loop() {
  //Élément obligatoire pour vérifier les transactions MQTT:
  mqtt.loop();

  //Vérification à intervalle régulière de la connectivité du transport et rétablir s'il y a lieu
  if ((millis() - checkEtherStatusTimer) > checkEtherStatusDelay) {
    checkEtherStatusTimer = millis();
    if (Ethernet.maintain() == 0) {
      if (!mqtt.connected()) {
        connectMQTT();
      }
    } else Serial.println("> Probleme de transport");
  }

  // publication d'un message à intervalle régulière, basé sur la variable 'msgInterval'.
  if (millis() - lastMillis > msgInterval) {
    lastMillis = millis();

    //Teste si la connectivité du client MQTT est tjrs là:
    if (mqtt.connected()) {


      // Confection du message à envoyer
      // Ici vous pouvez élaborer un objet JSON contenant des champs d'information
      // (note: vous avez un objet déclaré en global: Jsondoc;  pour le vider: Jsondoc.clear())
      // Par exemple: un compteur de messages recus ou envoyés
      // Une information sur la version logicielle de ce code
      // L'état de la DEL, du bouton, etc
      // La quantitée mémoire restante du ESP32:   ESP.getFreeHeap()
      Jsondoc.clear();



      //Lorsque votre msg JSON est complet, le sérialiser dans une chaine de caractères
      String msg = "{\"hello from\": \"" + String(MQTT_DEVICEID) + "\" }";
      // int dataSize = serializeJson(doc, msg);


      //Publication du message sur un topic:
      if (mqtt.publish(mqttPublishTo, msg.c_str())) {
        recMsgCount += 1;  //Incrémente le compteur de message envoyé. Affiche l'information sur le moniteur série:
                       Serial.println("> Publication msg#" + String(recMsgCount) + " au topic " + String(mqttPublishTo));
      } else {
        //En cas d'erreur d'envoi, on affiche les détails:
        Serial.println("> ERREUR publication");
      }  //Fin if-then-else publication MQTT
    } else {
      Serial.println("> Ne peut procéder: client MQTT non-connecté!");
    }  //Fin if-then-else test de connectivité MQTT
  }    //Fin de la minuterie
}  //Fin de loop