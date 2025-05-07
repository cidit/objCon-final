
#include "ThingSpeak.h"

#include <SPI.h>
#define USE_W5100 true          //Requis pour utiliser le shield DFRobot. Pas requis avec un W5500 connecté SPI.
#include <Ethernet.h>           //Lib arduino-ethernet v2.0.2
#include <ArduinoHttpClient.h>  //Librairie wrapper HTTP: arduinoHttpClient v0.6.1
#include <ArduinoJson.h>        //Librairie de manipulation document JSON v7.3.0
#include "DHT.h"
#include <ProtoTGP.h>  //Pour utiliser la librairie ProtoTGP


byte mac[] = { 0x24, 0x6f, 0x28, 0x01, 0x01, 0xB0 };

char serverName[] = "api.openweathermap.org";  // à compléter: le nom du serveur
String path = "/data/2.5/weather";             // à compléter: ressource (path) à demander au service

const char* myWriteAPIKey = "8ID5AO60YD6V94W9";
int port = 80;

DHT dht(33, DHT11);
ProtoTGP proto;
EthernetClient client;  //  https://www.arduino.cc/reference/en/libraries/ethernet/ethernetclient/
HttpClient httpClient(client, serverName, port);


// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 3000;

// Variable to hold temperature readings
float temperatureC;
//uncomment if you want to get temperature in Fahrenheit
//float temperatureF;


void setup() {
  Serial.begin(115200);  //Initialize serial

  Ethernet.init(SS);
  proto.begin();


  if (Ethernet.begin(mac) == 0) {  // référence: https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.begin/
    Serial.println("> Echec de configuration DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("> Blindage non disponible.  Désolé cette application requiert le réseau Ethernet. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("> Cable Ethernet non-connecté. SVP, vérifiez de nouveau est redémarrer (RESET)");
    }
    while (true) { delay(1); }  //boucle infinie - on ne fait plus rien si on parvient ici.
  }

  dht.begin();
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}

void loop() {
  proto.refresh();
  if ((millis() - lastTime) > timerDelay) {

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    Serial.print("Humidity: ");
    Serial.println(h);
    Serial.print("Temperature: ");
    Serial.println(t);

    // Get a new temperature reading
    // temperatureC = bme.readTemperature();
    // Serial.print("Temperature (ºC): ");
    // Serial.println(temperatureC);

    //uncomment if you want to get temperature in Fahrenheit
    /*temperatureF = 1.8 * bme.readTemperature() + 32;
    Serial.print("Temperature (ºF): ");
    Serial.println(temperatureF);*/

    // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
    // pieces of information in a channel.  Here, we write to field 1.
    // int x = ThingSpeak.writeField(1, 1, 4, myWriteAPIKey);
    // x = ThingSpeak.writeField(1, 2, 5, myWriteAPIKey);
    ThingSpeak.setField(1, t);
    ThingSpeak.setField(2, h);
    int x = ThingSpeak.writeFields(1, myWriteAPIKey);
    //uncomment if you want to get temperature in Fahrenheit
    //int x = ThingSpeak.writeField(myChannelNumber, 1, temperatureF, myWriteAPIKey);

    if (x == 200) {
      Serial.println("Channel update successful.");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    lastTime = millis();
  }
}