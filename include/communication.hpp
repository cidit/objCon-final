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
    constexpr uint32_t SIGNAL_BANDWIDTH = 125E3;

    struct dataframe
    {
        uint8_t version; // default: 0x07
        uint8_t destination;
        uint8_t provenance;
        uint16_t message_number;
        uint16_t timestamp;   // seconds
        int16_t mystery_data; // temperature*100, C
    } __attribute__((packed));

    
        // TODO: ocument
    void init() {
        // TODO: implement
    }

    bool get_df(dataframe&out) {
        
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            while (LoRa.available()) {
                // TODO: verify reinterpret cast syntax
                auto outb = reinterpret_cast<byte*>(&out);
                LoRa.readBytes(outb, sizeof(out));
            }
            return true;
        }
        return false;
    }
}
