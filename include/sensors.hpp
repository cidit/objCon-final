
namespace sensors
{
  struct dataframe
  {
    float temperature_C, // provenance: SHT20
        humidity_Perc,   // provenance: SHT20
        pressure_kP,     // provenance: BME280
        luminosity_lux;  // provenance: GY-49
  };

}
