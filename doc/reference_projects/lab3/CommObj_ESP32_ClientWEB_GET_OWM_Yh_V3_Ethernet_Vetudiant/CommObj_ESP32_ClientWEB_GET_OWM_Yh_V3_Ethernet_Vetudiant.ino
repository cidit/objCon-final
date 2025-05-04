// 244-470-AL - Communication des Objets 4.0
// Hiver 2025 - Laboratoire 03 - Partie 2 - Meteo OWM

#include <SPI.h>
#define USE_W5100 true          //Requis pour utiliser le shield DFRobot. Pas requis avec un W5500 connecté SPI.
#include <Ethernet.h>           //Lib arduino-ethernet v2.0.2
#include <ArduinoHttpClient.h>  //Librairie wrapper HTTP: arduinoHttpClient v0.6.1
#include <ArduinoJson.h>        //Librairie de manipulation document JSON v7.3.0
//[CODE] inclure le fichier secret.h, à modifier pour VOTRE utilisation
#include "secrets.h"

#include <ProtoTGP.h>  //Pour utiliser la librairie ProtoTGP

// SOURCE D'INSPIRATION:
// https://randomnerdtutorials.com/esp32-http-get-open-weather-map-thingspeak-arduino/

//Version du programme :
#define VERSION "1.8.1"

// Broches pour la connectivité SPI au module Ethernet
#define SS 5  //module W5500 sur le port SPI de la protoTPhys

byte mac[] = { 0x24, 0x6f, 0x28, 0x01, 0x01, 0xB0 };

ProtoTGP proto;
//Variables globales à utiliser (initialisées à des valeurs arbitraires)
float temperature = -45.5, humidite = 0.5, pression = 130.5, vitVent = 1.0, dirVent = 1.0;

void setup() {
  Serial.begin(115200);  //Démarrage du port série
  proto.begin();
  while (!Serial) {};  //Attendre que le port série soir fin prêt
  delay(5000);         // Pour permettre à Windows de connecter le moniteur série

  //Affiche le nom du programme au démarrage :
  Serial.println("> ESP32 Client Ethernet. Version du code = " + String(VERSION));

  //Paramétrisation de la broche SS avec la carte Ethernet:
  //[CODE] - consulter: https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.init/
  Ethernet.init(SS);

  Serial.println("> Initialization Ethernet IP via DHCP:");

  if (Ethernet.begin(mac) == 0) {  // référence: https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.begin/
    Serial.println("> Echec de configuration DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("> Blindage non disponible.  Désolé cette application requiert le réseau Ethernet. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("> Cable Ethernet non-connecté. SVP, vérifiez de nouveau est redémarrer (RESET)");
    }
    while (true) { delay(1); }  //boucle infinie - on ne fait plus rien si on parvient ici.
  }

  Serial.println(checkEthernetModule());

  //Affiche dans le moniteur série l'adresse MAC
  Serial.print("> Adresse MAC : ");
  getNPrintMAC();

  //Affiche dans le moniteur série l'adresse IP attribuée par le réseau
  Serial.print("\n> Adresse IP : ");
  Serial.println(Ethernet.localIP());

  Serial.println("\n> Branchement réussi avec le réseau Ethernet");

  if (getURLServeur()) {  //C'est à cet appel que tout se joue
    Serial.println("> Tout a bien été - Résultats:");

    //Afficher ici les données obtenues.. par exemple:
    Serial.println("\tTemp = " + String(temperature, 1) + "C");
    //Et les autres mesures demandées...
    // Consulter l'API de OWM afin d'obtenir des détails sur les unités des valeurs.

    //Et à ce point, faites afficher l'information à l'écran OLED

  } else {
    Serial.println("> Un problème est survenu...");
  }

}  //Setup

void loop() {
  proto.refresh();
  
  //[CODE]: lire et afficher à intervalle régulière OWM (2 minutes)
  getURLServeur();


  
  Serial.println("temperature: " + String(temperature));
  Serial.println("humidite: " + String(humidite));
  Serial.println("pression: " + String(pression));
  Serial.println("vitVent: " + String(vitVent));
  Serial.println("dirVent: " + String(dirVent));

  
  proto.ecran.ecrire("temperature: " + String(temperature), 1);
  proto.ecran.ecrire("humidite: " + String(humidite), 2);
  proto.ecran.ecrire("pression: " + String(pression), 3);
  proto.ecran.ecrire("vitVent: " + String(vitVent), 4);
  proto.ecran.ecrire("dirVent: " + String(dirVent), 5);

  delay(60*1000);
}  //Loop
