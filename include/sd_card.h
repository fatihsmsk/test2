#ifndef SD_CARD_H
#define SD_CARD_H
#include "Config.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"



// Fonksiyon prototipleri
bool initSDCard();
int initializeAndRead(); 
// Tanılama fonksiyonu, satırın son elemanı olup olmadığını belirtmek için bool parametresi eklendi
void writeDiagnostics(const char* component, int statusCode, bool is_last = false); 
bool createFolder(fs::FS &fs, const char *path);
bool createFile(fs::FS &fs, const char *path);
String readFile(fs::FS &fs, const char *path);
bool writeFile(fs::FS &fs, const char *path, const char *message);
bool appendFile(fs::FS &fs, const char *path, const char *message);
bool deleteFile(fs::FS &fs, const char *path);
void listDir(fs::FS &fs, const char * dirname, uint8_t levels); // Yardımcı fonksiyon
bool ensureDataFileWithHeader(fs::FS &fs, const char *filePath, const char *headerContent);


#endif // SD_CARD_H
