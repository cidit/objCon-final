void getNPrintMAC(void) {
  byte macBuffer[6];               // create a buffer to hold the MAC address
  Ethernet.MACAddress(macBuffer);  // fill the buffer
                                   //  Serial.print("The MAC address is: ");
  for (byte octet = 0; octet < 6; octet++) {
    if (macBuffer[octet] < 0x10) Serial.print('0');
    Serial.print(macBuffer[octet], HEX);
    if (octet < 5) {
      Serial.print(':');
    }
  }
}

String checkEthernetModule() {
  String retValue;

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    retValue = "Ethernet shield was not found.";
  } else if (Ethernet.hardwareStatus() == EthernetW5100) {
    retValue = "W5100 Ethernet controller detected.";
  } else if (Ethernet.hardwareStatus() == EthernetW5200) {
    retValue = "W5200 Ethernet controller detected.";
  } else if (Ethernet.hardwareStatus() == EthernetW5500) {
    retValue = "W5500 Ethernet controller detected.";
  }

  return retValue;
}

bool getURLServeur(void) {
  bool retCode = false;
  //Si le branchement Ethernet est toujours bon...
  if (Ethernet.maintain() == 0) {
    Serial.println("> Préparation d'une requête HTTP GET");
    // *** Configuration - Début
    //Paramètres pour aller chercher du data chez OpenWeatherMap (OWM):
    char serverName[] = "api.openweathermap.org";  // à compléter: le nom du serveur
    String path = "/data/2.5/weather";           // à compléter: ressource (path) à demander au service
    char token[] = OWM_TOKEN;                      // utilise ici votre OWM API token (privé), défini dans secret.h
    int port = 80;

    //Paramètres spécifiques pour l'API de OWM:
    float lat = OWM_MYCOORLAT;   //Lattitude géographique, selon secret.h
    float lon = OWM_MYCOORLONG;  //Longitude géographique, selon secret.h

    // *** Configuration - Fin

    //Autres options de la requete chez OWM
    char units[] = "metric";                          //On désire les données selon le format métrique du SI
    char options[] = "minutely,hourly,daily,alerts";  //Afin d'écourter la réponse, on se limite à exclure quelques éléments

    // CONSULTER LA DOCUMENTATION DE l'API de OWM: https://openweathermap.org/current
    // Il faut forger le arguments de la requête GET avec les paramètres précédemment définis.
    // Format requis par OWM pour obtenir la météo courante:  ?lat={lat}&lon={lon}&appid={API key}
    // Concaténation des paramètres dans 1 variable:
     // vous devez icI assembler correctement les paramètres: longitude, latitude, unités, options et clé d'API
    String getReq = String(path) + 
      String("?lat=") + 
      String(lat) + 
      String("&lon=") + 
      String(lon) + 
      String("&appid=") +
      String(OWM_TOKEN); 
    Serial.println("getReq = " + getReq);

    //Préparer une connexion HTTP par le ESP32:
    EthernetClient client;                            //  https://www.arduino.cc/reference/en/libraries/ethernet/ethernetclient/
    HttpClient httpClient(client, serverName, port);  //liaison des paramètres https://github.com/arduino-libraries/ArduinoHttpClient

    Serial.println("> Forge de la requête HTTP");

    //********** Debut de la forge d'une requete HTTP utilisant la méthode GET
    httpClient.beginRequest();
    httpClient.get(getReq);

    //****************HEADER***************
    //Indique la destination de la requête HTTP :
    httpClient.sendHeader("User-Agent", "ESP32HTTPClient");
    httpClient.sendHeader("Content-Type", "application/x-www-form-urlencoded");
    httpClient.sendHeader("Connection", "close");
    //*************************************
    Serial.println("> Envoie de la requête HTTP");
    httpClient.endRequest();

    int statusCode = httpClient.responseStatusCode();
    //**************************************

    //Vérifie la réponse du serveur :
    if (statusCode > 0) {
      //Affiche le code de retour :
      Serial.print("> Code reçu du serveur : ");
      Serial.println(statusCode);

      if (httpClient.headerAvailable()) {  //Teste si une entête de réponse est disponible
        Serial.println("> Entête http détectée: ");
        //Itération jusqu'à 15 items de l'entête (notre limite, arbitraire):
        byte maxLoopCount = 15;
        do {
          Serial.print("\t");
          Serial.print(httpClient.readHeaderName());
          Serial.print(" : ");
          Serial.println(httpClient.readHeaderValue());
          maxLoopCount--;
        } while (httpClient.headerAvailable() && !httpClient.endOfHeadersReached() && maxLoopCount > 0);

      } else Serial.println("> Aucune entête http disponible (douteux)");

      //Affiche le body transmis par le serveur :
      String response = httpClient.responseBody();

      //Affichage du contenu:
      Serial.println("> Contenu retourné par le serveur : ");
      Serial.println(response);
      Serial.println("> Copier et coller dans un outil JSON prettifyer: https://jsonformatter.org/json-pretty-print");

      if (traiteReponseJSON(response)) {
        Serial.println("> Traitement complété avec succès");
        retCode = true;
      } else {
        Serial.println("> Traitement non-complété avec succès");
      }

    } else {

      Serial.print("> Erreur de connexion au serveur: ");
      Serial.println(statusCode);
    }

    //Libère les ressources du ESP32 utilisées pour la requête HTTP:
    Serial.println("> Libération de resources");
    httpClient.stop();

  } else {
    //Si le branchement avec le Ethernet n'est pas valide
    Serial.println("> Erreur de connexion réseau Ethernet");
  }
  Serial.println("> Fin de getURLServeur()");
  return retCode;
}

/*
  Instructions (code) à écrire ici permettant de désérialiser la réponse String dans un objet JSON
  afin de récupérer les données à considérer:
  conditions actuelles: température, humidité, pression barométrique, vitesse et direction des vents
*/
bool traiteReponseJSON(String response) {

  //Création d'un document JSON pour le désassemblage de la chaîne JSON
  JsonDocument doc;
  auto error = deserializeJson(doc, response);

  if (error.code() != DeserializationError::Ok) {
    return false;
  }

  // [CODE]   À vous de compléter...
  // Consulter l'exemple: https://arduinojson.org/v6/example/parser/
  // Voir la docume de la fonction de désassemblage: https://arduinojson.org/v6/api/json/deserializejson/
  // (ainsi que https://arduinojson.org/v6/api/misc/deserializationerror/ afin de traiter les cas d'erreur)
  // InstructionS
  // [...]
  // InstructionS
  
  float temperature_kelvin = doc["main"]["temp"];
  float pressure_hectopascal = doc["main"]["humidity"], 
        pressure_bar = pressure_hectopascal/10E3;
  float wind_speed_mps = doc["wind"]["speed"];
  float humidity_percent = doc["main"]["humidity"];
  long wind_direction_degree = doc["wind"]["deg"];

  temperature = temperature_kelvin-273.15; // degrees
  humidite = humidity_percent/100; 
  pression = pressure_bar;
  vitVent = wind_speed_mps;
  dirVent = wind_direction_degree;


  return true;
}