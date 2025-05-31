#pragma once

/**
 * DÉTAILS IMPORTANTS sur le format des symboles dans ce fichier
 *
 * Les namespaces ségrèguent les differentes fonctionnalités de la station de manière standardisée.
 * Elles comportent toutes une fonction <NAMESPACE>::init() qui initialize la fonctionnalité
 * de manière synchrone sans valeur de retours.
 *
 * Les namespaces SHT20, BME280, GY49 et LORA décrivent des fonctionnalités de "capteurs". (évidament,
 * le LoRa n'est pas un capteur spécifique comme les autres, mais pour les besoins de la station, il est
 * plus simple de le considérer comme tel.)
 *
 * Ces namespaces de type "capteur" ont deux particularités récurentes:
 * - elles définissent un type de donnée correspondant aux données attendues du capteur.
 * - elles implémentent une fonction <NAMESPACE>::read(<DATATYPE>&out) qui sert à effectuer une seule
 *   lecture synchrone du capteur et qui retourne si la lecture a été effectuée avec succes, ou non.
 */

#include <Arduino.h>
#include <ModbusRTUMaster.h> //https://github.com/CMB27/ModbusRTUMaster
#include <HardwareSerial.h>  //https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/HardwareSerial.h
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <PubSubClient.h>
#include <SD.h>

#include "secrets.h"

struct sensors_data
{
    float temperature_C, // provenance: SHT20
        humidity_Perc,   // provenance: SHT20
        pressure_kPa,    // provenance: BME280
        luminosity_lux;  // provenance: GY-49
};

/**
 * Ceci est le format des données récoltées pour chaques messages de la station.
 * C'est aussi le format utilisé pour enregistrer des données dans la carte SD
 */
struct share_data
{
    uint16_t system_id;
    uint32_t recording_id;
    uint64_t timestamp, uptime; // unix, s
    sensors_data sensors;
    float mystery_data; // refer to lora dataframe
};

namespace SHT20
{

    struct data
    {
        float temperature_C, humidity_Perc;
    };

    HardwareSerial remote(2);

    constexpr uint32_t MODBUS_BAUD = 9600, MODBUS_CONFIG = SERIAL_8N1;

    constexpr uint8_t
        // Numéro d'identifiant du périphérique SHT20 modbus (par défaut)
        MODBUS_PERIPH_ID = 0x01,
        // Adresse de départ de lecture du registre (là où commence la donnée température)
        MODBUS_REGISTER_ID = 0x01,
        // Nombre de registre désiré d'obtenir par la requête: 2 afin de lire température et humidité d'un seul coup
        MODBUS_REGISTER_LEN = 0x02;

    // Définitions des broches:
    const uint8_t dePin = 32;

    // Objet modbus pour réaliser la communication:
    //  Note: ici c'est l'objet modbus qui s'occupe de contrôler la broche RE/DE pour le RS-485
    ModbusRTUMaster modbus(remote, dePin); // serial port, driver enable pin for rs-485 (optional)

