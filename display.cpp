/*
 * Copyright (c) 2006-2020 Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mbed.h"
#include "GUI.h"

#include "actuators.h"

#include "display.h"

//#include "main.h"

#include "MQTTThread.h"

#include "vt100.h"

#include "constants.h"

#include <iomanip>

#include <ios>

#include <iostream>
#include <sstream>

#include <string>

#include <array>

using display_t = struct  {
    std::array<char , textLen> dispTxt;    /* buffer for text version of data */
    int line;   /* position of data on the screen console */
    int column;   /* A counter value               */
} __attribute__((aligned(align128)));

static MemoryPool<display_t, qLen> mdpool;
static Queue<display_t, qLen> dqueue;
extern bool dispUp;
extern bool mqttUp;
float latitude = 0.0;
bool latNorth;
int latDeg;
int latMin;
float latSec;
float lngitude = 0.0;
bool lngWest;
int lngDeg;
int lngMin;
float lngSec;
char deg = 176;

void displayUpdate(int command, float value) {
    if (command == LATITUDE_TOPIC) {
        latitude = value;
        latNorth = (latitude < 0); // is position -ve 
        latitude = fabs(latitude);
        latDeg=static_cast<int>(latitude);
        latitude = (latitude - latDeg) * 60;
        latMin = static_cast<int>(latitude);
        latSec = (latitude - latMin) * 60;
    }
    if (command == LONGITUDE_TOPIC ) {
        lngitude = value;
        lngWest = (lngitude < 0); // is position -ve 
        lngitude = fabs(lngitude);
        lngDeg=static_cast<int>(lngitude);
        lngitude = (lngitude - lngDeg) * 60;
        lngMin = static_cast<int>(lngitude);
        lngSec = (lngitude - lngMin) * 60;
    }

    std::ostringstream buff;
    display_t *update = mdpool.alloc();
    time_t thetimeis = time(nullptr);
    switch(command) {
        case LIGHT_LEVEL_TOPIC :
            buff << "Light Level     = " << setw(4) << fixed << setprecision(1) << value << "%  ";
            strcpy(&update->dispTxt[0], buff.str().c_str());
            update->column = col0;
            update->line = levelL;
            dqueue.put(update,0,0);
            break;

        case LIGHT_STATE_TOPIC :
            buff << "Light State     = " << ((value != 0.0)?"  On ":" Off "); 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//           sprintf((char*)&update->dispTxt, "Light State     = %s",);
            update->column = col0;
            update->line = stateL;
            dqueue.put(update,0,0);
            break;

        case LIGHT_SWITCH_TOPIC :
            buff << "Light Switch    = " <<  ((value != 0.0F)?"  On ":" Off "); 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Light Switch    = %s", value?"  On":" Off");
            update->column = col0;
            update->line = switchL;
            dqueue.put(update,0,0);
            break;

        case REDLED_TOPIC :
            buff << "Red LED    = " <<  ((value != 0.0F)?" Off ":"  On "); 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Red LED    = %s", value?"Off":" On");
            update->column = col1;
            update->line = localL;
            dqueue.put(update,0,0);
            break;

        case GREENLED_TOPIC :
            buff << "Green LED  = " <<  ((value != 0.0F)?" Off ":"  On "); 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//          sprintf((char*)&update->dispTxt, "Green LED  = %s", value?"Off ":" On");
            update->column = col1;
            update->line = autoL;
            dqueue.put(update,0,0);
            break;

        case BLUELED_TOPIC :
            buff << "Blue LED   = " <<  ((value != 0.0F)?" Off ":"  On "); 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//          sprintf((char*)&update->dispTxt, "Blue LED   = %s", value?"Off":" On");
            update->column = col1;
            update->line = blueL;
            dqueue.put(update,0,0);
            break;

        case LIGHT_THRESH_TOPIC :
            buff << "Light Thresh    = " << setw(4) << fixed << setprecision(1) << value << "%  ";
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Light Threshold = %4.0f%c", value, 0x25);
            update->column = col0;
            update->line = threshL;
            dqueue.put(update,0,0);
            break;

        case LATITUDE_TOPIC :

            buff << "Latitude is " << setw(3) << latDeg << deg << ", " << latMin <<  "\', "
                 << setw(5) << setprecision(3) << fixed << latSec << "\" " << (latNorth?"South":"North"); 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Local Control mode is %s", value?"On ":"Off");
            update->column = col0;
            update->line = localL;
            dqueue.put(update,0,0);
            break;

        case LONGITUDE_TOPIC :

            buff << "Longitude is " << setw(3) << lngDeg << deg << ", " << lngMin <<  "\', "
                 << setw(5) << setprecision(3) << fixed << lngSec << "\" " << (lngWest?"West":"East"); 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Manual control mode is %s", value?"On ":"Off");
            update->column = col0;
            update->line = autoL;
            dqueue.put(update,0,0);
            break;

        case TEMPERATURE_TOPIC :
            buff << "Temperature   = " <<  fixed << setprecision(1) << value << "C  "; 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Temperature     = %6.1fC       ", value);
            update->column = col0;
            update->line = levelL;
            dqueue.put(update,0,0);
            break;
        case TEMP_THRESH_TOPIC :
            buff << "Temp Thresh   = " <<  fixed << setprecision(1) << value << "C  "; 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Temp Threshold  = %4.0fC", value);
            update->column = col1;
            update->line = threshL;
            dqueue.put(update,0,0);
            break;
        case RX_COUNT:
            buff << "Rx Count = " <<  fixed << setprecision(0) << value; 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Rx count = %d", (int)value);
            update->column = col0;
            update->line = rxL;
            dqueue.put(update,0,0);
            break;
        case TX_COUNT:
            buff << "Tx Count = " <<  fixed << setprecision(0) << value; 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Tx count = %d", (int)value);
            update->column = col0;
            update->line = txL;
            dqueue.put(update,0,0);
            break;

        case TIME:
            strftime((&update->dispTxt[0]), textLen, "%a, %d/%m/%Y %H:%M:%S   ", localtime(&thetimeis));
            update->column = col0;
            update->line = timeL;
            dqueue.put(update,0,0);
            break;
        case STATUSLED_TOPIC :
            buff << "Status LED = " <<  ((value != 0.0F)?" Off ":"  On "); 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Status LED = %s", value?"Off":" On");
            update->column = col1;
            update->line = txL;
            dqueue.put(update,0,0);
            break;
        case ORANGELED_TOPIC :
            buff << "Orange LED = " <<  ((value != 0.0F)?" Off ":"  On "); 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Orange LED = %s", value?"Off":" On");
            update->column = col1;
            update->line = rxL;
            dqueue.put(update,0,0);
            break;
        case HEATER_SWITCH_TOPIC :
            buff << "Heater Switch = " <<  ((value != 0.0F)?"  On ":" Off "); 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf(reinterpret_cast<char*>(&update->dispTxt), "Heater Switch   = %s", value != 0.0F?"  On":" Off");
            update->column = col1;
            update->line = switchL;
            dqueue.put(update,0,0);
            break;
        case HEATER_STATE_TOPIC :
            buff << "Heater State  = " <<  ((value != 0.0F)?"  On ":" Off "); 
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf(reinterpret_cast<char*>(&update->dispTxt), "Heater State    = %s", value?"  On":" Off");
            update->column = col1;
            update->line = stateL;
            dqueue.put(update,0,0);
            break;
        case REL_HUMIDITY_TOPIC :
            buff << "Relative Humidity  = " << setw(4) << fixed << setprecision(1) << value << "%  " ;
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf(reinterpret_cast<char*>(&update->dispTxt), "Heater State    = %s", value?"  On":" Off");
            update->column = col0;
            update->line = stateL;
            dqueue.put(update,0,0);
            break;
       default:
            buff << "Default value   = " << value << "%";
            strcpy(&update->dispTxt[0], buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "%s", buff.str().c_str());
//            sprintf((char*)&update->dispTxt, "Default %f", value);
            update->column = col0;
            update->line = stat2;
            dqueue.put(update,0,0);
            break;
    }
}

