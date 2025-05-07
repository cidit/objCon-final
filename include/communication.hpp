#pragma once
#include <Arduino.h>

#include <LoRa.h>

namespace lora
{

    // re-deal
#define RFM95_ss 5
#define RFM95_rst 14
#define RFM95_dio0 2

    constexpr uint8_t CHANNEL = 2,
                      SYNC_WORD = 0xAA,
                      SPREADING_FACTOR = 10,
                      CODING_RATE = 5;
    constexpr uint32_t SIGNAL_BAND = 125E3;

    struct dataframe
    {
        uint8_t version; // default: 0x07
        uint8_t destination;
        uint8_t provenance;
        uint16_t message_number;
        uint16_t timestamp;   // seconds
        int16_t mystery_data; // temperature*100, C
    } __attribute__((packed));

    uint32_t frequency_from_channel(int channel) {
        constexpr uint32_t start = 902.3E6, increments = 200E3;
        return start + channel*increments;
    }

    // TODO: document
    void init()
    {
        Serial.println("LoRa Dump Registers");
      
        // --- DEBUT INITIALISATION LORA
        //setup LoRa transceiver module
        Serial.println("> RFM95: Init pins");
        LoRa.setPins(RFM95_ss, RFM95_rst, RFM95_dio0);
      
        Serial.println("> RFM95: Initializing");
        while (!LoRa.begin(frequency_from_channel(CHANNEL))) {
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
      
        //The spreading factor (SF) impacts the communication performance of LoRa, which uses an SF between 7 and 12. A larger SF increases the time on air, which increases energy consumption, reduces the data rate, and improves communication range. For successful communication, as determined by the SF, the modulation method must correspond between a transmitter and a receiver for a given packet.
        Serial.println("> RFM95: SF");
        LoRa.setSpreadingFactor(SPREADING_FACTOR);
      
        Serial.println("> RFM95: BAND");
        LoRa.setSignalBandwidth(SIGNAL_BAND);
      
        Serial.println("> RFM95: CR");
        LoRa.setCodingRate4(CODING_RATE);
        // --- FIN INITIALISATION LORA
      
    }

    // TODO: document
    bool read(dataframe &out)
    {
        int packetSize = LoRa.parsePacket();
        if (packetSize)
        {
            while (LoRa.available())
            {
                // TODO: verify reinterpret cast syntax
                auto out_b = reinterpret_cast<uint8_t *>(&out);
                LoRa.readBytes(out_b, sizeof(out));
            }
            return true;
        }
        return false;
    }
}
