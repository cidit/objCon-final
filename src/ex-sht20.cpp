#include <Arduino.h>

/*
  Laboratoire 01 - Communication sérielle - manip 5: modbus
  244-470-AL - Communication des objets 4.0
  Hiver 2025
  Modifié/adapté par Yh janvier 2025


  Ce code est une démontration du fonctionnement d'une interrogation modbus avec un module
   capteur de température et d'humidité sur RS-4585 (SHT20 modbus).

  Module AliExpress: https://www.aliexpress.us/item/3256804306512553.html

  PVI - Méthodes de la librairie ModbusRTUMaster (le # est le function code):
    1 (Read Coils):  CR PID
      modbus.readCoils(unitId, startAddress, buffer, quantity)
    2 (Read Discrete Inputs)
      modbus.readDiscreteInputs(unitId, startAddress, buffer, quantity)
    3 (Read Holding Registers)
      modbus.readHoldingRegisters(unitId, startAddress, buffer, quantity)
    4 (Read Input Registers)
      modbus.readInputRegisters(unitId, startAddress, buffer, quantity)
    5 (Write Single Coil)
      modbus.writeSingleCoil(unitId, address, value)
    6 (Write Single Holding Register)
      modbus.writeSingleHoldingRegister(unitId, address, value)
    15 (Write Multiple Coils)
      modbus.writeMultipleCoils(unitId, startingAddress, buffer, quantity)
    16 (Write Multiple Holding Registers).
      modbus.writeMultipleHoldingRegisters(unitId, startingAddress, buffer, quantity)

  En cas d'erreur: modbus.getExceptionResponse()

*/

#define _VERSION "1.0.2"

#include <ModbusRTUMaster.h> //https://github.com/CMB27/ModbusRTUMaster
#include <HardwareSerial.h>  //https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/HardwareSerial.h

HardwareSerial SerialRemoteRS485(2);
HardwareSerial SerialLocal(0);

#define LOCAL_SER_BAUD 115200
#define MODBUS_BAUD 9600
#define MODBUS_CONFIG SERIAL_8N1

#define MODBUS_PERIPH_ID 0x01    // Numéro d'identifiant du périphérique SHT20 modbus (par défaut)
#define MODBUS_REGISTER_ID 0x01  // Adresse de départ de lecture du registre (là où commence la donnée température)
#define MODBUS_REGISTER_LEN 0x02 // Nombre de registre désiré d'obtenir par la requête: 2 afin de lire température et humidité d'un seul coup

// Définitions des broches:
const uint8_t RX2_pin = 16;
const uint8_t TX2_pin = 17;
const uint8_t dePin = 21;
const int RX0_pin = 3;
const int TX0_pin = 1;

// Objet modbus pour réaliser la communication:
//  Note: ici c'est l'objet modbus qui s'occupe de contrôler la broche RE/DE pour le RS-485
ModbusRTUMaster modbus(SerialRemoteRS485, dePin); // serial port, driver enable pin for rs-485 (optional)

// Variables de la minuterie de lecture:
uint32_t lastMillis = 0;
const uint32_t pollingPeriod = 5000;

// Ne sert qu'à décoder l'erreur de chiffre en texte, en cas de besoin:
const uint8_t numErrorStrings = 12;
const char *errorStrings[numErrorStrings] = {
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

const uint8_t numExceptionResponseStrings = 4;
const char *exceptionResponseStrings[] = {"illegal function", "illegal data address", "illegal data value", "server device failure"};

void printError(uint8_t error)
{
  if (!error)
    return;
  SerialLocal.print("Error: ");
  if (error >= 4 && error < (numErrorStrings + 4))
  {
    SerialLocal.print(errorStrings[error - 4]);
    if (error == MODBUS_RTU_MASTER_EXCEPTION_RESPONSE)
    {
      uint8_t exceptionResponse = modbus.getExceptionResponse();
      SerialLocal.print(exceptionResponse);
      if (exceptionResponse >= 1 && exceptionResponse <= numExceptionResponseStrings)
      {
        SerialLocal.print(" - ");
        SerialLocal.print(exceptionResponseStrings[exceptionResponse - 1]);
      }
      // no longer in v 2.x modbus.clearExceptionResponse();
    }
  }
  else
    SerialLocal.print("Unknown error");
  SerialLocal.println();
}

/*--- Setup - Loop ----------------------------------------------------------------*/

void setup()
{
  SerialLocal.begin(LOCAL_SER_BAUD, MODBUS_CONFIG, RX0_pin, TX0_pin);
  SerialLocal.setMode(UART_MODE_UART);

  // Note: l'objet modbus s'occupe de la broche RE/DE; on configure donc un UART "normal" du point de vue du ESP32
  SerialRemoteRS485.begin(MODBUS_BAUD, MODBUS_CONFIG, RX2_pin, TX2_pin);
  modbus.begin(MODBUS_BAUD, MODBUS_CONFIG);
  modbus.setTimeout(200);

  delay(1000);
  Serial.println(__FILE__);
  SerialLocal.println("Ready!");
}

void loop()
{
  // minuterie, à intervale de la seconde
  if (millis() - lastMillis > pollingPeriod)
  {
    lastMillis = millis();

    int slaveID = MODBUS_PERIPH_ID;
    int startAddress = MODBUS_REGISTER_ID;
    int qtee = MODBUS_REGISTER_LEN;

    SerialLocal.println("Cmd: READ Type= IR sID=" + String(slaveID) + " addr=" + String(startAddress) + " len=" + String(qtee));

    uint8_t errorCode = 0;
    uint16_t inputRegisters[qtee]; // Création d'un bloc mémoire pour recevoir les données.

    // La requete modbus se réalise ici:
    errorCode = modbus.readInputRegisters(slaveID, startAddress, inputRegisters, qtee);

    // Si pas d'erreur, on affiche le contenu du tableau de données, sinon on affiche l'erreur:
    if (!errorCode)
    {
      SerialLocal.println("Read " + String(qtee) + " input registers(s) at ID 0x" + String(slaveID, HEX) + " at address 0x" + String(startAddress, HEX));
      for (int i = 0; i < qtee; i++)
      {
        SerialLocal.println("\tIR[" + String(i) + "] : " + String(inputRegisters[i], DEC) + " (0x" + String(inputRegisters[i], HEX) + ")");
        SerialLocal.println(inputRegisters[i] / 10.f);
      }
    }
    else
    {
      SerialLocal.print("Got error: ");
      printError(errorCode);
    } // end-if (!errorCode)
  }
}
