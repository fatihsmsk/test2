#include <pgmspace.h>

#ifndef SECRETS_H
#define SECRETS_H

#define AWS_IOT_ENDPOINT "a2dtvw2fzqrl82-ats.iot.us-east-1.amazonaws.com"
#define AWS_IOT_PORT 8883

// Cihazınız için MQTT Client ID (Sağlanan THINGNAME)
#define MQTT_CLIENT_ID "test_connection"

// Veri gönderilecek MQTT Konusu (Sağlanan AWS_IOT_PUBLISH_TOPIC)
#define AWS_IOT_PUBLISH_TOPIC "test_topic"
// Abone olunacak MQTT Konusu (Sağlanan AWS_IOT_SUBSCRIBE_TOPIC - gerekirse kullanılır)
#define AWS_IOT_SUBSCRIBE_TOPIC "test_topic"

#endif // SECRETS_H
