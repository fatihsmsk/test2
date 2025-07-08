#ifndef CONFIG_H
#define CONFIG_H

// /////////////////////////////////////////////////////////////////
// ''''''' GENEL KONFİGÜRASYON ''''''''''''''''''' //

#define BLE_MSG_LENGTH 39  // BLE mesaj uzunluğu (Örnek değer)
// Seri haberleşme loglarını aç/kapat
#define LOG_SERIAL_ENABLED  1

// Seri haberleşme baud hızı
#define SERIAL_BAUD_RATE 115200


// Cihaz ID'si 
#define DEVICE_ID "test" // Cihaz ID'si (örneğin, "001", "002", ...)*********************

//#define uykusuresi (20 * 1000000)  // 30 saniye**********************
#define uykusuresi (3600ULL * 1000000ULL) // 1 saat = 3.600.000.000 µs (64-bit güvenli)*************************
//#define uykusuresi (7200LL * 1000000LL)  // 2 saat = 7.200.000.000 µs (64-bit güvenli)*************************
#define MAX_WAKECOUNTER 1    // verinin gönderileceği maksimum uyanma sayısı
// SD karta kaydedilecek dosyanın adını bir değişkende tutalım
#define SD_DATA_FILE "/sensor_data.log"

// GPS Bağlantı Parametreleri
#define CommStack_ConnGPS_DelayRetry 2000 // GPS bağlantı denemesi gecikmesi (ms) - Varsayılan 2 saniye
#define CommStack_ConnGPS_MaxRetry   5    // Maksimum GPS bağlantı deneme sayısı - Varsayılan 

// ESP32 UART pinleri (SIM808 için)
#define UART_RX_PIN 16
#define UART_TX_PIN 17
#define PWR_sim808 5  // SİM808 açma/kapama pini


#define BAT_VOLTAGE_PIN 34 //batarya voltajı ölçüm pini 
#define SOLAR_VOLTAGE_PIN 35 //solar panel voltajı ölçüm pini 

#define SOLAR_VOLTAGE_DIVIDER_R1 4700.0 // VCC ile ADC pini arasındaki direnç (Ohm)  
#define SOLAR_VOLTAGE_DIVIDER_R2 3300.0 // ADC pini ile GND arasındaki direnç (Ohm)  

// Voltaj Ölçüm Ayarları
#define VOLTAGE_DIVIDER_R1 1000.0 // batarya VCC ile ADC pini arasındaki direnç (Ohm)  
#define VOLTAGE_DIVIDER_R2 3300.0 // batarya ADC pini ile GND arasındaki direnç (Ohm)  
#define ADC_VREF 3.42            // ADC referans voltajı (V) -0.10v device1 3.32 **************************
#define ADC_MAX_VALUE 4095.0      // ADC Maksimum Değeri (12-bit için 4095)  

//SD KART PINLERİ
#define SD_CS_PIN 15
#define SPI_MOSI_PIN 4
#define SPI_MISO_PIN 19
#define SPI_SCK_PIN  18

//RTC 1302 PINLERİ
#define RTC_IO_PIN   33  // DAT
#define RTC_SCLK_PIN 32  // CLK
#define RTC_CE_PIN   25  // RST

// NPK sensörü pinleri (Modbus için)
#define RXX 14
#define TXX 26
#define DE_RE 27  // RS485 yön kontrol pini
#define PWRNPK 13 // NPK sensörünü açma/kapama pini

//      BME280 Sensör Tanımları
#define I2C_SDA 21  // ESP32'nin SDA pini
#define I2C_SCL 22  // ESP32'nin SCL pini
#define SEALEVELPRESSURE_HPA (1013.25) // Deniz seviyesindeki varsayılan basınç
#define PWRBME 23//BME280

// SIM kart PIN numarası (varsa girin, yoksa boş bırakın)
#define SIM_CARD_PIN ""
#define APN "internet" // Operatörünüzün APN'i
#define APN_USERNAME ""
#define APN_PASSWORD ""

// ''''''' LOGGER AYARLARI ''''''''''''''''''' //
// ... (Mevcut Logger ayarları) ...
#ifndef CORE_DEBUG_LEVEL
#define CORE_DEBUG_LEVEL 4  // Varsayılan log seviyesi
#endif

#define LOG_LEVEL_SELECTED  CORE_DEBUG_LEVEL // Seçili log seviyesi

// Log seviyeleri
#define LOGGER_LEVEL_ERROR  1
#define LOGGER_LEVEL_WARN   2
#define LOGGER_LEVEL_INFO   3
#define LOGGER_LEVEL_DEBUG  4

// Log mesaj kuyruğu ayarları
#define LOGGER_QUEUE_SIZE  10
#define LOGGER_MAX_LOG_LENGTH  512

// Logger task stack boyutu
#define LOGGER_TASK_STACK_SIZE   16384

// Log mesajlarını ne sıklıkla göstereceğiz? (ms)
#define PRINT_INTERVAL 1000
#define DEFAULT_LOOP_INTERVAL_MS  5000 // Veri gönderme aralığı


#ifdef LOG_LEVEL_SELECTED
    #if LOG_LEVEL_SELECTED == LOGGER_LEVEL_DEBUG
        #define LOOP_INTERVAL_MS   DEFAULT_LOOP_INTERVAL_MS  // Debug modunda da aynı kalsın veya ayarlayın
    #else
        #define LOOP_INTERVAL_MS   DEFAULT_LOOP_INTERVAL_MS
    #endif
#else
    #define LOOP_INTERVAL_MS   DEFAULT_LOOP_INTERVAL_MS
#endif

#endif // CONFIG_H