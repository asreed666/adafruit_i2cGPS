#include "mbed.h"

#include <iostream>
#include <iomanip>

#include "GUI.h"

#include "SparkFun_I2C_GPS_Arduino_Library.h"
#include "SHT40.h"
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

I2C i2c(P6_1, P6_0);
//Serial pc(USBTX, USBRX, 115200);
I2CGPS myI2CGPS;
SHT40 sht40(P6_1, P6_0);
string configString;

struct dataSet{
    float temperature = 0;
    float humidity = 0;
}THData;

// The TinyGPS++ object
TinyGPSPlus gps;
static void smartDelay(unsigned long ms);
static void printFloat(double val, bool valid, int len, int prec);
static void printInt(unsigned long val, bool valid, int len);
static void printDateTime(TinyGPSDate &d, TinyGPSTime &t);
static void printStr(const char *str, int len);

Thread mqttThreadHandle;
Thread displayThreadHandle;
bool dispUp = false;

// main() runs in its own thread in the OS
int main() {
      /* Initialise display  and mqtt connections and launch threads*/

  mqttThreadHandle.start(callback(mqttThread));
  displayThreadHandle.start(callback(displayThread));

  while (myI2CGPS.begin(i2c, 400000) == false) {
    cout<<("GPS Module failed to respond. Please check wiring.") << endl;
    ThisThread::sleep_for(500);
  }
  cout << ("GPS module found!") << endl;
  cout << ("Testing TinyGPS++ library v. ");
  cout << ( TinyGPSPlus::libraryVersion());
  cout << ("by Mikal Hart and adapted for Mbed by asr") << endl;

  /* if GPS module is found let us configure it */
  // setup PPS LED
  while(dispUp == false) {
      printf(".");
      thread_sleep_for(1000);
  }
  configString = myI2CGPS.createMTKpacket(285, ",4,25");
  myI2CGPS.sendMTKpacket(configString);

  cout << "\033[20;1H";
  cout << (
      "Sats HDOP Latitude   Longitude   Fix  Date       Time     Date Alt    "
      "Course Speed Card  Distance Course Card  Chars Sentences Checksum") << endl;
  cout << (
      "          (deg)      (deg)       Age                      Age  (m)    "
      "--- from GPS ----  ---- to London  ----  RX    RX        Fail")  << endl;
  cout << (
      "------------------------------------------------------------------------"
      "---------------------------------------------------------------") << endl;
  char lngBuff[32], latBuff[32];
  char tempBuff[32], humBuff[32];
  auto currLat=0.0;
  auto currLong=0.0;
  while (true) {
    static const double LONDON_LAT = 51.508131;
    static const double LONDON_LON = -0.128002;
    cout << "\033[21;1H";
  
    printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
//    GUI_DispStringAt("Num Stlts: ", 0, 40);
//    GUI_DispDecAt(gps.satellites.value(), 100, 40, 2);
    printInt(gps.hdop.value(), gps.hdop.isValid(), 5);
    printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
//    GUI_DispStringAt("latitude: ", 0, 60);
    printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
//    GUI_DispStringAt("longitude: ", 0, 80);
    if (gps.location.isValid()) {
        auto latitude = static_cast<float>(gps.location.lat());
        currLat = latitude;
        auto lngitude = static_cast<float>(gps.location.lng());
        currLong = lngitude;

//        sprintf(lngBuff,"%.2f", abs(gps.location.lng()));
    }
    else {
        strcpy(latBuff, "**.**.****");
        strcpy(lngBuff, "**.**.****");
    }
    smartDelay(0);
//    GUI_DispStringAt(latBuff, 100, 60);
//    GUI_DispStringAt(lngBuff, 100, 80);

    printInt(gps.location.age(), gps.location.isValid(), 5);
    printDateTime(gps.date, gps.time);
    printFloat(gps.altitude.meters(), gps.altitude.isValid(), 7, 2);
    printFloat(gps.course.deg(), gps.course.isValid(), 7, 2);
    printFloat(gps.speed.kmph(), gps.speed.isValid(), 6, 2);
    printStr(gps.course.isValid() ? TinyGPSPlus::cardinal(gps.course.value())
                                  : "*** ", 6);

    unsigned long distanceKmToLondon =
        (unsigned long)TinyGPSPlus::distanceBetween(
            gps.location.lat(), gps.location.lng(), LONDON_LAT, LONDON_LON) /
        1000;
    printInt(distanceKmToLondon, gps.location.isValid(), 9);

    double courseToLondon = TinyGPSPlus::courseTo(
        gps.location.lat(), gps.location.lng(), LONDON_LAT, LONDON_LON);

    printFloat(courseToLondon, gps.location.isValid(), 7, 2);

    const char *cardinalToLondon = TinyGPSPlus::cardinal(courseToLondon);

    printStr(gps.location.isValid() ? cardinalToLondon : "*** ", 6);

    printInt(gps.charsProcessed(), true, 6);
    printInt(gps.sentencesWithFix(), true, 10);
    printInt(gps.failedChecksum(), true, 9);
    float tempC = THData.temperature;
    float RH = THData.humidity;
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
  }
    
  do {
    while (myI2CGPS.available()) {
      gps.encode(myI2CGPS.read());
    }
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