void displayThread()
{
//    cout << "\033c" << "\033[?25l" << endl;
#ifdef TARGET_CY8CKIT_062_WIFI_BT
    GUI_Init();
    GUI_Clear();
    cout << "SHT40 From Adafruit"  << endl;
    GUI_SetFont(GUI_FONT_10_1);
    GUI_SetTextAlign(GUI_TA_LEFT);
    GUI_SetFont(GUI_FONT_20B_1);
    GUI_SetColor(GUI_WHITE);
    GUI_SetBkColor(GUI_BLACK);
    GUI_SetFont(GUI_FONT_13B_1);
//    GUI_SetTextAlign(GUI_TA_CENTER);
    GUI_DispStringAt("Telemetry Data", titleL, 0);
    GUI_DispStringAt(THING_NAME, midway-col1, 0);
    GUI_DispStringAt("MQTT Broker:", midway, 0);
    GUI_SetTextAlign(GUI_TA_RIGHT);
    GUI_DispStringAt(MQTT_BROKER, titleR, 0);
//    GUI_SetTextAlign(GUI_TA_CENTER);
#endif
    dispUp = true;
    while (mqttUp == false) { 
        ThisThread::sleep_for(tenthSec);
    }
    RIS;
    thread_sleep_for(tenthSec);
    CLS;
    thread_sleep_for(tenthSec);
    HIDE_CURSOR;
    CUR_POS(1, 1);
    std::cout << "Thing: " << THING_NAME << " - MQTT Broker: " << MQTT_BROKER;
    while (true) {
        osEvent evt = dqueue.get(1);
//        printf("*");
        if (evt.status == osEventMessage) {
          auto *update = static_cast<display_t *>(evt.value.p);
          CUR_POS( update->line, update->column );
          std::cout << &(update->dispTxt[0]);
#ifdef TARGET_CY8CKIT_062_WIFI_BT
//            GUI_DispStringAt(ipAddress, 165, 22);
            GUI_DispStringAt(&update->dispTxt[0], (update->column)*xScale, ( update->line )* yScale);
#endif
            mdpool.free(update);
        }
        ThisThread::sleep_for(1);
    }
}
