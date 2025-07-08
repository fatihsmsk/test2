#include "RTC_1302.h"
#include <Config.h>
#include "LoggerModule.h"

// countof makrosu, C++'da dizi boyutunu güvenli bir şekilde almak için kullanılır.
#ifndef countof
#define countof(a) (sizeof(a) / sizeof(a[0]))
#endif

RTC_Module::RTC_Module(uint8_t io_pin, uint8_t sclk_pin, uint8_t ce_pin)
    : _myWire(io_pin, sclk_pin, ce_pin), _Rtc(_myWire) {}

void RTC_Module::begin() {
    LOG_INFO("RTC_Module: Başlatılıyor...");

    _Rtc.Begin();
    initializeRtc(); // RTC'yi yapılandır ve gerekirse zamanı ayarla
}

void RTC_Module::initializeRtc() {
    /*
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    LOG_INFO("RTC_Module: Derleme zamanı: ");
    printDateTimeToSerial(compiled);
    LOG_INFO("");

    if (!_Rtc.IsDateTimeValid()) {
        LOG_INFO("RTC_Module: RTC DateTime geçersiz! Derleme zamanına ayarlanıyor.");
        _Rtc.SetDateTime(compiled);
    }*/

    if (_Rtc.GetIsWriteProtected()) {
        LOG_INFO("RTC_Module: RTC yazmaya karşı korumalıydı, yazma etkinleştiriliyor.");
        _Rtc.SetIsWriteProtected(false);
    }

    if (!_Rtc.GetIsRunning()) {
        LOG_INFO("RTC_Module: RTC çalışmıyordu, şimdi başlıyor.");
        _Rtc.SetIsRunning(true);
    }
    RtcDateTime now = _Rtc.GetDateTime();
    printDateTimeToSerial(now);
    /*if (now < compiled) {
        LOG_INFO("RTC derleme zamanından daha eskidir! (DateTime güncelleniyor)");
        _Rtc.SetDateTime(compiled);
    } else if (now > compiled) {
        LOG_INFO("RTC derleme zamanından daha yenidir. (bu beklenen bir durumdur)");
        printDateTimeToSerial(now);
    } else if (now == compiled) {
        LOG_INFO("RTC derleme zamanı ile aynıdır! ");
        printDateTimeToSerial(now);
    }*/
}
    

RtcDateTime RTC_Module::getDateTime() {
    return _Rtc.GetDateTime();
}

void RTC_Module::printDateTimeToSerial(const RtcDateTime& dt) {
    char datestring[25]; // "AA/GG/YYYY SS:DD:ss" formatı için yeterli alan

    snprintf_P(datestring,
               countof(datestring),
               PSTR("%02u/%02u/%04u %02u:%02u:%02u"), // PSTR program belleğinden okur
               dt.Month(),
               dt.Day(),
               dt.Year(),
               dt.Hour(),
               dt.Minute(),
               dt.Second());
    LOG_INFO(datestring);
}

String RTC_Module::getTimestamp() {
    RtcDateTime now = _Rtc.GetDateTime();
    if (!now.IsValid()) {
        return "NOT_IMPLEMENTED_TIMESTAMP";
    }
    // YYYY-MM-DD HH:MM:SS formatı için
    char timestampBuffer[20]; 
    snprintf(timestampBuffer, sizeof(timestampBuffer), "%04u-%02u-%02u %02u:%02u:%02u",
             now.Year(),
             now.Month(),
             now.Day(),
             now.Hour(),
             now.Minute(),
             now.Second());
    return String(timestampBuffer);
}

void RTC_Module::setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    RtcDateTime newDateTime(year, month, day, hour, minute, second);
    if (newDateTime.IsValid()) {
        _Rtc.SetDateTime(newDateTime);
        LOG_INFO("RTC zamanı başarıyla ayarlandı.");
        printDateTimeToSerial(newDateTime);
    } else {
        LOG_ERROR("RTC için ayarlanan tarih/saat geçersiz.");
    }
}

int RTC_Module::initializeAndRead() {
    _Rtc.Begin();

    // Kurulum kontrolü: RTC'nin çalıştığından emin ol.
    if (!_Rtc.GetIsRunning()) {
        LOG_INFO("RTC was not running, attempting to start.");
        _Rtc.SetIsRunning(true);
        // Başlatmayı denedikten sonra tekrar kontrol et.
        if (!_Rtc.GetIsRunning()) {
            LOG_ERROR("RTC setup failed: Could not start the clock.");
            return 1; // setup_error
        }
    }
    
    // Kurulum kontrolü: Yazma korumasının kapalı olduğundan emin ol.
    if (_Rtc.GetIsWriteProtected()) {
        LOG_INFO("RTC was write-protected, disabling protection.");
        _Rtc.SetIsWriteProtected(false);
         if (_Rtc.GetIsWriteProtected()) {
            LOG_ERROR("RTC setup failed: Could not disable write protection.");
            return 1; // setup_error
        }
    }

    // Okuma kontrolü: Zamanı al ve geçerli olduğunu doğrula.
    RtcDateTime now = _Rtc.GetDateTime();
    if (!now.IsValid()) {
        LOG_ERROR("RTC read failed: The date/time is not valid.");
        return 2; // read_error
    }

    LOG_INFO("RTC initialize and read successful.");
    printDateTimeToSerial(now); // Mevcut zamanı logla
    return 0; // success
}


bool RTC_Module::isHealthy() {
    if (!_Rtc.GetIsRunning()) {
        LOG_WARN("RTC is not running.");
        return false;
    }
    if (!_Rtc.IsDateTimeValid()) {
        LOG_WARN("RTC date/time is not valid.");
        return false;
    }
    // Yılın mantıklı bir değer olup olmadığını kontrol et
    RtcDateTime now = _Rtc.GetDateTime();
    if (now.Year() < 2024) {
        LOG_WARN("RTC year seems incorrect (Year: %d).", now.Year());
        return false;
    }
    return true;
}
