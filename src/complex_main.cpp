#include <Arduino.h>

#include <SPI.h>
#include <LoRa.h>
#include <Ecran.h>
#include <ProtoTGP.h> //Pour utiliser la librairie ProtoTGP
// #include "ThingSpeak.h"
#include <WiFi.h>
// #include <WiFiMulti.h>  // en complément pour supporter +ieurs config Wifi
#include <esp_wifi.h> //requis pour changer la MAC du Wifi
// #include <ArduinoHttpClient.h>  //Librairie wrapper HTTP: arduinoHttpClient v0.6.1
#include <Ethernet.h>     // arduino-ethernet v2.0.1
#include <PubSubClient.h> // v2.8  https://pubsubclient.knolleary.net/
#include <ArduinoJson.h>  //Librairie de manipulation document JSON v6.21.0
#include <Decodeur.h>
#include "DHT.h"
#include <ModbusRTUMaster.h> //https://github.com/CMB27/ModbusRTUMaster
#include <HardwareSerial.h>  //https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/HardwareSerial.h

#include "timer.hpp"
#include "sensors.hpp"
#include "persistance.hpp"
#include "communication.hpp"
#include "print.hpp"
#include "secrets.h"

constexpr uint8_t DE_PIN = 21,
                  GREEN_LED_PIN = 2,
                  RED_LED_PIN = 4,
                  BME280_ADDR = 0x76,
                  GY49_ADDR = 0x4A;

WiFiClient wifi;
PubSubClient mqtt(wifi);
JsonDocument json;
HardwareSerial remote(2);
Ecran monEcran;
time_t startup_unix_timestamp;
Adafruit_BME280 inner_bme;

IntervalTimer main_timer(5 * 1000), // TODO: CHANGE TO 60s 
    screen_scroll_timer(2 * 1000),
    reconnect_timer(5 * 1000);

sensors::SHT20 sht20(remote, DE_PIN);
sensors::BME280 bme280(inner_bme, BME280_ADDR);
sensors::GY49 gy49(Wire, GY49_ADDR);
communication::CommunicationService comm_service(mqtt);

void handle_tb_message(char *topic, byte *payload, unsigned int length)
{
  // recMsgCount++;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(String(payload, length));

  json.clear();
  deserializeJson(json, payload, length);

  String which_led = json["led"];

  if (which_led == "red")
  {
    bool isHigh = json["state"];
    digitalWrite(RED_LED_PIN, isHigh ? HIGH : LOW);
  }
  else if (which_led == "green")
  {
    bool isHigh = json["state"];
    digitalWrite(GREEN_LED_PIN, isHigh ? HIGH : LOW);
  }
}

void setup()
{
  Serial.begin(115200);
  remote.begin(sensors::SHT20::MODBUS_BAUD, sensors::SHT20::MODBUS_CONFIG);

  while (!Serial)
    ;
  monEcran.begin();
  mqtt.setCallback(handle_tb_message);

  lora::init(Serial);
  // persistance::init(Serial); // TODO
  comm_service.init(Serial);
  sht20.init(Serial);
  bme280.init(Serial);
  gy49.init(Serial);

  // if (comm_service.connect(Serial))
  // {
  //   configTime(NTP_gmtOffset_sec, NTP_daylightOffset_sec, NTP_SERVER);
  //   time(&startup_unix_timestamp);
  // }
}

void loop()
{
  static persistance::dataframe df{
      .system_id = uint16_t(SYS_ID),
      .recording_id = 0,
      .timestamp = 0,
      .uptime = 0,
      .sensors = {0},
      .mystery_data = 0,
  };
  
  // lifecycle updates
  monEcran.refresh();
  mqtt.loop();
  
  auto now = millis();

  // update dataframe time
  df.uptime = uint64_t(now / 1000);
  df.timestamp = uint64_t(startup_unix_timestamp + df.uptime);

  // bool comms_online = false;
  // if (!comm_service.is_connected())
  // {
  //   comms_online = comm_service.connect(Serial);
  // }
  // else
  // {
  //   comms_online = true;
  // }

  auto is_time = main_timer.is_time_mut(now);
  if (is_time)
  {
    json.clear();

    df.recording_id += 1;

    {
      lora::dataframe lora_data;
      if (lora::read(lora_data, Serial))
      {
        df.mystery_data = lora_data.mystery_data;
        json["mystery"] = lora_data.mystery_data;
      }
    }
    {
      sensors::sht20_data sensor_data;
      if (sht20.read(sensor_data, Serial))
      {
        df.sensors.humidity_Perc = sensor_data.humidity_Perc;
        df.sensors.temperature_C = sensor_data.temperature_C;
        json["humidity"].set(sensor_data.humidity_Perc);
        json["temperature"].set(sensor_data.temperature_C);
      }
    }
    {
      sensors::bme280_data sensor_data;
      if (bme280.read(sensor_data, Serial))
      {
        df.sensors.pressure_kP = sensor_data.pressure_Pa;
        json["pressure"].set(sensor_data.pressure_Pa);
      }
    }
    {
      sensors::gy49_data sensor_data;
      if (gy49.read(sensor_data, Serial))
      {
        df.sensors.luminosity_lux = sensor_data;
        json["light level"].set(sensor_data);
      }
    }
  }

  // if (comms_online)
  // {
  //   comm_service.publish(json, Serial);
  // }

  // persistance::append_row(df, Serial); // TODO impl


  // printing info to the screen
  monEcran.effacer();
  monEcran.ecrire("rid: " + String(df.recording_id), 0);
  monEcran.ecrire("tem: " + String(df.sensors.temperature_C), 1);
  monEcran.ecrire("hum: " + String(df.sensors.humidity_Perc), 2);
  monEcran.ecrire("pre: " + String(df.sensors.pressure_kP), 3);
  monEcran.ecrire("lux: " + String(df.sensors.luminosity_lux), 4);
  monEcran.ecrire("mys: " + String(df.mystery_data), 5);
  monEcran.ecrire("upt: " + String(df.uptime), 6);
  monEcran.ecrire("time left: " + String(main_timer.get_time_left(now)/1000), 7);
}
