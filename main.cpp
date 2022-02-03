#include "mbed.h"

#include "SparkFun_I2C_GPS_Arduino_Library.h"
#include "externs.h"

I2C i2c(P6_1, P6_0);
Serial pc(USBTX, USBRX, 115200);
I2CGPS myI2CGPS;
string configString;

// The TinyGPS++ object
TinyGPSPlus gps;
static void smartDelay(unsigned long ms);
static void printFloat(float val, bool valid, int len, int prec);
static void printInt(unsigned long val, bool valid, int len);
static void printDateTime(TinyGPSDate &d, TinyGPSTime &t);
static void printStr(const char *str, int len);

// main() runs in its own thread in the OS
int main() {

  while (myI2CGPS.begin(i2c, 400000) == false) {
    pc.printf("Module failed to respond. Please check wiring.\n");
    ThisThread::sleep_for(500);
  }
  pc.printf("GPS module found!\n");
  pc.printf("Testing TinyGPS++ library v. ");
  pc.printf("%s", TinyGPSPlus::libraryVersion());
  pc.printf("by Mikal Hart and adapted for Mbed by asr\n");

  pc.printf(
      "Sats HDOP Latitude   Longitude   Fix  Date       Time     Date Alt    "
      "Course Speed Card  Distance Course Card  Chars Sentences Checksum\n");
  pc.printf(
      "          (deg)      (deg)       Age                      Age  (m)    "
      "--- from GPS ----  ---- to London  ----  RX    RX        Fail\n");
  pc.printf(
      "------------------------------------------------------------------------"
      "---------------------------------------------------------------\n");

  /* if GPS module is found let us configure it */
  // setup PPS LED
  configString = myI2CGPS.createMTKpacket(285, ",4,25");
  myI2CGPS.sendMTKpacket(configString);

  while (true) {
    static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;

    printInt(gps.satellites.value(), gps.satellites.isValid(), 5);
    printInt(gps.hdop.value(), gps.hdop.isValid(), 5);
    printFloat(gps.location.lat(), gps.location.isValid(), 11, 6);
    printFloat(gps.location.lng(), gps.location.isValid(), 12, 6);
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
    pc.printf("\n");

    smartDelay(1000);

    if (clock_ms() > 5000 && gps.charsProcessed() < 10)
      pc.printf("No GPS data received: check wiring\n");
  }
}
/*
    while (myI2CGPS.available()) // available() returns the number of new bytes
                                 // available from the GPS module
    {
      uint8_t incoming = myI2CGPS.read(); //Read the latest byte from Qwiic GPS
      if(incoming == '$') pc.printf("\n"); //Break the sentences onto new lines
      pc.printf("%c", incoming); //Print this character
    }
    smartDelay(1000);
  }
  return 0;
}*/

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms) {
  unsigned long start = clock_ms();
  do {
    while (myI2CGPS.available())
      gps.encode(myI2CGPS.read());
  } while (clock_ms() - start < ms);
}

static void printFloat(float val, bool valid, int len, int prec) {
  if (!valid) {
    while (len-- > 1)
      pc.printf("*");
    pc.printf(" ");
  } else {
    pc.printf("%5.2f ", val);
    //    int vi = abs((int)val);
    //    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    //    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    //    for (int i=flen; i<len; ++i)
    //      pc.printf(' ');
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
  pc.printf("%s", sz);
  smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t) {
  if (!d.isValid()) {
    pc.printf("********** ");
  } else {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    pc.printf("%s", sz);
  }

  if (!t.isValid()) {
    pc.printf("******** ");
  } else {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    pc.printf("%s", sz);
  }

  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

static void printStr(const char *str, int len) {
  int slen = strlen(str);
  for (int i = 0; i < len; ++i)
    pc.printf("%c", i < slen ? str[i] : ' ');
  smartDelay(0);
}