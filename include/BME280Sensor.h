#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>

class BME280Sensor {
public:
    BME280Sensor();
    bool begin(uint8_t address = 0x76);
    bool readData();
    bool readBME280WithRetry(int maxRetries = 5);
    float getTemperature() const { return temperature; }
    float getHumidity() const { return humidity; }
    float getPressure() const { return pressure; }

private:
    Adafruit_BME280 bme;
    float temperature;
    float humidity;
    float pressure;
};

#endif // BME280_SENSOR_H
