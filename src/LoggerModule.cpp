#include "LoggerModule.h"
#include <stdarg.h>
#include <Config.h>

static const size_t LOG_QUEUE_SIZE  = LOGGER_QUEUE_SIZE;   // Kuyruktaki maksimum log mesaj sayısı
static const size_t MAX_LOG_LENGTH  = LOGGER_MAX_LOG_LENGTH;  // Log mesajlarının maksimum uzunluğu

static uint8_t s_logLevel      = LOGGER_LEVEL_ERROR;
static bool    s_serialEnabled = false;
static unsigned long lastPrintTime = 0;

// Log mesajlarını tutacak yapı
typedef struct {
    char msg[MAX_LOG_LENGTH];
    uint8_t level;
} LogItem_t;

static QueueHandle_t s_loggerQueue = nullptr;

void LoggerInit()
{
    s_logLevel = LOG_LEVEL_SELECTED;
    s_serialEnabled = LOG_SERIAL_ENABLED;

    Serial.begin(SERIAL_BAUD_RATE);
    
    // Kuyruk oluşturulmadıysa oluştur
    if (!s_loggerQueue) {
        s_loggerQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(LogItem_t));
    }
}

void LoggerPrint(uint8_t level, const char* func, int line, const char* format, ...)
{
    // Seçili log seviyesinden düşükse veya seri haberleşme kapalıysa mesajı atla
    if ((level > s_logLevel) || (!s_serialEnabled)) {
        return;
    }

    char buf[MAX_LOG_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    // Fonksiyon adı ve satır numarası ile mesajı oluştur
    char finalMsg[MAX_LOG_LENGTH];
    snprintf(finalMsg, sizeof(finalMsg), "[%s:%d] %s", func, line, buf);
    if (s_serialEnabled) {
        Serial.println(buf);  // Burada loglar seri porta yazılıyor olmalı
    }
    // Kuyruğa ekle
    if (s_loggerQueue) {
        LogItem_t item;
        strncpy(item.msg, finalMsg, MAX_LOG_LENGTH);
        item.msg[MAX_LOG_LENGTH-1] = '\0';
        item.level = level;
        xQueueSend(s_loggerQueue, &item, 0);
    }
}

void LoggerTask(void* pvParams)
{
    LogItem_t item;
    for (;;) {
        // Yeni log mesajı bekle
        if (xQueueReceive(s_loggerQueue, &item, portMAX_DELAY) == pdTRUE) {
            if (s_serialEnabled) {
                Serial.println(item.msg);
                
            }
        }
    }
}

// Belirli aralıklarla mesaj yazdırma fonksiyonu
void LoggerPrintLoopMessage(uint8_t* msg) {
    unsigned long currentTime = millis();
    
    // Eğer belirlenen süre geçtiyse log yazdır
    if (currentTime - lastPrintTime >= PRINT_INTERVAL) {
        lastPrintTime = currentTime;  

        // Mesajı HEX formatında oluştur
        String dbg;
        for (int i = 0; i < BLE_MSG_LENGTH; i++) {
            dbg += String(msg[i], HEX);
            dbg += " ";
        }
        LOG_INFO("TxMsg: %s", dbg.c_str());
    }
}
