#ifndef RTC_1302_H
#define RTC_1302_H

#include <Arduino.h>
#include <Config.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

class RTC_Module {
public:
    RTC_Module(uint8_t io_pin, uint8_t sclk_pin, uint8_t ce_pin);
    void begin();
    RtcDateTime getDateTime();
    String getTimestamp(); // "YYYY-MM-DD HH:MM:SS" formatında zaman damgası döndürür
    void printDateTimeToSerial(const RtcDateTime& dt); // Detaylı tarih/saat bilgisini Seri Port'a yazdırır

private:
    ThreeWire _myWire;
    RtcDS1302<ThreeWire> _Rtc;
    void initializeRtc(); // RTC başlangıç ayarlarını yapar
};

#endif // RTC_1302_H