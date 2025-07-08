
#include <Arduino.h>
#include <ArduinoJson.h> // May not be needed directly if all JSON is in driver
#include <inttypes.h>    // For PRIu64 definition
#include <HardwareSerial.h> // For serial communication
#include "LoggerModule.h"
#include "Config.h"
#include "NpkSensor.h"
#include "BME280Sensor.h"
#include "secrets.h"      // For secrets (though many are now used by driver)
#include "esp_sleep.h"
#include "esp_system.h"
#include <Wire.h>
#include <Adafruit_Sensor.h> // For BME280
#include "Communication_Driver.h" // Include the new communication driver
#include "sd_card.h"
#include "RTC_1302.h" //rtc nesnesi için gerekli kütüphane

RTC_Module rtc(RTC_IO_PIN, RTC_SCLK_PIN, RTC_CE_PIN); // RTC nesnemizi oluşturuyoruz

HardwareSerial modemSerial(2); // Modem için
HardwareSerial modbusSerial(1); // NPK için
NpkSensor npkSensor(modbusSerial, 1);  // Modbus adresi 1 olarak ayarlandı
BME280Sensor bmeSensor; // BME280 sensörü için nesne oluşturuldu

Communication_Driver commDriver(modemSerial, npkSensor, bmeSensor, rtc);

RTC_DATA_ATTR int wakeCounter = 0;

// Define the CSV header
const char* csvHeader = "measurement_id,measurement_time,soil_nitrogen,soil_phosphorus,soil_potassium,soil_humidity,soil_temperature,soil_electrical_conductivity,soil_ph,weather_air_temperature,weather_air_humidity,weather_air_pressure,system_solar_panel_voltage,system_battery_voltage,system_supply_4V\n";

void powerUpSensors() {
    digitalWrite(PWRBME, HIGH);
    digitalWrite(PWRNPK, HIGH);
    commDriver.pwrmodem(); // Modemi aç
    LOG_INFO("BME280 ve NPK sensörleri açıldı.");
}

void powerDownSensors() {
    digitalWrite(PWRBME, LOW);
    digitalWrite(PWRNPK, LOW);
    LOG_INFO("BME280 ve NPK sensörleri kapatıldı.");
}

void goToDeepSleep() {
    powerDownSensors(); // If you implement this
    commDriver.disconnect(); // Disconnect modem and power off
    LOG_INFO("ESP32 %" PRIu64 " mikrosaniye derin uyku moduna geçiyor...", (uint64_t)uykusuresi);
    Serial.flush(); // Flush serial buffer before sleep
    esp_sleep_enable_timer_wakeup(uykusuresi);
    esp_deep_sleep_start(); // Enter deep sleep
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    LoggerInit();
    pinMode(PWR_sim808, OUTPUT);
    pinMode(PWRBME, OUTPUT);
    pinMode(PWRNPK, OUTPUT);
    digitalWrite(PWR_sim808, HIGH); // Modem için PWR pinini HIGH yap(kendiliğinden açılmasını engellemek için)
    modemSerial.begin(9600, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);    
    powerUpSensors();
    rtc.begin(); 
    rtc.getTimestamp();
    
   
    LOG_INFO("Sistem Başlatılıyor...");
    if (initSDCard()) { // initSDCard() sd_card.h/cpp içinden çağrılıyor
        // SD Kart başarıyla başlatıldıysa, veri dosyasını ve başlığını kontrol et/oluştur
        if (ensureDataFileWithHeader(SD, SD_DATA_FILE, csvHeader)) { // YENİ FONKSİYON ÇAĞRISI
            LOG_INFO("Veri dosyası (%s) başlığıyla birlikte hazır.", SD_DATA_FILE);
        } else {
            LOG_ERROR("Veri dosyası (%s) için başlık oluşturulamadı/doğrulanamadı!", SD_DATA_FILE);
            // Burada kritik bir hata yönetimi yapılabilir, örneğin SD kartsız devam etme veya durma
        }
    } else {
        LOG_ERROR("SD Kart kullanılamıyor. Veri kaydı yapılamayacak.");
        // SD kart olmadan devam etme veya sistemi durdurma kararı burada verilebilir.
    }

    if (!bmeSensor.begin()) {
        LOG_ERROR("BME280 sensörü bulunamadı!");
    } else {
        LOG_INFO("BME280 Başarıyla Başlatıldı!");
    }

    npkSensor.begin(); 
    LOG_INFO("NPK Sensörü başlatıldı.");    
    LOG_INFO("Kurulum tamamlandı.");
}

