#include "NpkSensor.h"
#include "Config.h"
#include "LoggerModule.h" // Logger için gerekli

// **Statik üye değişkenleri**
NpkSensor* NpkSensor::instance = nullptr;

NpkSensor::NpkSensor(HardwareSerial &serial, uint8_t modbusAddress)
    : modbusSerial(serial), modbusAddress(modbusAddress) {
    instance = this;
}

void NpkSensor::begin() {
    pinMode(DE_RE, OUTPUT);
    digitalWrite(DE_RE, LOW);
    modbusSerial.begin(4800, SERIAL_8N1, RXX, TXX);

    node.begin(modbusAddress, modbusSerial);
    
    // **Statik üye fonksiyonları kullanarak yönlendirme**
    node.preTransmission(NpkSensor::preTransmissionStatic);
    node.postTransmission(NpkSensor::postTransmissionStatic);
}

// **Statik fonksiyonlar**
void NpkSensor::preTransmissionStatic() {
    if (instance) {
        instance->preTransmission();
    }
}

void NpkSensor::postTransmissionStatic() {
    if (instance) {
        instance->postTransmission();
    }
}

void NpkSensor::preTransmission() {
    digitalWrite(DE_RE, HIGH);
}

void NpkSensor::postTransmission() {
    digitalWrite(DE_RE, LOW);
}

bool NpkSensor::readData() {
    uint8_t result = node.readHoldingRegisters(0x00, 7);

    if (result == node.ku8MBSuccess) {
        NEM = node.getResponseBuffer(0) * 0.1;
        SICAKLIK = node.getResponseBuffer(1) * 0.1;
        EC = node.getResponseBuffer(2);
        PH = node.getResponseBuffer(3) * 0.1;
        AZOT = node.getResponseBuffer(4);
        FOSFOR = node.getResponseBuffer(5);
        POTASYUM = node.getResponseBuffer(6);

        npk_error_code = 0;
        return true;
    } else {
        Serial.print("Modbus Hatası: ");
        switch (result) {
            case node.ku8MBIllegalFunction:
                Serial.println("Illegal Function");
                npk_error_code = 1;
                break;
            case node.ku8MBIllegalDataAddress:
                Serial.println("Illegal Data Address");
                npk_error_code = 2;
                break;
            case node.ku8MBIllegalDataValue:
                Serial.println("Illegal Data Value");
                npk_error_code = 3;
                break;
            case node.ku8MBSlaveDeviceFailure:
                Serial.println("Slave Device Failure");
                npk_error_code = 4;
                break;
            default:
                Serial.println(result, HEX);
                npk_error_code = result;
        }
        return false;
    }
}

bool NpkSensor::readNPKWithRetry(int maxRetries) {
    bool NPK_success = false;
    int retryCount = 0;

    while (retryCount < maxRetries) {
        if (readData()) {
            LOG_INFO("NPK verisi başarıyla okundu.");
            NPK_success = true;
            break;
        }
        retryCount++;
        begin(); // tekrar başlat
        LOG_WARN("NPK okuma denemesi %d başarısız.", retryCount);
        delay(1000);
    }

    if (!NPK_success) {
        LOG_ERROR("NPK sensörüne %d denemede erişilemedi!", maxRetries);
    }

    return NPK_success;
}



