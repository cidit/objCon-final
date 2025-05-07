#pragma once

#include <Adafruit_BME280.h>

#include <ModbusRTUMaster.h> //https://github.com/CMB27/ModbusRTUMaster
#include <HardwareSerial.h>  //https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/HardwareSerial.h

namespace sensors
{

#define LOCAL_SER_BAUD 115200
#define MODBUS_BAUD 9600
#define MODBUS_CONFIG SERIAL_8N1

#define MODBUS_PERIPH_ID 0x01    // Numéro d'identifiant du périphérique SHT20 modbus (par défaut)
#define MODBUS_REGISTER_ID 0x01  // Adresse de départ de lecture du registre (là où commence la donnée température)
#define MODBUS_REGISTER_LEN 0x02 // Nombre de registre désiré d'obtenir par la requête: 2 afin de lire température et humidité d'un seul coup

  // Définitions des broches:
  const uint8_t dePin = 4;
  // const uint8_t RX2_pin = 16;
  // const uint8_t TX2_pin = 17;
  // const int RX0_pin = 3;
  // const int TX0_pin = 1;

  HardwareSerial SerialRemoteRS485(2);
  // HardwareSerial SerialLocal(0);
  // Objet modbus pour réaliser la communication:
  //  Note: ici c'est l'objet modbus qui s'occupe de contrôler la broche RE/DE pour le RS-485
  ModbusRTUMaster modbus(SerialRemoteRS485, dePin); // serial port, driver enable pin for rs-485 (optional)

  struct dataframe
  {
    float temperature_C, // provenance: SHT20
        humidity_Perc,   // provenance: SHT20
        pressure_kP,     // provenance: BME280
        luminosity_lux;  // provenance: GY-49
  };

  Adafruit_BME280 bme; // I2C

// MAX44009 I2C address is 0x4A(74)
#define GY49_ADDR 0x4A
#define SEALEVELPRESSURE_HPA (1013.25)

  void setup_gy49()
  {
    // Start I2C Transmission
    Wire.beginTransmission(GY49_ADDR);
    // Select configuration register
    Wire.write(0x02);
    // Continuous mode, Integration time = 800 ms
    Wire.write(0x40);
    // Stop I2C transmission
    Wire.endTransmission();

    delay(300);
  }

  void setup_bme280()
  {
    bool status;

    // byte a = 0b00111011;

    // default settings
    // (you can also pass in a Wire library object like &Wire2)
    status = bme.begin(0x76);
    if (!status)
    {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x");
      Serial.println(bme.sensorID(), 16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
      while (1)
        delay(10);
    }

    Serial.println();
  }

  void setup_sht20()
  {
    // Note: l'objet modbus s'occupe de la broche RE/DE; on configure donc un UART "normal" du point de vue du ESP32
    // SerialRemoteRS485.begin(MODBUS_BAUD, MODBUS_CONFIG, RX2_pin, TX2_pin);
    modbus.begin(MODBUS_BAUD, MODBUS_CONFIG);
    modbus.setTimeout(200);
  }

  float get_gy49()
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

    // Convert the data to lux
    int exponent = (data[0] & 0xF0) >> 4;
    int mantissa = ((data[0] & 0x0F) << 4) | (data[1] & 0x0F);
    float luminance = pow(2, exponent) * mantissa * 0.045;
    return luminance;
  }

  struct bme_data
  {
    float temperature_C, pressure_Pa, humidity_Perc, altitude_m;
  };

  bme_data get_bme280()
  {
    return (bme_data){
        .temperature_C = bme.readTemperature(),
        .pressure_Pa = bme.readPressure(),
        .humidity_Perc = bme.readHumidity(),
        .altitude_m = bme.readAltitude(SEALEVELPRESSURE_HPA),
    };
  }

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
    static const char *exceptionResponseStrings[] = {"illegal function", "illegal data address", "illegal data value", "server device failure"};

    if (!error)
      return;
    Serial.print("Error: ");
    if (error >= 4 && error < (numErrorStrings + 4))
    {
      Serial.print(errorStrings[error - 4]);
      if (error == MODBUS_RTU_MASTER_EXCEPTION_RESPONSE)
      {
        uint8_t exceptionResponse = modbus.getExceptionResponse();
        Serial.print(exceptionResponse);
        if (exceptionResponse >= 1 && exceptionResponse <= numExceptionResponseStrings)
        {
          Serial.print(" - ");
          Serial.print(exceptionResponseStrings[exceptionResponse - 1]);
        }
        // no longer in v 2.x modbus.clearExceptionResponse();
      }
    }
    else
    {
      Serial.print("Unknown error");
    }
    Serial.println();
  }

  struct sht_data
  {
    float temperature_C, humidity_Perc;
  };

  sht_data get_sht20()
  {

    int slaveID = MODBUS_PERIPH_ID;
    int startAddress = MODBUS_REGISTER_ID;
    int qtee = MODBUS_REGISTER_LEN;

    uint16_t inputRegisters[qtee]; // Création d'un bloc mémoire pour recevoir les données.

    // La requete modbus se réalise ici:
    auto errorCode = modbus.readInputRegisters(slaveID, startAddress, inputRegisters, qtee);

    // Si pas d'erreur, on affiche le contenu du tableau de données, sinon on affiche l'erreur:
    if (errorCode)
    {
      Serial.print("Got error: ");
      print_modbus_error(errorCode);
      return {0, 0};
    }

    auto temperature_C = inputRegisters[0] / 100.0;
    auto humidty_Perc = inputRegisters[1] / 100.0;

    return (sht_data){
        .temperature_C = temperature_C,
        .humidity_Perc = humidty_Perc,
    };
  }

}