    /**
     * Nom: SHT20::print_modbus_error
     * Fonction: converti un code d'erreur modbus en messages d'erreurs descriptifs.
     * Argument(s) réception: le code d'erreur modbus
     * Argument(s) de retour: (rien)
     * Modifie/utilise (globale): (rien)
     * Notes:
     * - le code est tiré du laboratoire #5 fait en classe.
     */
    void print_modbus_error(uint8_t error)
    {
        // Ne sert qu'à décoder l'erreur de chiffre en texte, en cas de besoin:
        static const uint8_t numErrorStrings = 12;
        static const char *errorStrings[numErrorStrings] = {
            "Response timeout",
            "Frame error in response",
            "CRC error in response",
            "Unknown communication error",
            "Unexpected unit ID in response",
            "Exception response ",
            "Unexpected function code in response",
            "Unexpected response length",
            "Unexpected byte count in response",
            "Unexpected data address in response",
            "Unexpected value in response",
            "Unexpected quantity in response"};

        static const uint8_t numExceptionResponseStrings = 4;
        static const char *exceptionResponseStrings[numExceptionResponseStrings] = {"illegal function", "illegal data address", "illegal data value", "server device failure"};

        if (!error)
            return;

        Serial.print("Error: ");
        if (error >= numExceptionResponseStrings && error < (numErrorStrings + numExceptionResponseStrings))
        {
            Serial.print(errorStrings[error - numExceptionResponseStrings]);
            if (error == MODBUS_RTU_MASTER_EXCEPTION_RESPONSE)
            {
                uint8_t exceptionResponse = modbus.getExceptionResponse();
                Serial.print(exceptionResponse);
                if (exceptionResponse >= 1 && exceptionResponse <= numExceptionResponseStrings)
                {
                    Serial.print(" - ");
                    Serial.print(exceptionResponseStrings[exceptionResponse - 1]);
                }
            }
        }
        else
        {
            Serial.print("Unknown error");
        }
        Serial.println();
    }

    void init()
    {
        Serial.println("init [SHT20]");

        remote.begin(MODBUS_BAUD, MODBUS_CONFIG);
        modbus.begin(MODBUS_BAUD, MODBUS_CONFIG);
        modbus.setTimeout(500);
    }

    bool read(data &out)
    {
        // Création d'un bloc mémoire pour recevoir les données.
        uint16_t inputRegisters[MODBUS_REGISTER_LEN];

        // La requete modbus se réalise ici:
        uint8_t errorCode = modbus.readInputRegisters(
            MODBUS_PERIPH_ID,
            MODBUS_REGISTER_ID,
            inputRegisters,
            MODBUS_REGISTER_LEN);

        if (errorCode)
        {
            Serial.print("Got error: " + String(errorCode));
            print_modbus_error(errorCode);
            return false;
        }

        out = {
            .temperature_C = inputRegisters[0] / 10.f,
            .humidity_Perc = inputRegisters[1] / 10.f,
        };
        return true;
    }
}

namespace BME280
{
    struct data
    {
        float temperature_C, pressure_Pa, humidity_Perc, altitude_m;
    };

    static constexpr float SEALEVELPRESSURE_HPA = (1013.25);

    constexpr uint8_t BME280_ADDR = 0x76;

    Adafruit_BME280 bme;

    void init()
    {
        Serial.println("init [BME280]");
        auto status = bme.begin(BME280_ADDR);
        if (!status)
        {
            Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
            Serial.print("SensorID was: 0x");
            Serial.println(bme.sensorID(), 16);
            Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
            Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
            Serial.print("        ID of 0x60 represents a BME 280.\n");
            Serial.print("        ID of 0x61 represents a BME 680.\n");
        }
    }

    bool read(data &out)
    {
        data d = {
            .temperature_C = bme.readTemperature(),
            .pressure_Pa = bme.readPressure(),
            .humidity_Perc = bme.readHumidity(),
            .altitude_m = bme.readAltitude(SEALEVELPRESSURE_HPA),
        };

        if (isnan(d.pressure_Pa))
        {
            Serial.println("BME280 disconnected");
            return false;
        }
        return true;
    }
}

namespace GY49
{
    constexpr uint8_t GY49_ADDR = 0x4A;

    using data = float;

    void init()
    {
        Serial.println("init [GY49]");

        // Start I2C Transmission
        Wire.beginTransmission(GY49_ADDR);
        // Select configuration register
        Wire.write(0x02);
        // Continuous mode, Integration time = 800 ms
        Wire.write(0x40);
        // Stop I2C transmission
        Wire.endTransmission();
    }