void loop() {
    wakeCounter++;
    LOG_INFO("Loop başlatıldı - Wake count: %d", wakeCounter);
    powerUpSensors(); 
    commDriver.readAndProcessSolarVoltage(); // Güneş paneli voltajını oku
    commDriver.readAndProcessBatteryVoltage();
    commDriver.updateModemBatteryStatus(); // For _sup_4v
    bmeSensor.readBME280WithRetry(); 
    npkSensor.readNPKWithRetry();
    

    // Mevcut okumalar için CSV veri satırı oluştur
    String csvDataRow = commDriver.createCsvDataLine();
    String dataToLog = csvDataRow + "\n"; 

    if (appendFile(SD, SD_DATA_FILE, dataToLog.c_str())) {
        LOG_INFO("Veri başarıyla SD karta kaydedildi: %s", SD_DATA_FILE);
    } else {
        LOG_ERROR("SD karta veri kaydetme başarısız!");
    }

    if (wakeCounter >= MAX_WAKECOUNTER) {
        LOG_INFO("MAX_WAKECOUNTER'a ulaşıldı. Veri göndermeye hazırlanıyor.");
        
        // Gönderme için modem ve MQTT kurulumu
        if (!commDriver.setupModem()) { 
             LOG_ERROR("Modem kurulumu gönderilmeden önce başarısız oldu. Uyku moduna geçilecek.");
             goToDeepSleep();
             return; 
        }
       
        if (!commDriver.isMqttConnected()) {
            LOG_INFO("MQTT bağlantısı yok, bağlantı kuruluyor...");
          
            if (!commDriver.connectMQTT()) { 
                LOG_ERROR("MQTT bağlantısı kurulamadı. Veri SD kartta kalacak.");
                 goToDeepSleep(); 
                 return; 
            }
        }

        if (commDriver.connectMQTT()) {
            commDriver.mqttLoop(); // Process any incoming MQTT messages

            String fileContent = readFile(SD, SD_DATA_FILE);

            if (fileContent.length() > 0 && fileContent.indexOf('\n') != fileContent.lastIndexOf('\n')) { // Ensure there's more than just header
                LOG_INFO("SD karttan veriler okunuyor (%s), tamamı AWS JSON formatında gönderilecek.", SD_DATA_FILE);
                LOG_INFO("Gönderilecek toplam CSV veri boyutu: %d bytes", fileContent.length());

                String awsPayload = commDriver.createJsonPayloadForAWS(fileContent);
                
                if (commDriver.publishData(awsPayload.c_str())) {
                    LOG_INFO("AWS JSON başarıyla MQTT'ye gönderildi.");
                    LOG_INFO("SD karttaki veri dosyası siliniyor: %s", SD_DATA_FILE);
                    wakeCounter = 0; // Reset wake counter
                    if (deleteFile(SD, SD_DATA_FILE)) {
                        LOG_INFO("Dosya başarıyla silindi: %s", SD_DATA_FILE);
                        // Rewrite header for the new file
                        if (writeFile(SD, SD_DATA_FILE, csvHeader)) {
                           LOG_INFO("Başlık yeni %s dosyasına silme sonrası yazıldı.", SD_DATA_FILE);
                        } else {
                           LOG_ERROR("%s dosyasına silme sonrası başlık yazılamadı.", SD_DATA_FILE);
                        }
                    } else {
                        LOG_ERROR("Dosya silinemedi: %s", SD_DATA_FILE);
                    }
                } else {
                    LOG_ERROR("MQTT'ye AWS JSON gönderimi başarısız. Dosya silinmedi: %s", SD_DATA_FILE);
                    // Data remains on SD card for next attempt.
                    // Consider how to handle wakeCounter reset if send fails repeatedly.
                }
            } else {
                LOG_INFO("SD kartta gönderilecek yeterli veri yok (%s boş, sadece başlık veya okunamadı).", SD_DATA_FILE);
                 // If file is empty or only has header, maybe reset it.
                if (fileContent.length() > 0 && fileContent.indexOf('\n') == fileContent.lastIndexOf('\n') && wakeCounter > 0) {
                    LOG_INFO("Sadece başlık var gibi görünüyor, dosya siliniyor ve sayaç sıfırlanıyor.");
                    wakeCounter = 0;
                     if (deleteFile(SD, SD_DATA_FILE)) {
                        if (writeFile(SD, SD_DATA_FILE, csvHeader)) {
                           LOG_INFO("Başlık yeni %s dosyasına yazıldı.", SD_DATA_FILE);
                        } else {
                           LOG_ERROR("%s dosyasına başlık yazılamadı.", SD_DATA_FILE);
                        }
                    }
                }
            }
        } else {
            LOG_ERROR("MQTT bağlantısı yok. SD kart verileri şimdilik gönderilemiyor.");
        }
        delay(1000); 
    }
    LOG_INFO("Derin uyku moduna hazırlanılıyor...");
    goToDeepSleep(); 
}