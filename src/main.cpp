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
#include "sensor.h"
#include "persistance.hpp"
#include "communication.hpp"
// #include "secrets.h"

void print_to_screen(Ecran &screen, persistance::dataframe &df, Timer &update_timer, Timer &screen_timer)
{
  auto now = millis();

  screen.effacer();
  // TODO: print time left before next send

  screen.ecrire("time_left: " + String(update_timer.get_time_left(now)), 1, 2);

  screen.ecrire("sid: " + String(df.system_id) + ", ", 1);
  screen.ecrire("rid: " + String(df.recording_id) + ", ", 2);
  screen.ecrire("upt: " + String(df.uptime) + ", ", 3);
  screen.ecrire("tim: " + String(df.timestamp) + ", ", 4);
  screen.ecrire("hum: " + String(df.sensors.humidity_Perc) + ", ", 5);
  screen.ecrire("tem: " + String(df.sensors.temperature_C) + ", ", 6);
  screen.ecrire("pre: " + String(df.sensors.pressure_kP) + ", ", 7);
  screen.ecrire("lux: " + String(df.sensors.luminosity_lux) + ", ", 8);
  screen.ecrire("mys: " + String(df.mystery_data) + ", ", 9);
}

Timer main_timer(60 * 1000),
    screen_scroll_timer(2 * 1000);

Ecran monEcran;

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;
  monEcran.begin();

  lora::init();
}

void loop()
{
  monEcran.refresh();
  auto now = millis();
  main_timer.is_time_mut(now);
  screen_scroll_timer.is_time_mut(now);

  persistance::dataframe df{};

  print_to_screen(monEcran, df, main_timer, screen_scroll_timer);
}
