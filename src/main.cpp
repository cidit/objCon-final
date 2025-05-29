#include <Arduino.h>

#include <SPI.h>
#include <LoRa.h>
#include <Ecran.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <PubSubClient.h> // v2.8  https://pubsubclient.knolleary.net/
#include <ArduinoJson.h>  //Librairie de manipulation document JSON v6.21.0

#include "utils.hpp"
#include "timer.hpp"
#include "secrets.h"
#include <esp_wifi.h>

Ecran ecran;
time_t startup_unix_timestamp = 0;
JsonDocument json;

IntervalTimer timer(5 * 1000);

void handle_tb_message(char *topic, byte *payload, unsigned int length)
{
    static constexpr uint8_t
        GREEN_LED_PIN = 2,
        RED_LED_PIN = 4;

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(String(payload, length));

    json.clear();
    deserializeJson(json, payload, length);

    String which_led = json["params"]["led"];

    if (which_led == "red")
    {
        bool isHigh = json["params"]["state"];
        digitalWrite(RED_LED_PIN, isHigh ? HIGH : LOW);
    }
    else if (which_led == "green")
    {
        bool isHigh = json["params"]["state"];
        digitalWrite(GREEN_LED_PIN, isHigh ? HIGH : LOW);
    }
}

void setup()
{
    Serial.begin(115200);
    while (not Serial)
        ;

    ecran.begin();

    SHT20::init();
    BME280::init();
    GY49::init();
    LORA::init();
    WIFI::init();

    if (WIFI::connect())
    {
        configTime(NTP_gmtOffset_sec, NTP_daylightOffset_sec, NTP_SERVER);
        startup_unix_timestamp = WIFI::get_ntp_time();
        Serial.println("current unix time: " + String(startup_unix_timestamp));
    }
    
    MQTT::init();
    SDCARD::init();

    MQTT::mqtt_client.setCallback(handle_tb_message);

    Serial.println("Ready!");
}

void loop()
{
    static share_data df{
        .system_id = uint16_t(SYS_ID),
        .recording_id = 0,
        .timestamp = 0,
        .uptime = 0,
        .sensors = {0},
        .mystery_data = 0,
    };

    ecran.refresh();
    auto now = millis();

    df.uptime = now / 1000;
    df.timestamp = startup_unix_timestamp + df.uptime;

    auto is_comms_online = maintain_comms();

    if (timer.is_time_mut(now))
    {
        df.recording_id += 1;
        json.clear();

        {
            SHT20::data d;
            if (SHT20::read(d))
            {
                json["temperature_C"] = df.sensors.temperature_C = d.temperature_C;
                json["humidity_Perc"] = df.sensors.humidity_Perc = d.humidity_Perc;
            }
        }

        {
            BME280::data d;
            if (BME280::read(d))
            {
                json["pressure_kPa"] = df.sensors.pressure_kPa = d.pressure_Pa * 1000; // Pa -> kPa
            }
        }

        {
            GY49::data d;
            if (GY49::read(d))
            {
                json["luminosity_lux"] = df.sensors.luminosity_lux = d;
            }
        }

        {
            LORA::data d;
            if (LORA::read(d))
            {
                json["mystery_data"] = df.mystery_data = d.mystery_data;
            }
        }

        serializeJson(json, Serial);
        Serial.println();

        if (is_comms_online) {
            MQTT::publish(json);
        }
        SDCARD::append_row(df);
    }

    ecran.effacer();
    ecran.ecrire("rid: #" + String(df.recording_id), 0);
    ecran.ecrire("tem: " + String(df.sensors.temperature_C) + "C", 1);
    ecran.ecrire("hum: " + String(df.sensors.humidity_Perc) + "%", 2);
    ecran.ecrire("pre: " + String(df.sensors.pressure_kPa) + "kPa", 3);
    ecran.ecrire("lux: " + String(df.sensors.luminosity_lux) + "lux", 4);
    ecran.ecrire("mys: " + String(df.mystery_data) + "?", 5);
    ecran.ecrire("upt: " + String(df.uptime) + "s", 6);
    ecran.ecrire("time left: " + String(timer.get_time_left(now) / 1000) + "s", 7);
}
