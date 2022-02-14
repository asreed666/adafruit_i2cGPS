#ifndef CONSTANTS_H
#define CONSTANTS_H

#define col0    2
#define col1    55
#define title   1
#define timeL   2
#define levelL  4
#define threshL 5
#define switchL 6
#define stateL  7
#define localL  9
#define autoL   10
#define blueL   11
#define txL     12
#define rxL     13
#define stat0   14
#define stat1   15
#define stat2   16

constexpr auto titleL = 0;
constexpr auto titleR = 320;
constexpr auto midway = 160;

constexpr auto textLen = 80;

constexpr auto align8 = 8;
constexpr auto align64 = 64;
constexpr auto align128 = 128;

constexpr auto hundredthSec = 10;
constexpr auto tenthSec = 100;
constexpr auto oneSec = 1000;


constexpr auto defLightThresh = 50.0F;
constexpr auto defHumidThresh = 50.0F;
constexpr auto defTempThresh = 20.0F;
constexpr auto deadZoneL = 20;
constexpr auto deadZoneT = 5;


constexpr auto xScale = 3;
constexpr auto yScale = 12;
constexpr auto qLen = 32;
#endif