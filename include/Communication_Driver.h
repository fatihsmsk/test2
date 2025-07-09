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

#include <RunningAverage.h>
#include <Update.h> // OTA için eklendi
#include <ArduinoHttpClient.h> // OTA için eklendi

// TinyGSM Libraries
#define TINY_GSM_MODEM_SIM808 // Modem türünü tanımla (GPS İÇİN)
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <SSLClient.h> // Güvenli bağlantı için


extern RTC_DATA_ATTR int wakeCounter;

class Communication_Driver {
public:
    Communication_Driver(HardwareSerial& modemSerialPort, NpkSensor& npkSensor, BME280Sensor& bmeSensor, RTC_Module& rtcModule);

    int getLastStatusCode();
    void resetLastStatusCode();

    bool pwrmodem();
    bool setupModem();
    int restartModem();
    void disconnect();
    
    bool connectGPRS();
    int restartGPRS();

    bool connectMQTT();
    void mqttLoop();
    bool isMqttConnected();
    static void staticMqttCallback(char* topic, byte* payload, unsigned int length);

    String createCsvDataLine(); // New: To create a single CSV data row
    String createJsonPayloadForAWS(const String& csv_content); // New: To create the final JSON
    bool publishData(const char* payload);
    void publishGpsData(); // GPS isteğini işleyen fonksiyon
    
    bool enableGPS();
    bool disableGPS();
    bool getGPS(float* lat, float* lon, float* speed = nullptr, float* alt = nullptr, int* year = nullptr, int* month = nullptr, int* day = nullptr, int* hour = nullptr, int* minute = nullptr, int* second = nullptr);
    bool readGPSWithRetry(int maxRetries = 50);
    volatile bool gps_request_flag = false;
    void updateRtcWithGpsTime();
    
    void updateModemBatteryStatus();
    float readAndProcessBatteryVoltage();
    float readAndProcessSolarVoltage();

    volatile bool ota_request_flag = false;
    String ota_url;
    String ota_version_id;
    void performOTA(const char* ota_url, const char* version_id);
    

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
    char gpsTime[25];
    
    int _last_status_code; // Sunucudan gelen son durum kodunu saklar

    float _sup_bat_external;
    float _sup_solar_external;
    char _sup_4v[6];              // Modemin pil voltajını bir dize olarak depolar (örneğin, "4.12V")
    RunningAverage _batteryVoltageAvg;
    RunningAverage _solarVoltageAvg;

    static Communication_Driver* _instance; 
    // Yardımcı fonksiyon: SHA-256 hash'i hesaplar
    String calculateSHA256(const String& input);

    


};

#endif