    bool read(data &out)
    {

        unsigned int data[2];

        // Start I2C Transmission
        Wire.beginTransmission(GY49_ADDR);
        // Select data register
        Wire.write(0x03);
        // Stop I2C transmission
        Wire.endTransmission();

        // Request 2 bytes of data
        Wire.requestFrom(GY49_ADDR, 2);

        // Read 2 bytes of data
        // luminance msb, luminance lsb
        if (Wire.available() == 2)
        {
            data[0] = Wire.read();
            data[1] = Wire.read();
        }
        else
        {
            Serial.println("did not receive enough data.");
            return false;
        }

        // Convert the data to lux
        int exponent = (data[0] & 0xF0) >> 4;
        int mantissa = ((data[0] & 0x0F) << 4) | (data[1] & 0x0F);
        float luminance = pow(2, exponent) * mantissa * 0.045;
        out = luminance;
        return true;
    }
}

namespace LORA
{

    struct data
    {
        uint8_t version; // default: 0x07
        uint8_t destination;
        uint8_t provenance;
        uint16_t message_number;
        uint16_t timestamp;   // seconds
        int16_t mystery_data; // temperature*100, C
    } __attribute__((packed));

    constexpr uint8_t RFM95_ss = 5,
                      RFM95_rst = 14,
                      RFM95_dio0 = 2;

    constexpr uint8_t CHANNEL = 2,
                      SYNC_WORD = 0xAA,
                      SPREADING_FACTOR = 10,
                      CODING_RATE = 5;
    constexpr uint32_t SIGNAL_BAND = 125E3;

    /**
     * Nom: LORA::frequency_from_channel
     * Fonction: fait la traduction entre un channel lora et la fréquence qui lui correspond.
     * Argument(s) réception: le channel
     * Argument(s) de retour: la fréquance
     * Modifie/utilise (globale): (rien)
     * Notes: (rien)
     */
    const uint64_t frequency_from_channel(int channel)
    {
        constexpr uint32_t START = 902.3E6, INCREMENTS = 200E3;
        return START + channel * INCREMENTS;
    }

    void init()
    {
        Serial.println("init [LORA]");

        Serial.println("LoRa Dump Registers");

        // --- DEBUT INITIALISATION LORA
        // setup LoRa transceiver module
        Serial.println("> RFM95: Init pins");
        LoRa.setPins(RFM95_ss, RFM95_rst, RFM95_dio0);

        Serial.println("> RFM95: Initializing");
        while (!LoRa.begin(frequency_from_channel(CHANNEL)))
        {
            Serial.print(".");
            delay(500);
        }
        Serial.println("> RFM95: configuration réussie");

        Serial.println("> RFM95: dumping registers:");
        LoRa.dumpRegisters(Serial);
        Serial.println("> RFM95: dump complété.");

        Serial.println("> RFM95: set registers:");
        Serial.println("> RFM95: LoRaSyncWord");
        LoRa.setSyncWord(SYNC_WORD);

        Serial.println("> RFM95: set CRC");
        LoRa.enableCrc();

        // The spreading factor (SF) impacts the communication performance of LoRa, which uses an SF between 7 and 12. A larger SF increases the time on air, which increases energy consumption, reduces the data rate, and improves communication range. For successful communication, as determined by the SF, the modulation method must correspond between a transmitter and a receiver for a given packet.
        Serial.println("> RFM95: SF");
        LoRa.setSpreadingFactor(SPREADING_FACTOR);

        Serial.println("> RFM95: BAND");
        LoRa.setSignalBandwidth(SIGNAL_BAND);

        Serial.println("> RFM95: CR");
        LoRa.setCodingRate4(CODING_RATE);
        // --- FIN INITIALISATION LORA
    }

    bool read(data &out)
    {
        int packetSize = LoRa.parsePacket();
        if (packetSize)
        {
            Serial.print("Receiving LoRa packet.");
            if (LoRa.available() != sizeof(out))
            {
                Serial.println("LoRa: incomplete packet");
                return false;
            }
            else
            {
                auto out_b = reinterpret_cast<uint8_t *>(&out);
                LoRa.readBytes(out_b, sizeof(out));
                return true;
            }
        }
        return false;
    }
}

namespace WIFI
{
    WiFiMulti wifi_multi;

