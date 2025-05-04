/*  --- Entête principale -- information sur le programme
 *   
 *  Programme:        Technique en Génie Physique
 *  Auteur:           Félix St-Gelais, Gregoire Trivale
 *  Pltform matériel: ESP32 DOIT KIT V1 - protoTPhys
 *  Pltform develop : Arduino 2.0.3/1.8.19)
 *  Description:      Ce script reprend la manipulation 1 avec la communication sérielle RS-485.
 *
 *  Notes:
 *   Les commentaires dans ce code ne s'applique qu'aux nouveaux changements par rapport à 
 *   la version précédente.
 */

#include <HardwareSerial.h>

HardwareSerial remote = HardwareSerial(2);
const int transmit_pin = 18;


void setup() {
  Serial.begin(9600);
  remote.begin(9600);

  // activation du mode rs485
  remote.setPins(-1, -1, -1, transmit_pin);
  remote.setMode(UART_MODE_RS485_HALF_DUPLEX);

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (Serial.available()) {
    String data = Serial.readString();
    remote.println(data);
  }
  if (remote.available()) {
    String data = remote.readString();
    Serial.println(data);

    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW); 
  }
}
 