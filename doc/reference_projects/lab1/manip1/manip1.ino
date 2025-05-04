/*  --- Entête principale -- information sur le programme
 *   
 *  Programme:        Technique en Génie Physique
 *  Auteur:           Félix St-Gelais, Gregoire Trivale
 *  Pltform matériel: ESP32 DOIT KIT V1 - protoTPhys
 *  Pltform develop : Arduino 2.0.3/1.8.19)
 *  Description:      Ce script permet a deux plateformes protoTphys de comuniquer en impriment l'entrée 
 *                    serielle sur une carte sur la sortie sérielle de l'autre.
 *  Notes:
 *   La longue latence de la comunication est due à l'utilisation de la fonction readString sur 
 *   les objets de communication
 */


#include <HardwareSerial.h>

// cet objet pilote la comunication sérielle avec l'autre carte protoTphys. Il utilise le port série #2.
HardwareSerial serial_UART = HardwareSerial(2);

void setup() {
  Serial.begin(9600);
  serial_UART.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (Serial.available()) {
    String data = Serial.readString();
    serial_UART.println(data);
  }
  if (serial_UART.available()) {
    String data = serial_UART.readString();
    Serial.println(data);

    // on alume une LED sur la carte protoTphys pour signaler qu'elle recoit des données
    digitalWrite(LED_BUILTIN, HIGH);  
    delay(1000);                      
    digitalWrite(LED_BUILTIN, LOW);
  }
}
 