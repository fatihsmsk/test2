#ifndef NPK_SENSOR_H
#define NPK_SENSOR_H

#include <Arduino.h>
#include <ModbusMaster.h>
#include "Config.h"

class NpkSensor {
public:
    NpkSensor(HardwareSerial &serial, uint8_t modbusAddress);
    void begin();
    bool readData();
    bool readNPKWithRetry(int maxRetries = 5);

    float getSicaklik() const { return SICAKLIK; }
    float getNem() const { return NEM; }
    float getEC() const { return EC; }
    float getPH() const { return PH; }
    float getAzot() const { return AZOT; }
    float getFosfor() const { return FOSFOR; }
    float getPotasyum() const { return POTASYUM; }
    int getErrorCode() const { return npk_error_code; }

private:
    HardwareSerial &modbusSerial;
    ModbusMaster node;
    uint8_t modbusAddress;
    int npk_error_code = 0;

    float SICAKLIK, NEM, EC, PH, AZOT, FOSFOR, POTASYUM;

    void preTransmission();
    void postTransmission();

    static void preTransmissionStatic();
    static void postTransmissionStatic();
    static NpkSensor* instance;
};

#endif // NPK_SENSOR_H
