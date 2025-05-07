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

void getURLServeur(void) {
  //Si le branchement Etherne est toujours bon...
  if (Ethernet.maintain() == 0) {  // https://www.arduino.cc/reference/en/libraries/ethernet/ethernet.maintain/

    Serial.println("> Préparation d'une requête HTTP");

    // Préparation des variables pour la requête http:
    // open-notify.org: utile pour retirer du data (api json)
    char serverName[] = "api.open-notify.org";  // à compléter: le nom du serveur
    char path[] = "/iss-now.json";              // à compléter: la resource à demander
    int port = 80;

    //Crée un objet http du type HTTPClient, utilisant le Ethernet
    EthernetClient client;                            //  https://www.arduino.cc/reference/en/libraries/ethernet/ethernetclient/
    HttpClient httpClient(client, serverName, port);  //liaison des paramètres https://github.com/arduino-libraries/ArduinoHttpClient

    Serial.println("> Forge d'une requête HTTP");

    //********** Debut de la forge d'une requete HTTP utilisant la méthode GET
    httpClient.beginRequest();
    httpClient.get(path);
    //****************HEADER***************
    //Indique la destination de la requête HTTP :
    httpClient.sendHeader("User-Agent", "ESP32HTTPClient");
    httpClient.sendHeader("Connection", "close");
    //*************************************
    Serial.println("> Envoie de la requête HTTP");
    httpClient.endRequest();

    //On attend ici la réponse du serveur (bloquant)
    int statusCode = httpClient.responseStatusCode();
    //**************************************

    //Vérification du code de réponse du serveur:
    if (statusCode > 0) {
      //Affiche le code de réponse:
      Serial.print("> Code reçu du serveur : ");
      Serial.println(statusCode);

      if (httpClient.headerAvailable()) {
        Serial.println("> Entête http détectée: ");
        //Itération jusqu'à 15 items de l'entête:
        byte maxLoopCount = 15;
        do {
          Serial.print("\t");
          Serial.print(httpClient.readHeaderName());
          Serial.print(" : ");
          Serial.println(httpClient.readHeaderValue());
          maxLoopCount--;
        } while (httpClient.headerAvailable() && !httpClient.endOfHeadersReached() && maxLoopCount > 0);

      } else Serial.println("> Aucune entête http disponible (douteux)");

      //Récupération du document transmis par le serveur:
      String response = httpClient.responseBody();

      //Affichage du contenu:
      Serial.println("> Contenu retourné par le serveur : ");
      Serial.println(response);

      //Traitement de la réponse à travers un objet JSON:
      JsonDocument doc;  //Création d'un caneva de document JSON nommé "doc"

      DeserializationError errorCatcher = deserializeJson(doc, response);  //désassemblage de la réponse (un String) en objet JSON (doc)

      //Tester pour connaître le résultat de l'opération précédente:
      if (errorCatcher.code() == DeserializationError::Ok) {
        // Si tout est ok, on récupère les infos provenant de l'objet JSON
        // Exemple recu: {"iss_position": {"latitude": "-12.0892", "longitude": "72.3354"}, "message": "success", "timestamp": 1675566127}
        Serial.println("> Désassemblage réussie - Résultat:");
        Serial.print("\tPos lattitude: ");
        Serial.println(doc["iss_position"]["latitude"].as<float>());

        //[CODE]        //Faire chose semblable pour récupérer la longitude
        longitude = doc["iss_position"]["longitude"];
        latitude = doc["iss_position"]["latitude"];

        //[CODE]        //Adapter pour récupérer timestamp
        when_timestamp = doc["timestamp"];

      } else {
        Serial.println("> Erreur de désassemblage JSON:");
        Serial.println(errorCatcher.c_str());
      }
    } else {
      Serial.print("> Erreur de connexion au serveur: ");
      Serial.println(statusCode);
    }

    Serial.println("> Libération de resources");
    //Libère les ressources du ESP32 utilisées pour la requête HTTP:
    httpClient.stop();

  } else {
    //Si le branchement avec le Ethernet n'est pas valide
    Serial.println("> Erreur de connexion réseau Ethernet");
  }

  Serial.println("> Fin de getURLServeur()");
}