    /**
     * Nom: WIFI::init
     * Fonction: Initialize la fonctionnalité WIFI et enregistre les divers points d'accès auquels la station essayera de se connecter.
     * Argument(s) réception: (rien)
     * Argument(s) de retour: (rien)
     * Modifie/utilise (globale):
     * - WiFi
     * - WIFI::wifi_multi
     * Notes:
     * - une amélioration serait de transferer les identifiants et mots de passe des points d'access à un fichier de configuration.
     */
    void init()
    {
        Serial.println("init [WIFI]");

        WiFi.mode(WIFI_STA);
        wifi_multi.addAP("CAL-Techno", "technophys123");
        wifi_multi.addAP("zeppy", "Newera2012");
        wifi_multi.addAP("MQTT", "");
        wifi_multi.addAP("Invite", "");
        wifi_multi.addAP("CAL-Net", "");
    }

    /**
     * Nom: WIFI::connect
     * Fonction: Fait la connection et configuration de la fonctionnalité WIFI.
     * Argument(s) réception: (rien)
     * Argument(s) de retour: si la connection est établie ou non.
     * Modifie/utilise (globale):
     * - WiFi
     * - WIFI::wifi_multi
     * Notes: (rien)
     */
    bool connect()
    {
        int n = WiFi.scanNetworks();
        if (n == 0)
        {
            Serial.println("No network found.");
            return false;
        }

        Serial.println(String(n) + " network(s) found.");

        if (wifi_multi.run(5000) != WL_CONNECTED)
        {
            Serial.println("Could not connect to any networks.");
            return false;
        }

        Serial.println("Connected to wifi!");
        Serial.println("\tSSID: " + WiFi.SSID());
        Serial.println();

        return true;
    }

    /**
     * Nom: WIFI::get_ntp_time
     * Fonction: récupère le temps unix en secondes.
     * Argument(s) réception: (rien)
     * Argument(s) de retour: le temps unix en secondes.
     * Modifie/utilise (globale): (rien)
     * Notes:
     * - plusieurs valeurs utilisées dans cette fonction doivent être définies dans un fichier `secrets.h`.
     * - une connection WiFi valide doit être établie avant de pouvoir se connecter à MQTT.
     */
    time_t get_ntp_time()
    {
        // https://randomnerdtutorials.com/epoch-unix-time-esp32-arduino/

        time_t t;
        struct tm timeinfo;
        if (not getLocalTime(&timeinfo))
        {
            Serial.println("Failed to obtain time");
            return 0;
        }
        time(&t);
        return t;
    }
}

namespace MQTT
{
    WiFiClient wifi_client;
    PubSubClient mqtt_client(wifi_client);

    /**
     * Nom: MQTT::connect
     * Fonction: Fait la connection et configuration du client mqtt.
     * Argument(s) réception: (rien)
     * Argument(s) de retour: si la connection est établie ou non.
     * Modifie/utilise (globale):
     * - MQTT::mqtt_client
     * Notes:
     * - plusieurs valeurs utilisées dans cette fonction doivent être définies dans un fichier `secrets.h`.
     * - une connection WiFi valide doit être établie avant de pouvoir se connecter à MQTT.
     */
    bool connect()
    {
        mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
        if (not mqtt_client.connect(MQTT_clientId, MQTT_userName, MQTT_password))
        {
            Serial.println("Could not connect to MQTT.");
            return false;
        }
        mqtt_client.subscribe(MQTT_SUBSCRIBE_TOPIC);
        Serial.println("Connected to MQTT!");
        return true;
    }

    /**
     * Nom: MQTT::init
     * Fonction: initiallise la fonctionnalité MQTT.
     * Argument(s) réception: (rien)
     * Argument(s) de retour: (rien)
     * Modifie/utilise (globale):
     * - MQTT::mqtt_client
     * Notes: (rien)
     */
    void init()
    {
        Serial.println("init [MQTT]");

        connect();
    }

