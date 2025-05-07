#include <Arduino.h>

#include "sensors.hpp"

// TODO: document
// HINT: saving things (to the SD card, screen, TB)
namespace persistance {
    struct dataframe {
      uint16_t recording_id,
        system_id,
        timestamp,  // unix, s
        uptime;     // s
      sensors::dataframe sensors;
      float mystery_data;  // refer to laura dataframe
    };
    // TODO: implement
  }