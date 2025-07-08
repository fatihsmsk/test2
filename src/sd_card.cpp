#include "sd_card.h"
#include <Arduino.h> // Serial print için
#include "LoggerModule.h"
#include "Config.h" 

bool initSDCard() {
   
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN); // SCK, MISO, MOSI, CS 

    if (!SD.begin(SD_CS_PIN)) { // SD_CS_PIN Config.h dosyasında tanımlı olmalı
        LOG_ERROR("SD Kart başlatılamadı veya takılı değil!");
        return false;
    }
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        LOG_ERROR("SD kart takılı değil!");
        return false;
    }
    
    LOG_INFO("SD Kart başarıyla başlatıldı.");
     return true;
}

bool createFolder(fs::FS &fs, const char *path) {
    Serial.printf("Klasör oluşturuluyor: %s\n", path);
    if (fs.mkdir(path)) {
        Serial.println("Klasör oluşturuldu");
        return true;
    } else {
        Serial.println("Klasör oluşturma başarısız");
        return false;
    }
}

bool createFile(fs::FS &fs, const char *path) {
    Serial.printf("Dosya oluşturuluyor: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Dosya oluşturma başarısız");
        return false;
    }
    file.close();
    Serial.println("Dosya oluşturuldu (boş)");
    return true;
}

String readFile(fs::FS &fs, const char *path) {
    Serial.printf("Dosya okunuyor: %s\n", path);
    File file = fs.open(path, FILE_READ);
    if (!file) {
        Serial.println("Dosya okuma için açılamadı");
        return "";
    }

    String fileContent = "";
    while (file.available()) {
        fileContent += (char)file.read();
    }
    file.close();
    Serial.println("Dosya okuma tamamlandı.");
    // Serial.println(fileContent); // İçeriği yazdırmak için
    return fileContent;
}

bool writeFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Dosyaya yazılıyor: %s\n", path);
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Dosya yazma için açılamadı");
        return false;
    }
    if (file.print(message)) {
        Serial.println("Dosyaya yazıldı");
        file.close();
        return true;
    } else {
        Serial.println("Dosyaya yazma başarısız");
        file.close();
        return false;
    }
}

bool appendFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Dosyaya ekleniyor: %s\n", path);
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Dosya ekleme için açılamadı");
        return false;
    }
    if (file.print(message)) {
        Serial.println("Dosyaya eklendi");
        file.close();
        return true;
    } else {
        Serial.println("Dosyaya ekleme başarısız");
        file.close();
        return false;
    }
}

bool deleteFile(fs::FS &fs, const char *path) {
    Serial.printf("Dosya siliniyor: %s\n", path);
    if (fs.remove(path)) {
        Serial.println("Dosya silindi");
        return true;
    } else {
        Serial.println("Dosya silme başarısız");
        return false;
    }
}

// Yardımcı Fonksiyon: Dizini listelemek için
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
    Serial.printf("Dizin listeleniyor: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Dizin açılamadı");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Bu bir dizin değil");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                listDir(fs, file.name(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

bool ensureDataFileWithHeader(fs::FS &fs, const char *filePath, const char *headerContent) {
    LOG_INFO("Veri dosyası kontrol ediliyor: %s", filePath);
    if (!fs.exists(filePath)) {
        LOG_INFO("Dosya %s mevcut değil. Oluşturuluyor ve başlık yazılıyor.", filePath);
        if (writeFile(fs, filePath, headerContent)) {
            LOG_INFO("Başlık %s dosyasına yazıldı.", filePath);
            return true;
        } else {
            LOG_ERROR("%s dosyasına başlık yazılamadı.", filePath);
            return false;
        }
    } else {
        File dataFile = fs.open(filePath, FILE_READ);
        bool isEmpty = true;
        if (dataFile) {
            if (dataFile.size() > 0) {
                isEmpty = false;
            }
            dataFile.close();
        } else {
            LOG_ERROR("%s dosyasının boyutu kontrol edilemedi.", filePath);
            // Dosya açılamıyorsa, yine de başlığı yazmayı deneyebilir veya hata olarak kabul edebilirsiniz.
            // Şimdilik, dosyanın var olduğunu ancak açılamadığını varsayarak false dönüyoruz.
            return false;
        }

        if (isEmpty) {
            LOG_INFO("%s mevcut ama boş. Başlık yazılıyor.", filePath);
            // Boş dosyaya appendFile kullanmak yerine writeFile daha uygun olabilir,
            // ancak mevcut appendFile fonksiyonunuzla devam edelim.
            // Eğer dosya gerçekten boşsa, appendFile yeni içerik ekleyecektir.
            // Ancak, başlığın tekrar tekrar eklenmemesi için,
            // writeFile daha güvenli olabilir ya da appendFile öncesi dosyanın gerçekten boş olduğundan emin olunmalı.
            // Şimdilik appendFile ile devam edelim, çünkü orijinal kodda da benzer bir mantık var.
            if (appendFile(fs, filePath, headerContent)) {
                LOG_INFO("Başlık boş olan %s dosyasına eklendi.", filePath);
                return true;
            } else {
                LOG_ERROR("Başlık boş olan %s dosyasına eklenemedi.", filePath);
                return false;
            }
        } else {
            LOG_INFO("%s mevcut ve boş değil. Başlık yeniden yazılmadı.", filePath);
            return true; // Dosya zaten var ve dolu, işlem başarılı.
        }
    }
}