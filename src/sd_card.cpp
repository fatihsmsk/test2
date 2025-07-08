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

int initializeAndRead() {
    // SD kartı başlatmayı dene.
    // initSDCard() zaten loglama yapıyor.
    if (!initSDCard()) {
        return 1; // setup_error
    }

    // SD kart için, başarılı bir başlatma "okunabilir" olduğu anlamına gelir.
    // Belirli bir "okuma hatası" (2), kartın kendisiyle değil, dosya işlemleriyle ilgili olurdu.
    return 0; // success
}

void writeDiagnostics(const char* component, int statusCode, bool is_last) {
    const char* diagnosticFilePath = DIAGNOSTICS_FILE;
    const long maxFileSize = 100 * 1024; // 100 KB

    // Dosya boyutunu kontrol et
    if (SD.exists(diagnosticFilePath)) {
        File diagnosticFile = SD.open(diagnosticFilePath, FILE_READ);
        if (diagnosticFile) {
            if (diagnosticFile.size() > maxFileSize) {
                diagnosticFile.close(); // Dosyayı silmeden önce kapat
                LOG_WARN("Tanılama dosyası boyutu 100KB'ı aşıyor. Silme ve yeniden oluşturma.");
                SD.remove(diagnosticFilePath);
            } else {
                diagnosticFile.close();
            }
        }
    }

    // Durum mesajını oluştur
    String statusMessage = String(component);
    switch (statusCode) {
        case 0:
            statusMessage += "_success";
            break;
        case 1:
            statusMessage += "_setup_error";
            break;
        case 2:
            statusMessage += "_read_error";
            break;
        default:
            statusMessage += "_unknown_error_";
            statusMessage += String(statusCode);
            break;
    }
    
    // Virgül gerekip gerekmediğini kontrol et
    bool needs_comma = false;
    File file = SD.open(diagnosticFilePath, FILE_READ);
    if (file && file.size() > 0) {
        file.seek(file.size() - 1);
        if (file.read() != '\n') {
            needs_comma = true;
        }
    }
    file.close();

    String message_to_append;
    if (needs_comma) {
        message_to_append += ",";
    }
    message_to_append += statusMessage;

    if (is_last) {
        message_to_append += "\n";
    }

    // Mesajı dosyaya ekle
    if (!appendFile(SD, diagnosticFilePath, message_to_append.c_str())) {
        LOG_ERROR("tanılama dosyasına yazılamadı!");
    } else {
        // Başarılı yazma logunu kapatabiliriz, çok fazla log oluşturabilir.
        // LOG_INFO("Diagnostic written: %s", statusMessage.c_str());
    }
}

bool createFolder(fs::FS &fs, const char *path) {
    LOG_INFO("Klasör oluşturuluyor: %s", path);
    if (fs.mkdir(path)) {
        LOG_INFO("Klasör oluşturuldu");
        return true;
    } else {
        LOG_ERROR("Klasör oluşturma başarısız");
        return false;
    }
}

bool createFile(fs::FS &fs, const char *path) {
    LOG_INFO("Dosya oluşturuluyor: %s", path);
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        LOG_ERROR("Dosya oluşturma başarısız");
        return false;
    }
    file.close();
    LOG_INFO("Dosya oluşturuldu (boş)");
    return true;
}

String readFile(fs::FS &fs, const char *path) {
    LOG_INFO("Dosya okunuyor: %s", path);
    File file = fs.open(path, FILE_READ);
    if (!file) {
        LOG_ERROR("Dosya okuma için açılamadı");
        return "";
    }

    String fileContent = "";
    while (file.available()) {
        fileContent += (char)file.read();
    }
    file.close();
    LOG_INFO("Dosya okuma tamamlandı.");
    // LOG_INFO(fileContent.c_str()); // İçeriği yazdırmak için
    return fileContent;
}

bool writeFile(fs::FS &fs, const char *path, const char *message) {
    LOG_INFO("Dosyaya yazılıyor: %s", path);
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        LOG_ERROR("Dosya yazma için açılamadı");
        return false;
    }
    if (file.print(message)) {
        LOG_INFO("Dosyaya yazıldı");
        file.close();
        return true;
    } else {
        LOG_ERROR("Dosyaya yazma başarısız");
        file.close();
        return false;
    }
}

bool appendFile(fs::FS &fs, const char *path, const char *message) {
    LOG_INFO("Dosyaya ekleniyor: %s", path);
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        LOG_ERROR("Dosya ekleme için açılamadı");
        return false;
    }
    if (file.print(message)) {
        LOG_INFO("Dosyaya eklendi");
        file.close();
        return true;
    } else {
        LOG_ERROR("Dosyaya ekleme başarısız");
        file.close();
        return false;
    }
}

bool deleteFile(fs::FS &fs, const char *path) {
    LOG_INFO("Dosya siliniyor: %s", path);
    if (fs.remove(path)) {
        LOG_INFO("Dosya silindi");
        return true;
    } else {
        LOG_ERROR("Dosya silme başarısız");
        return false;
    }
}

// Yardımcı Fonksiyon: Dizini listelemek için
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
    LOG_INFO("Dizin listeleniyor: %s", dirname);

    File root = fs.open(dirname);
    if (!root) {
        LOG_ERROR("Dizin açılamadı");
        return;
    }
    if (!root.isDirectory()) {
        LOG_ERROR("Bu bir dizin değil");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            LOG_INFO("  DIR : %s", file.name());
            if (levels) {
                listDir(fs, file.name(), levels - 1);
            }
        } else {
            LOG_INFO("  FILE: %s", file.name());
            LOG_INFO("  SIZE: %d", file.size());
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
