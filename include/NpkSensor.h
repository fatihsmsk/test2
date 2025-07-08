#ifndef NPK_SENSOR_H
#define NPK_SENSOR_H

#include <Arduino.h>
#include <ModbusMaster.h>
#include "Config.h"

class NpkSensor {
public:
    NpkSensor(HardwareSerial &serial, uint8_t modbusAddress);
    int initializeAndRead();
    bool begin();
    bool readData();
    bool writeData();
    bool readNPKWithRetry(int maxRetries = 5);

    float getSicaklik() const { return SICAKLIK; }
    float getNem() const { return NEM; }
    float getEC() const { return EC; }
    float getPH() const { return PH; }
    float getAzot() const { return AZOT; }
    float getFosfor() const { return FOSFOR; }
    float getPotasyum() const { return POTASYUM; }
    float getSalinity() const { return TUZLULUK; }
    float getTDS() const { return TDS; }

    int getErrorCode() const { return npk_error_code; }

    // --- KALİBRASYON FONKSİYONLARI ---
    /**
     * @param factor 0-100 arası bir değer (0.0% - 10.0% arasına karşılık gelir).
     */
    bool setConductivityFactor(uint16_t factor);
    /**
     * @param factor 0-100 arası bir değer (0.00 - 1.00 arasına karşılık gelir).
     */
    bool setSalinityFactor(uint16_t factor);
    /**
     * @param factor 0-100 arası bir değer (0.00 - 1.00 arasına karşılık gelir).
     */
    bool setTDSFactor(uint16_t factor);
    bool setNitrogenFactor(float factor);
    bool setPhosphorusFactor(float factor);
    bool setPotassiumFactor(float factor);
    bool setTemperatureOffset(int16_t offset);
    bool setHumidityOffset(int16_t offset);
    bool setConductivityOffset(int16_t offset);
    bool setPHOffset(int16_t offset);
    bool setNitrogenOffset(int16_t offset);
    bool setPhosphorusOffset(int16_t offset);
    bool setPotassiumOffset(int16_t offset);
    void factorOffsetReset();


private:
    HardwareSerial &modbusSerial;
    ModbusMaster node;
    uint8_t modbusAddress;
    int npk_error_code = 0;

    float SICAKLIK, NEM, EC, PH, AZOT, FOSFOR, POTASYUM, TUZLULUK, TDS;

    void preTransmission();
    void postTransmission();

    static void preTransmissionStatic();
    static void postTransmissionStatic();
    static NpkSensor* instance;
};

#endif // NPK_SENSOR_H
