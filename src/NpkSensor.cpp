#include "NpkSensor.h"
#include "Config.h"
#include "LoggerModule.h" // Logger için gerekli

// **Statik üye değişkenleri**
NpkSensor* NpkSensor::instance = nullptr;

NpkSensor::NpkSensor(HardwareSerial &serial, uint8_t modbusAddress)
    : modbusSerial(serial), modbusAddress(modbusAddress) {
    instance = this;
}

int NpkSensor::initializeAndRead() {
    LOG_INFO("NPK sensörü başlatılıyor...");
    if (!begin()) {
        LOG_ERROR("NPK sensörü başlatılamadı. Okuma yapılmayacak.");
        return 1;
    }
    
    factorOffsetReset(); // Kalibrasyon faktörlerini sıfırla

    LOG_INFO("NPK sensörü başlatıldı. Veri okunuyor...");
    if (!readNPKWithRetry(5)) {
        LOG_ERROR("NPK sensöründen veri okunamadı.");
        return 2;
    }

    LOG_INFO("NPK sensörü başlatma ve okuma işlemi başarılı.");
    return 0;
}


bool NpkSensor::begin() {
    pinMode(DE_RE, OUTPUT);
    digitalWrite(DE_RE, LOW);
    modbusSerial.begin(4800, SERIAL_8N1, RXX, TXX);

    node.begin(modbusAddress, modbusSerial);
    node.preTransmission(NpkSensor::preTransmissionStatic);
    node.postTransmission(NpkSensor::postTransmissionStatic);

    // Sensörle test iletişimi yap: 0x00 adresinden 1 register oku
    uint8_t result = node.readHoldingRegisters(0x00, 1);
    if (result == node.ku8MBSuccess) {
        LOG_INFO("NPK sensör testi başarılı. Haberleşme kuruldu.");
        return true;
    } else {
        LOG_ERROR("NPK sensör testi başarısız. Modbus kodu: 0x%02X", result);
        return false;
    }
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

bool NpkSensor::writeData() {
   
    return true; // Başarılı olarak kabul edelim
}

bool NpkSensor::readData() {
    uint8_t result = node.readHoldingRegisters(0x00, 9);
    
    if (result == node.ku8MBSuccess) {
        NEM = node.getResponseBuffer(0) * 0.1;
        SICAKLIK = node.getResponseBuffer(1) * 0.1;
        EC = node.getResponseBuffer(2);
        PH = node.getResponseBuffer(3) * 0.1;
        AZOT = node.getResponseBuffer(4);
        FOSFOR = node.getResponseBuffer(5);
        POTASYUM = node.getResponseBuffer(6);
        TUZLULUK = node.getResponseBuffer(7);
        TDS = node.getResponseBuffer(8);

        npk_error_code = 0;
        return true;
    } else {
        LOG_ERROR("Modbus Hatası: ");
        switch (result) {
            case node.ku8MBIllegalFunction:
                LOG_ERROR("Illegal Function");
                npk_error_code = 1;
                break;
            case node.ku8MBIllegalDataAddress:
                LOG_ERROR("Illegal Data Address");
                npk_error_code = 2;
                break;
            case node.ku8MBIllegalDataValue:
                LOG_ERROR("Illegal Data Value");
                npk_error_code = 3;
                break;
            case node.ku8MBSlaveDeviceFailure:
                LOG_ERROR("Slave Device Failure");
                npk_error_code = 4;
                break;
            default:
                LOG_ERROR("Unknown Modbus Error: 0x%02X", result);
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
            LOG_INFO("Sicaklik: %.1f, Nem: %.1f, EC: %d, pH: %.1f, Azot: %d, Fosfor: %d, Potasyum: %d, Tuzluluk: %d, TDS: %d",
                     (int)(SICAKLIK*10)/10.0, (int)(NEM*10)/10.0, (int)EC, (int)(PH*10)/10.0, (int)AZOT, (int)FOSFOR, (int)POTASYUM, (int)TUZLULUK, (int)TDS);
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
    
// --- KALİBRASYON FONKSİYONLARININ IMPLEMENTASYONU ---

bool NpkSensor::setConductivityFactor(uint16_t factor) {
    uint8_t result = node.writeSingleRegister(0x0022, factor);
    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set Conductivity Factor to %d", factor);
        return true;
    }
    LOG_ERROR("Failed to set Conductivity Factor. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setSalinityFactor(uint16_t factor) {
    uint8_t result = node.writeSingleRegister(0x0023, factor);
    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set Salinity Factor to %d", factor);
        return true;
    }
    LOG_ERROR("Failed to set Salinity Factor. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setTDSFactor(uint16_t factor) {
    uint8_t result = node.writeSingleRegister(0x0024, factor);
    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set TDS Factor to %d", factor);
        return true;
    }
    LOG_ERROR("Failed to set TDS Factor. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setTemperatureOffset(int16_t offset) {
    uint8_t result = node.writeSingleRegister(0x0050, offset);
    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set Temperature Offset to %d", offset);

        return true;
    }
    LOG_ERROR("Failed to set Temperature Offset. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setHumidityOffset(int16_t offset) {
    uint8_t result = node.writeSingleRegister(0x0051, offset);
    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set Humidity Offset to %d", offset);
        return true;
    }
    LOG_ERROR("Failed to set Humidity Offset. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setConductivityOffset(int16_t offset) {
    uint8_t result = node.writeSingleRegister(0x0052, offset);
    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set Conductivity Offset to %d", offset);
        return true;
    }
    LOG_ERROR("Failed to set Conductivity Offset. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setPHOffset(int16_t offset) {
    uint8_t result = node.writeSingleRegister(0x0053, offset);
    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set PH Offset to %d", offset);
        return true;
    }
    LOG_ERROR("Failed to set PH Offset. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setNitrogenOffset(int16_t offset) {
    uint8_t result = node.writeSingleRegister(0x04EA, offset);
    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set Nitrogen Offset to %d", offset);
        return true;
    }
    LOG_ERROR("Failed to set Nitrogen Offset. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setPhosphorusOffset(int16_t offset) {
    uint8_t result = node.writeSingleRegister(0x04F4, offset);
    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set Phosphorus Offset to %d", offset);
        return true;
    }
    LOG_ERROR("Failed to set Phosphorus Offset. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setPotassiumOffset(int16_t offset) {
    uint8_t result = node.writeSingleRegister(0x04FE, offset);
    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set Potassium Offset to %d", offset);
        return true;
    }
    LOG_ERROR("Failed to set Potassium Offset. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setNitrogenFactor(float factor) {
    union {
        float float_val;
        uint32_t long_val;
    } val_union;
    val_union.float_val = factor;

    uint16_t highWord = (val_union.long_val >> 16);
    uint16_t lowWord = val_union.long_val & 0xFFFF;
    
    preTransmission();
    node.setTransmitBuffer(0, highWord);
    node.setTransmitBuffer(1, lowWord);
    uint8_t result = node.writeMultipleRegisters(0x04E8, 2); 
    postTransmission();

    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set Nitrogen Factor to %.2f", factor);
        return true;
    }
    LOG_ERROR("Failed to set Nitrogen Factor. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setPhosphorusFactor(float factor) {
    union {
        float float_val;
        uint32_t long_val;
    } val_union;
    val_union.float_val = factor;

    uint16_t highWord = (val_union.long_val >> 16);
    uint16_t lowWord = val_union.long_val & 0xFFFF;
    
    preTransmission();
    node.setTransmitBuffer(0, highWord);
    node.setTransmitBuffer(1, lowWord);
    uint8_t result = node.writeMultipleRegisters(0x04F2, 2); 
    postTransmission();

    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set Phosphorus Factor to %.2f", factor);
        return true;
    }
    LOG_ERROR("Failed to set Phosphorus Factor. Error: 0x%02X", result);
    return false;
}

bool NpkSensor::setPotassiumFactor(float factor) {
    union {
        float float_val;
        uint32_t long_val;
    } val_union;
    val_union.float_val = factor;

    uint16_t highWord = (val_union.long_val >> 16);
    uint16_t lowWord = val_union.long_val & 0xFFFF;
    
    preTransmission();
    node.setTransmitBuffer(0, highWord);
    node.setTransmitBuffer(1, lowWord);
    uint8_t result = node.writeMultipleRegisters(0x04FC, 2); 
    postTransmission();

    if (result == node.ku8MBSuccess) {
        LOG_INFO("Set Potassium Factor to %.2f", factor);
        return true;
    }
    LOG_ERROR("Failed to set Potassium Factor. Error: 0x%02X", result);
    return false;
}

void NpkSensor::factorOffsetReset(){

    LOG_INFO("Kalibrasyon ayarları yapılıyor...");
    bool calib_success = true;
    calib_success &=setTDSFactor(1); 
    calib_success &=setNitrogenFactor(1); // Azot için bir faktör ayarla
    calib_success &=setPhosphorusFactor(1); // Fosfor için bir faktör ayarla
    calib_success &=setPotassiumFactor(1); // Potasyum için bir faktör ayarla
    calib_success &=setConductivityFactor(1); // Elektriksel iletkenlik için bir faktör ayarla
    calib_success &=setSalinityFactor(1); // Tuzluluk için bir faktör ayarla
    calib_success &=setTemperatureOffset(0); 
    calib_success &=setPHOffset(0); // pH için bir ofset ayarla
    calib_success &=setConductivityOffset(0); // Elektriksel iletkenlik için bir ofset ayarla
    calib_success &=setNitrogenOffset(0); // Azot için bir ofset ayarla
    calib_success &=setPhosphorusOffset(0); // Fosfor için bir ofset ayarla
    calib_success &=setPotassiumOffset(0); // Potasyum için bir ofset ayarla
    calib_success &=setHumidityOffset(0); // Nem için bir ofset ayarla
    if (calib_success) {
        LOG_INFO("Kalibrasyon ayarları başarıyla gönderildi.");
    } else {
        LOG_ERROR("Kalibrasyon ayarları gönderilirken hata oluştu!");
    }
}