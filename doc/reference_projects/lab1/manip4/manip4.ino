/*  --- Entête principale -- information sur le programme
 *   
 *  Programme:        Technique en Génie Physique
 *  Auteur:           Félix St-Gelais, Gregoire Trivale
 *  Pltform matériel: ESP32 DOIT KIT V1 - protoTPhys
 *  Pltform develop : Arduino 2.0.3/1.8.19)
 *  Description:      Ce script permet reprend la manipulation 2 avec la communication sérielle RS-485
 *  Notes:
 *   Le gabarit du code a été rédigé par Yanick Heyneman
 */

//Consulter: https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/HardwareSerial.h
#include <HardwareSerial.h>
#include <Decodeur.h>  //Pour utiliser la librairie Decodeur

// On instancie 2 objets de type UART pour communiquer
HardwareSerial SerialRemote(2);
HardwareSerial SerialLocal(0);

// Déclarer ici 2 objets TGP_Decodeur: chacun son port série
Decodeur Serial_decodeur_Remote(&SerialRemote);
Decodeur Serial_decodeur_Local(&SerialLocal);


//Quelques définitions
const int BaudRate = 9600;
const int transmit_pin = 18;
const int RX0_pin = 3;   //broche RX du port série 0
const int TX0_pin = 1;   //broche TX du port série 0
const int RX2_pin = 16;  //broche RX du port série 2
const int TX2_pin = 17;  //broche TX du port série 2

// Mode de fonctionnement du port série: 8 bits de data, aucune parité, 1 bit d'arrêt
// consulter la définition SerialConfig de HardwareSerial.h pour utiliser la bonne définition de configuration
const int modeSerial = SERIAL_8N1;  //Mode de fonctionnement du port série: 8N1

//Broches des actions (2 DELs de la plaque protoTPhys)
const int localPin[2] = { 2, 4 };

//Broche de lecture du potentiometre:
const int potPin = 32;

void setup() {

  //Initialisation des 2 ports série: l'un local et l'autre est distant
  SerialLocal.begin(BaudRate, modeSerial, RX0_pin, TX0_pin);
  SerialRemote.begin(BaudRate, modeSerial, RX2_pin, TX2_pin);
  // activation du mode rs485
  SerialRemote.setPins(-1, -1, -1, transmit_pin);
  SerialRemote.setMode(UART_MODE_RS485_HALF_DUPLEX);

  //Initialisation des broches DEL et potentio
  pinMode(localPin[0], OUTPUT);
  pinMode(localPin[1], OUTPUT);
  pinMode(potPin, INPUT);  //Facultatif

  //Signaler à l'usager du PC que le ESP32 est prêt
  SerialLocal.println("Ready!");

  delay(1000);  //Attendre que tout soit bien configuré et prêt à l'emploi.
}

void loop() {
  //Procéder au refraichissement du contenu des objets tgp décodeur:
  Serial_decodeur_Remote.refresh();
  Serial_decodeur_Local.refresh();

  //Est-ce que l'objet tgp_decodeur local a recu de quoi?
  if (Serial_decodeur_Local.isAvailable()) {
    //Récupérer la commande et le nombre d'arguments dans des variables:
    char cmdLocale = Serial_decodeur_Local.getCommand();
    int nbArgLocale = Serial_decodeur_Local.getArgCount();

    //Commencer à indiquer à l'usager de la réception d'une commande locale
    SerialLocal.print("cmdLoc: ");

    //Traitement et validation pour la commande "D":
    if (
      cmdLocale == 'D') {
      if (nbArgLocale == 2) {                           // et que la commande possède 2 arguments...
        int numDel = Serial_decodeur_Local.getArg(0);   //Récupérer l'argument correspondant un numéro de la DEL
        int etatDel = Serial_decodeur_Local.getArg(1);  //Récupérer l'argument correspondant à l'état désirée de la DEL

        //Valider les valeurs avant d'envoyer
        if (numDel > -1 && numDel < 2)
          if (etatDel > -1 && etatDel < 256) {
            //Forger et envoyer vers le ESP32 distant la commande de changer l'état de la DEL

            SerialRemote.println();
            SerialRemote.println("D " + String(numDel) + " " + String(etatDel));
          }

        //Confirmer l'envoi de la commande à l'usager local:
        SerialLocal.println("Cmd D envoyée");
      }
    }

    //Si la commande est "P" (pour demander la lecture du potentiomètre distant)
    if (cmdLocale == 'P') {
      //Envoyer la commande de lire le potentiomètre vers le remote

      SerialRemote.println();
      SerialRemote.println("P");

      //Confirmer l'envoi de la commande à l'usager local:
      SerialLocal.println("Cmd P envoyée");
    }

    // cmd de test pour obtenir la valeure local du potentiometre
    if (cmdLocale == 'L') {
      SerialLocal.println(String(analogRead(32)));
    }
  }

  //Est-ce que l'objet tgp_decodeur distant a recu de quoi?
  if (Serial_decodeur_Remote.isAvailable()) {

    //Récupérer la commande et le nombre d'arguments dans des variables:
    char cmdRemote = Serial_decodeur_Remote.getCommand();
    int nbArgRemote = Serial_decodeur_Remote.getArgCount();

    //Commencer à indiquer à l'usager local de la réception d'une commande provenent du distant:
    SerialLocal.print("cmdRem: ");

    //Traitement et validation pour la commande "D":
    if (cmdRemote == 'D' && nbArgRemote == 2) {

      int numDel = Serial_decodeur_Remote.getArg(0);   //Récupérer l'argument correspondant un numéro de la DEL
      int etatDel = Serial_decodeur_Remote.getArg(1);  //Récupérer l'argument correspondant à l'état désirée de la DEL

      //Valider les arguments
      if (numDel > -1 && numDel < 2)
        if (etatDel > -1 && etatDel < 256) {
          digitalWrite(localPin[numDel], etatDel > 0 ? HIGH : LOW);

          //Retourner un accusé de réception à l'envoyeur
          SerialRemote.println();
          SerialRemote.println("d");

          //Informer l'usager local:
          SerialLocal.print("cmd D recue");
        }
    }

    //Cas d'une demande de lecture du pot local
    if (cmdRemote == 'P' && nbArgRemote == 0) {
      //Répondre avec "p [valeur_numérique_du_pot]"
      SerialLocal.println("(local) " + String(analogRead(potPin)));

      SerialRemote.println();
      SerialRemote.println("P " + String(analogRead(potPin)));
      //Indiquer à l'usager local qu'on a envoyé une réponse:
      SerialLocal.print("valPot envoyé");
    }
    //Cas de la réception d'une réponse de la commande remote
    if (cmdRemote == 'd') {
      //Confirmer à l'usager local que la transaction a été recue et traitée:
      SerialLocal.print("Etat Del changé");
    }
    //Cas de la réception d'une réponse de la demande de la valeur du potentio remote
    if (cmdRemote == 'P' && nbArgRemote == 1) {
      SerialLocal.println("got here");
      //Confirmer à l'usager local la valeur recue:
      SerialLocal.print("Valeur pot remote=" + String(Serial_decodeur_Remote.getArg(0)));
    }
    //Finaliser l'affichage:
    SerialLocal.println(".");
  }
}
// -- Yh - pour le cours 244-470-AL - version H25