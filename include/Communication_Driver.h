#ifndef COMMUNICATION_DRIVER_H
#define COMMUNICATION_DRIVER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include "Config.h"     // UART_RX_PIN, UART_TX_PIN, SIM_CARD_PIN, APN, DEVICE_ID, uykuresi vb. için.
#include "secrets.h"    // AWS_IOT_ENDPOINT, AWS_IOT_PORT, MQTT_CLIENT_ID, AWS_IOT_PUBLISH_TOPIC vb. için.
#include "LoggerModule.h"
#include "NpkSensor.h"
#include "BME280Sensor.h"
#include "RTC_1302.h" 
// TinyGSM Libraries

#define TINY_GSM_MODEM_SIM808 // Modem türünü tanımla (GPS İÇİN)
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <SSLClient.h> // Güvenli bağlantı için

extern RTC_DATA_ATTR int wakeCounter;

class Communication_Driver {
public:
   // Oluşturucu: Modemin seri portuna ve sensör nesnelerine yapılan referanslarla başlatır
    Communication_Driver(HardwareSerial& modemSerialPort, NpkSensor& npkSensor, BME280Sensor& bmeSensor, RTC_Module& rtcModule);

    bool pwrmodem();
// Modemi kurar 
// Başarılı olduğunda true, başarısız olduğunda false döndürür.
    bool setupModem();

    // GPRS şebekesine bağlanır
// Başarılı olursa true, başarısız olursa false döndürür.
    bool connectGPRS();

    // MQTT broker'a bağlanır (henüz kurulmamışsa modem kurulumu ve GPRS bağlantısı dahildir)
// Başarılı olursa true, başarısız olursa false döndürür.
    bool connectMQTT();
    
    String getTimestampISO8601(); // New: For ISO8601 formatted timestamps
    String createCsvDataLine(); // New: To create a single CSV data row
    String createJsonPayloadForAWS(const String& csv_content); // New: To create the final JSON


    // Sensör verileri ve pil durumuyla JSON yük dizesini oluşturur.
// sup_bat_val: Harici olarak hesaplanan ana pil voltajı.
    String createJsonPayload();

    // Verileri MQTT konusuna yayınlar.
// sup_bat_val: Ana pil voltajı.
// Yayınlama başarılıysa true, aksi takdirde false döndürür.
    bool publishData(const char* payload);

    // MQTT istemci döngüsünü (örneğin, canlı tutma, mesaj işleme) işler. loop()'u çağırır.
    void mqttLoop();

    // MQTT istemcisinin şu anda bağlı olup olmadığını kontrol eder.
// Bağlıysa true, aksi takdirde false döndürür.
    bool isMqttConnected();

    // GPRS'in şu anda sürücünün modem örneği üzerinden bağlı olup olmadığını kontrol eder.
// GPRS bağlıysa true, aksi halde false döndürür.
    bool isGprsConnectedDriver(); 

    // MQTT ve GPRS bağlantısını keser ve modemi kapatır.
    void disconnect();

    // Dahili modem pil voltaj dizisini (_sup_4v) günceller.
    void updateModemBatteryStatus();

    //pil ve solar voltajını okur
    float readAndProcessBatteryVoltage();
    float readAndProcessSolarVoltage();

    // GPS'i etkinleştirir ve konum bilgilerini alır.
    bool enableGPS();
    bool getGPSLocation(float* lat, float* lon, float* speed = nullptr, float* alt = nullptr, int* year = nullptr, int* month = nullptr, int* day = nullptr, int* hour = nullptr, int* minute = nullptr, int* second = nullptr);
    bool readGPSWithRetry(int maxRetries = 20);
    bool disableGPS();
    

    /**
     * GPRS iletişimini yeniden başlatır.
     * Önce mevcut GPRS bağlantısını keser, sonra yeniden bağlanmaya çalışır.
     * Başarılı olursa 0, başarısız olursa 1.
     */
    int restartGPRS();

    /**
     * Modemi yeniden başlatır.
     * Modemi kapatır, açar ve yeniden başlatır.
     *  Başarılı olursa 0, başarısız olursa 1.
     */
    int restartModem();
    // ---- Yeni Fonksiyonlar Sonu ---

    // PubSubClient'ın gelen MQTT mesajlarını işlemesi için statik geri çağırma işlevi.
    static void staticMqttCallback(char* topic, byte* payload, unsigned int length);
    
    // MQTT mesajlarını işlemek için statik geri arama tarafından çağrılan statik olmayan işleyici.
    void handleMqttCallback(char* topic, byte* payload, unsigned int length);

private:
    HardwareSerial& _modemSerial; // Modemin donanım seri portuna referans
    NpkSensor& _npkSensor;        // NPK sensör nesnesine referans
    BME280Sensor& _bmeSensor;     // BME280 sensör nesnesine referans
    RTC_Module& _rtc; 
    
    TinyGsm _modem;               // TinyGSM modem nesnesi
    TinyGsmClient _gsmClient;     // GPRS için TinyGSM istemcisi
    SSLClient _secureClient;      // Güvenli MQTT için SSL istemcisi
    PubSubClient _mqttClient;     // MQTT client

    // GPS verilerini saklamak için değişkenler
    float _gps_lat;
    float _gps_lon;
    float _gps_speed;
    float _gps_alt;
    int _gps_year;
    int _gps_month;
    int _gps_day;
    int _gps_hour;
    int _gps_minute;
    int _gps_second;
    bool _gps_fix_available;
    char Location[50];

    // Communication_Driver.h dosyasının uygun bir yerine (örneğin #include'lardan sonra):
    

    float _sup_bat_external;
    float _sup_solar_external;

    char _sup_4v[6];              // Modemin pil voltajını bir dize olarak depolar (örneğin, "4.12V")
   // Statik MQTT geri araması tarafından kullanılan bu sınıfın örneğine ait statik işaretçi
    static Communication_Driver* _instance; 

};

#endif 
