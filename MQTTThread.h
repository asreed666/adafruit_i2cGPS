#ifndef MQTT_H
#define MQTT_H

void mqttThread();
void sendPub( int topic, float value);
//extern char ipAddress[];
constexpr int NUM_TOPICS = 19;
constexpr int TOPIC_LEN = 80;
#ifdef TARGET_CY8CKIT_062_WIFI_BT
#define FLIP 0
#else 
#define FLIP 1
#endif

constexpr int LEDON = 0 ^ FLIP;  // active low on cy8ckit pioneer
constexpr int LEDOFF = 1 ^ FLIP; // active low on cy8ckit pioneer
constexpr auto MQTT_BROKER = "192.168.1.174";
constexpr auto THING_NAME = "CCP01";
constexpr auto SUBSCRIBE_TOPIC = "CCP01/lthresh";
constexpr auto LIGHT_LEVEL_TOPIC = 0;
constexpr auto LIGHT_STATE_TOPIC = 1;
constexpr auto LIGHT_SWITCH_TOPIC = 2;
constexpr auto REDLED_TOPIC = 3;
constexpr auto GREENLED_TOPIC = 4;
constexpr auto BLUELED_TOPIC = 5;
constexpr auto ANNOUNCE_TOPIC = 6;
constexpr auto LIGHT_THRESH_TOPIC = 7;
constexpr auto LATITUDE_TOPIC = 8;
constexpr auto LONGITUDE_TOPIC = 9;
constexpr auto TEMPERATURE_TOPIC = 10;
constexpr auto TEMP_THRESH_TOPIC = 11;
constexpr auto RX_COUNT = 12;
constexpr auto TX_COUNT = 13;
constexpr auto TIME = 14;
constexpr auto STATUSLED_TOPIC = 15;
constexpr auto ORANGELED_TOPIC = 16;
constexpr auto HEATER_STATE_TOPIC = 17;
constexpr auto HEATER_SWITCH_TOPIC = 18;
constexpr auto REL_HUMIDITY_TOPIC = 19;

#endif