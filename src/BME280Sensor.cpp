#include "BME280Sensor.h"
#include <Wire.h>
#include "Config.h"
#include "LoggerModule.h" // Logger için gerekli

BME280Sensor::BME280Sensor()
    : temperature(0), humidity(0), pressure(0) {}

bool BME280Sensor::begin(uint8_t address) {
    Wire.begin(I2C_SDA, I2C_SCL);
    return bme.begin(address);
}

int BME280Sensor::initializeAndRead() {
    // Sensörü başlatmayı dene
    if (!begin()) {
        LOG_ERROR("BME280 sensor setup failed.");
        return 1; // setup_error
    }

    // Sensörden veri okumayı dene
    if (!readBME280WithRetry(5)) {
        LOG_ERROR("BME280 sensor read failed.");
        return 2; // read_error
    }
    LOG_INFO("BME280 sensor initialized and data read successfully.");
    return 0; // success
}

bool BME280Sensor::readData() {
    temperature = bme.readTemperature(); // °C
    humidity = bme.readHumidity();       // %
    pressure = bme.readPressure() / 100.0F; // hPa
    LOG_INFO("BME280 verisi okundu: Sıcaklık: %.2f °C, Nem: %.2f %%, Basınç: %.2f hPa", temperature, humidity, pressure);
    return true; // Okuma başarılı sayılır, hata kontrolü istersen buraya eklenebilir
}

bool BME280Sensor::readBME280WithRetry(int maxRetries) {
    bool BME280_success = false;
    int retryCount = 0;

    while (retryCount < maxRetries) {
        if (begin()) {
            if (readData()) {
                LOG_INFO("BME280 verisi başarıyla okundu.");
                BME280_success = true;
                break;
            }
        }
        retryCount++;
        LOG_WARN("BME280 okuma denemesi %d başarısız.", retryCount);
        begin(); // tekrar başlat
        delay(1000);
    }

    if (!BME280_success) {
        LOG_ERROR("BME280 sensörüne %d denemede erişilemedi!", maxRetries);
    }

    return BME280_success;
}


