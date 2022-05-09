#include "mbed.h"

#include <iostream>
#include <iomanip>

#include "GUI.h"

//#include "SparkFun_I2C_GPS_Arduino_Library.h"
//#include "SHT40.h"
#include "MQTTThread.h"
#include "display.h"
//#include "externs.h"
#include "constants.h"

struct things_t {
    float lightLevelVal = 100.0;
    float lightThreshVal = defLightThresh;
    float temperatureVal= defTempThresh;
    float tempThreshVal = defTempThresh;
    float relHumidVal= 0.0;
    float relHumidThreshVal = defHumidThresh;
    float currLatitude = 0;
    float currLongitude = 0;
    bool redLedState = false;
    bool greenLedState = false;
    bool blueLedState = false;
    bool statusLedState = false;
    bool orangeLedState = false;
    int rxCountVal = 0;
    int txCountVal = 0;
//    char ipAddress[17] = "192.168.1.xxx";
    bool changed{};

} __attribute__((aligned(align64))) ;

things_t thingData;

uint32_t timeToNext = 0;  // Timer for when to publish
uint32_t period = 30;     // Periodicity of publishing
bool publishIt = false;

//I2C i2c(P6_1, P6_0);
//Serial pc(USBTX, USBRX, 115200);
//I2CGPS myI2CGPS;
//SHT40 sht40(P6_1, P6_0);
string configString;

struct dataSet{
    float temperature = 0;
    float humidity = 0;
}THData;

// The TinyGPS++ object

Thread mqttThreadHandle;
Thread displayThreadHandle;
bool dispUp = false;

// main() runs in its own thread in the OS
int main() {
      /* Initialise display  and mqtt connections and launch threads*/

  mqttThreadHandle.start(callback(mqttThread));
  displayThreadHandle.start(callback(displayThread));


  /* if GPS module is found let us configure it */
  // setup PPS LED
  while(dispUp == false) {
      printf(".");
      thread_sleep_for(1000);
  }
    while (true) {
    float tempC = THData.temperature;
//    float RH = THData.humidity;
    printFloat(tempC, true, 6, 3);
    sprintf(tempBuff,"Temperature: %2.1fC", tempC);
//    GUI_DispStringAt(tempBuff, 0, 100);
    printFloat(RH, true, 6, 3);
    sprintf(humBuff,"Relative Humidity: %2.1f%%", RH);
//    GUI_DispStringAt(humBuff, 0, 120);
    
    cout << endl;

    // Shall we send some data to the broker?

    if (abs(clock_s()) >= timeToNext) {
        publishIt = true;
        timeToNext = abs(clock_s()) + period;
    }
    else {
        publishIt = false;
    }
//    bool publishIt = (gps.time.second() == 0);
    displayUpdate(LATITUDE_TOPIC, currLat);
    if (publishIt == true) sendPub(LATITUDE_TOPIC, currLat);
    displayUpdate(LONGITUDE_TOPIC, currLong);
    if (publishIt == true) sendPub(LONGITUDE_TOPIC, currLong);
    displayUpdate(TEMPERATURE_TOPIC, tempC);
    if (publishIt == true) sendPub(TEMPERATURE_TOPIC, tempC);
    displayUpdate(REL_HUMIDITY_TOPIC , RH);
    if (publishIt == true) sendPub(REL_HUMIDITY_TOPIC, RH);

    smartDelay(1000);

    if (clock_ms() > 5000 && gps.charsProcessed() < 10) {
      cout << ("No GPS data received: check wiring") << endl;
    }
  }
}
/*
    while (myI2CGPS.available()) // available() returns the number of new bytes
                                 // available from the GPS module
    {
      uint8_t incoming = myI2CGPS.read(); //Read the latest byte from Qwiic GPS
      if(incoming == '$') printf("\n"); //Break the sentences onto new lines
      printf("%c", incoming); //Print this character
    }
    smartDelay(1000);
  }
  return 0;
}*/

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms) {
  unsigned long start = clock_ms();
  if (ms > 500) {
      THData.temperature = sht40.tempCF();
      THData.humidity = sht40.relHumidF();

  thread_sleep_for(1);
  } while (clock_ms() - start < ms);
}

static void printFloat(double val, bool valid, int len, int prec) {
  if (!valid) {
    while (len-- > 1) {
      cout << ("*");
    }
    cout << (" ");
  } else {
    cout << std::setprecision(prec) << std::setw(len) << (val);
    //    int vi = abs((int)val);
    //    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    //    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    //    for (int i=flen; i<len; ++i)
    //      printf(' ');
  }
  smartDelay(0);
}

static void printInt(unsigned long val, bool valid, int len) {
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i = strlen(sz); i < len; ++i)
    sz[i] = ' ';
  if (len > 0)
    sz[len - 1] = ' ';
  cout << (char *)sz;
  smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t) {
  if (!d.isValid()) {
    cout << ("********** ");
  } else {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.day(), d.month(), d.year());
    GUI_DispStringAt( sz, 0, 20);
    cout << (char *)sz;
  }

  if (!t.isValid()) {
    printf("******** ");
  } else {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    GUI_DispStringAt( sz, 120, 20);
    cout << (char *) sz;
  }

  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

static void printStr(const char *str, int len) {
  int slen = strlen(str);
  for (int i = 0; i < len; ++i)
    cout << (i < slen ? str[i] : ' ');
  smartDelay(0);
}