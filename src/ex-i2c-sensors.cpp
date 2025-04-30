#include <Arduino.h>

/*
 * Complete Project Details https://randomnerdtutorials.com
 */

/****************************************/
// Distributed with a free-will license.
// Use it any way you want, profit or free, provided it fits in the licenses of its associated works.
// MAX44009
// This code is designed to work with the MAX44009_I2CS I2C Mini //Module available from ControlEverything.com.
// https://www.controleverything.com/products

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

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
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    }

    Serial.println();
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


void get_bme280()
{
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" *C");

    // Convert temperature to Fahrenheit
    /*Serial.print("Temperature = ");
    Serial.print(1.8 * bme.readTemperature() + 32);
    Serial.println(" *F");*/

    Serial.print("Pressure = ");
    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.println();
}

void setup()
{
    // Initialise I2C communication as MASTER
    Wire.begin();
    // Initialise serial communication, set baud rate = 9600
    Serial.begin(115200);

    setup_gy49();
    setup_bme280();

}

void loop()
{
    float luminance = get_gy49();
    // Output data to serial monitor
    Serial.print("Ambient Light luminance :");
    Serial.print(luminance);
    Serial.println(" lux");

    get_bme280();
    delay(300);
}