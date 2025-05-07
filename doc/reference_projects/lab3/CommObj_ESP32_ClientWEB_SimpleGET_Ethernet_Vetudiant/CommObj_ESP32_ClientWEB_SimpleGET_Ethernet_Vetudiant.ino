// 244-470-AL - Communication des Objets 4.0
// Hiver 2024 - Laboratoire 03 - Partie 1 - Simple client Web

//Etudiant(e)s: vous devez étudier ce code et le compléter aux endroits indiqués par [CODE]

#include <SPI.h>
//#define USE_W5100 true           //Requis pour utiliser le shield DFRobot. Pas requis avec un W5500 connecté SPI.
#include <Ethernet.h>           //Lib arduino-ethernet v2.0.2
#include <ArduinoHttpClient.h>  //Librairie wrapper HTTP: arduinoHttpClient v0.6.1
#include <ArduinoJson.h>        //Librairie de manipulation document JSON v7.3.0

#include <ProtoTGP.h>  //Pour utiliser la librairie ProtoTGP

//Version du programme :
#define VERSION "1.9.5"

// Broche pour la connectivité SPI au module Ethernet
// SS = Slave Select = Chip Select
#define SS 5  //module W5500 sur le port SPI de la protoTPhys

byte mac[] = { 0x24, 0x6f, 0x28, 0x01, 0x01, 0xB0 };

ProtoTGP proto;
// EthernetClient ETH_client;
// HttpClient client = HttpClient(ETH_client, "api.open-notify.org");
long longitude, latitude, when_timestamp;

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

  //Si tout a bien été, on continue. Affiche dans le moniteur série l'adresse MAC
  Serial.print("> Adresse MAC : ");
  getNPrintMAC();

  //Affiche dans le moniteur série l'adresse IP attribuée par le réseau
  Serial.print("\n> Adresse IP : ");
  Serial.println(Ethernet.localIP());

  Serial.println("\n> Branchement réussi avec le réseau Ethernet");

  //Réalise le travail d'aller chercher du data à l'URL donné dans la fonction:

  getURLServeur();

  Serial.println("> Fin de Setup");

}  //Setup

void loop() {
  proto.refresh();
  //[CODE] lire, décortiquer et afficher à intervalle
  //    régulière de 1 minute la position de la SSI (1 minute) à l'écran OLED.
  Serial.println("requesting... ");
  // auto success = client.get("/iss-now.json");
  // Serial.println("xzxxxx "+ String(success));

  // // read the status code and body of the response
  // int statusCode = client.responseStatusCode();
  // String response = client.responseBody();

  // JsonDocument doc;
  // deserializeJson(doc, response);

  getURLServeur();


  Serial.print("longitude: ");
  Serial.println(longitude);
  Serial.print(" latitude: ");
  Serial.println(latitude);

  proto.ecran.ecrire("lon: " + String(longitude), 1);
  proto.ecran.ecrire("lat: " + String(latitude), 2);
  proto.ecran.ecrire("time: " + String(when_timestamp), 3);

  Serial.println("Wait five seconds");
  delay(5000);
}  //Loop
