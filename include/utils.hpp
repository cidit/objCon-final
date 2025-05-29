#pragma once

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

    void init()
    {
        Serial.println("init [MQTT]");

        connect();
    }

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

namespace SDCARD {

    String filename;
    constexpr uint8_t CHIP_SELECT = 33;

    void init() {
        Serial.println("init [SD]");

        auto success = SD.begin(CHIP_SELECT);
        if (not success) {
            Serial.println("SD card fail");
            return;
        }
        filename = "/" + String(WIFI::get_ntp_time()) + ".csv";
        auto dataFile = SD.open(filename, FILE_WRITE);
        dataFile.println("id,systemID,timestamp,uptime,temperature,humidite,pression,lumnosite,donneeLoRa");
        dataFile.close();
    }

    bool append_row(share_data&df) {
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