    /**
     * Nom: MQTT::publish
     * Fonction: Publie un document json sur le topic configuré.
     * Argument(s) réception: le json à publier
     * Argument(s) de retour: si la publication a été faite avec succes.
     * Modifie/utilise (globale):
     * - MQTT::mqtt_client
     * Notes:
     * - MQTT_PUBLISH_TOPIC doit être défini dans un fichier `secrets.h`.
     */
    bool publish(JsonDocument &json)
    {
        String payload;
        serializeJson(json, payload);
        if (mqtt_client.publish(MQTT_PUBLISH_TOPIC, payload.c_str()))
        {
            Serial.println("> Publication au topic ");
            return true;
        }
        else
        {
            Serial.println("> ERREUR publication");
            return false;
        }
    }
}

/**
 * Nom: maintain_comms
 * Fonction: Fait le maintient la communication WIFI et MQTT. Tente de re-connecter si possible ce qui n'est pas connecté si possible.
 * Argument(s) réception: (rien)
 * Argument(s) de retour: si la connection est en ligne ou non.
 * Modifie/utilise (globale):
 * - WiFi
 * - WIFI
 * - MQTT
 * Notes:  (spécial, source, amélioration, etc) TODO:
 */
bool maintain_comms()
{
    auto comms_online = false;
    if (not WiFi.isConnected())
    {
        comms_online = WIFI::connect() && MQTT::connect();
    }
    else if (not MQTT::mqtt_client.connected())
    {
        comms_online = MQTT::connect();
    }
    else
    {
        comms_online = true;
    }
    return comms_online;
}

namespace SDCARD
{

    String filename;
    constexpr uint8_t CHIP_SELECT = 33;

    /**
     * Nom: SDCARD::init
     * Fonction: initialize la sauvegarde via carte SD.
     * Argument(s) réception: (rien)
     * Argument(s) de retour: (rien)
     * Modifie/utilise (globale):
     * - SD
     * - SDCARD::filename
     * Notes:
     * - Initialize le nom du fichier à l'aide du unix timestamp pour permettre plusieurs activations sur plusieurs jours. Pour cette raison, le WIFI doit fonctionner avant d'appeller cette fonction.
     */
    void init()
    {
        Serial.println("init [SD]");

        auto success = SD.begin(CHIP_SELECT);
        if (not success)
        {
            Serial.println("SD card fail");
            return;
        }
        filename = "/" + String(WIFI::get_ntp_time()) + ".csv";
        auto dataFile = SD.open(filename, FILE_WRITE);
        // on initialise l'entête du fichier CSV.
        dataFile.println("id,systemID,timestamp,uptime,temperature,humidite,pression,lumnosite,donneeLoRa");
        dataFile.close();
    }

    /**
     * Nom: SDCARD::append_row
     * Fonction: ajoute une rangée à la fin du fichier CSV actif dans la carte SD.
     * Argument(s) réception:
     * - df: dataframe, tramme de données représentant les valeurs des colones de la rangée à ajouter.
     * Argument(s) de retour: si l'operation a été un succès, ou non.
     * Modifie/utilise (globale):
     * - SD
     * - SDCARD::filename
     * Notes:
     * - la fonctionalité doit être initialisée en premier (SDCARD::init).
     * - il est assumé que l'operation ne ratera jamais.
     */
    bool append_row(share_data &df)
    {
        auto dataFile = SD.open(filename, FILE_APPEND);
        dataFile.print(String(df.recording_id) + ",");
        dataFile.print(String(df.system_id) + ",");
        dataFile.print(String(df.timestamp) + ",");
        dataFile.print(String(df.uptime) + ",");
        dataFile.print(String(df.sensors.temperature_C) + ",");
        dataFile.print(String(df.sensors.humidity_Perc) + ",");
        dataFile.print(String(df.sensors.pressure_kPa) + ",");
        dataFile.print(String(df.sensors.luminosity_lux) + ",");
        dataFile.println(String(df.mystery_data));
        dataFile.close();
        return true;
    }
}