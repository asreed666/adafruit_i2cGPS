/* WiFi Example
 * Copyright (c) 2016 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MQTTThread.h"
#include "MQTTClientMbedOs.h"
#include "actuators.h"
#include "display.h"

//#include "main.h"
#include "mbed.h"

//#include "ntp-client/NTPClient.h"

//#include "sensors.h"

#include "constants.h"

#include <cstdint>

#include <cstring>

#include <cstdlib>//.h>
#include <iostream>

//using namespace std;

using pubPacket_t = struct {
  int topic;
  float value;
} __attribute__((aligned(align8)));
static auto qSize = 0;
static pubPacket_t myQueue[qLen];
static uint16_t stQueue = 0;
static uint16_t endQueue = 0;
static MemoryPool<pubPacket_t, 32> mpool1;
static Queue<pubPacket_t, 32> pqueue;
WiFiInterface *wifi;
//time_t currentTime = 1637530216;
uint32_t rxCount = 0;
uint32_t txCount = 0;
uint32_t lthresh = 50;
char ipAddress[17];
bool mqttUp = false;
//DigitalOut rxLed(LED1);
//DigitalOut errorLED(LED2);

auto sec2str(nsapi_security_t sec) -> const char * {
  switch (sec) {
  case NSAPI_SECURITY_NONE:
    return "None";
  case NSAPI_SECURITY_WEP:
    return "WEP";
  case NSAPI_SECURITY_WPA:
    return "WPA";
  case NSAPI_SECURITY_WPA2:
    return "WPA2";
  case NSAPI_SECURITY_WPA_WPA2:
    return "WPA/WPA2";
  case NSAPI_SECURITY_UNKNOWN:
  default:
    return "Unknown";
  }
}

auto scan_demo(WiFiInterface *wifi) -> int {
  WiFiAccessPoint *ap;

  printf("Scan:\n");

  int count = wifi->scan(nullptr, 0);

  if (count <= 0) {
    std::cout << "scan() failed with return value:" << count << endl;
    return 0;
  }

  /* Limit number of network arbitrary to 15 */
  count = count < 15 ? count : 15;

  ap = new WiFiAccessPoint[count];
  count = wifi->scan(ap, count);

  if (count <= 0) {
    std::cout << "scan() failed with return value: " << count << endl;
    return 0;
  }

  for (int i = 0; i < count; i++) {
    printf("Network: %s secured: %s BSSID: %hhX:%hhX:%hhX:%hhx:%hhx:%hhx RSSI: "
           "%hhd Ch: %hhd\n",
           ap[i].get_ssid(), sec2str(ap[i].get_security()),
           ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
           ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5],
           ap[i].get_rssi(), ap[i].get_channel());
  }
  printf("%d networks available.\n", count);

  delete[] ap;
  return count;
}
void LThreshArrived(MQTT::MessageData &md) {
  MQTT::Message &message = md.message;
  uint32_t len = md.message.payloadlen;
  char rxed[len + 1];

  strncpy(&rxed[0], static_cast<char *>((&md.message.payload)[0]), len);
  thingData.lightThreshVal = static_cast<float>(atof(&rxed[0]));
  thingData.rxCountVal += 1;
}

void sendPub(int pTopic, float pValue) {
  //    pubPacket_t *mpacket = mpool1.alloc();
  //    if (mpacket) {
  //        mpacket->topic = pTopic;
  //        mpacket->value = pValue;
  //        pqueue.put(mpacket);
  //    }
  //    pqueue.put(mpacket);
  if (qSize == qLen) {
    std::cout << "Publish queue is full!\n";
  } else {
    myQueue[stQueue].topic = pTopic;
    myQueue[(stQueue++) % qLen].value = pValue;
    qSize++;
    if (stQueue >= qLen)
      stQueue = 0;
  }
}
void mqttThread() {
  printf("Starting MQTT Reporting to %S\n", (wchar_t *)MQTT_BROKER);
  char buffer[80];
  char topicBuffer[80];
  uint32_t rc;
  uint32_t failure = 0;
  int lastRxCount = 0;
  int lastTxCount = 0;
  const char topicMap[NUM_TOPICS][TOPIC_LEN] = {
      "light",   "lightState", "lightSwitch", "redled", "greenled",
      "blueled", "announce",   "lthresh",     "local",  "manual",
      "temperature", "tempthresh", "rxCount", "txCount", "time", "statusled",
      "orangeled", "heaterState", "heaterSwitch"};

#ifdef MBED_MAJOR_VERSION
  printf("Mbed OS version %d.%d.%d\n\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION,
         MBED_PATCH_VERSION);
#endif

  wifi = WiFiInterface::get_default_instance();
  if (!wifi) {
    printf("ERROR: No WiFiInterface found.\n");
    while (1)
      ; // hang
  }

  int count = scan_demo(wifi);
  if (count == 0) {
    printf("No WIFI APs found - can't continue further.\n");
    while (1)
      ; // hang
  }

  printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
  int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD,
                          NSAPI_SECURITY_WPA_WPA2);
  if (ret != 0) {
    printf("\nConnection error: %d\n", ret);
    while (1)
      ; // hang
  }

  std::cout << "Success\n\n";
  std::cout << "MAC: " << wifi->get_mac_address() << endl;
  std::cout << "IP: " << wifi->get_ip_address() << endl;
  sprintf(ipAddress, "%s",wifi->get_ip_address());
  std::cout << "Netmask: " << wifi->get_netmask() << endl;
  std::cout << "Gateway: " << wifi->get_gateway() << endl;
  std::cout << "RSSI: " << wifi->get_rssi() << endl << endl;
  //NTPClient ntpclient(wifi); // connect NTP Client
                             //    set_time(currentTime);
  //time_t timestamp = ntpclient.get_timestamp(oneSec);
  //if (timestamp > 0) {   // timesatmp is valid if not less than 0
  //  set_time(timestamp); // set local time to current network time
  //  currentTime = time(NULL);
  //  strftime(buffer, 32, "%d/%m/%Y %H:%M:%S", localtime(&currentTime));
  //  printf("\n%s\r\n", buffer);
  //}
  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.clientID.cstring = (char *)THING_NAME;
  data.keepAliveInterval = 20;
  data.cleansession = 1;
  data.username.cstring = (char *)"";
  data.password.cstring = (char *)"";
  char *host = (char *)MQTT_BROKER;
  uint16_t port = 1883;
  TCPSocket socket;
  MQTTClient client(&socket);
  socket.open(wifi);
  rc = socket.connect(host, port);
  if (rc == 0) {
    printf("Succesful connection of socket to Host %s port %d\n", host, port);
  } else {
    printf("Socket connection failed");
    while (socket.connect(host, port)) {
      printf(".");
    }
  }
  rc = client.connect(data);
  if (rc == 0) {
    printf("Succesful connection of %s to Broker\n", data.clientID.cstring);
  } else {
    printf("Client connection failed");
  }
  MQTT::Message message{};
  sprintf(buffer, "Hello World! from %s\r\n", THING_NAME);
  message.qos = MQTT::QOS0;
  message.retained = false;
  message.dup = false;
  message.payload = (void *)buffer;
  message.payloadlen = strlen(buffer) + 1;

  rc = client.publish(topicMap[ANNOUNCE_TOPIC], message);
  if (rc == 0)
    printf("publish announce worked\n");
  else {
    printf("publish announce failed %d\n", rc);
  }
//  sprintf(topicBuffer, "%s/%S", THING_NAME, topicMap[LIGHT_THRESH_TOPIC]); // Does not Work????
  rc = client.subscribe( SUBSCRIBE_TOPIC, MQTT::QOS0, LThreshArrived); //Does not work????
  //rc = client.subscribe("CCP03/lthresh", MQTT::QOS0, LThreshArrived);
  if (rc != 0)
    printf("Subscription Error %d\n", rc);
  else {
    printf("Subscribed to %s len = %d\n", SUBSCRIBE_TOPIC, strlen(SUBSCRIBE_TOPIC));
//    printf("Subscribed to %s len = %d\n", "ThisNewThing/lthresh", strlen("ThisNewThing/lthresh"));
  }
  mqttUp = true;
  sendPub(LIGHT_THRESH_TOPIC, thingData.lightThreshVal);
  static int j = 0;
//  osEvent evt;
    bool consoleUpdate = true;
  while (true) {
    if (thingData.rxCountVal > lastRxCount) {
      //printf("\033[4;25HRx Count = %d\033[4;50HSet Threshold = %d\r\n", rxCount, lthresh);
//      displayUpdate(RX_COUNT, thingData.rxCountVal);
//      displayUpdate(LIGHT_THRESH_TOPIC, thingData.lightThreshVal);
      lastRxCount = thingData.rxCountVal;
      thingData.statusLedState = !thingData.statusLedState;
    }
    if (thingData.txCountVal > lastTxCount) {
      //printf("\033[5;25HTx Count = %d\r\n", txCount);
//      displayUpdate(TX_COUNT, static_cast<float>(thingData.txCountVal));
      lastTxCount = thingData.txCountVal;
      thingData.orangeLedState = !thingData.orangeLedState;
    }
    ThisThread::sleep_for(10);
//    currentTime = time(NULL);
//    displayUpdate(TIME, 0.0f);

    //    evt = pqueue.get(1);
    //    if (evt.status == osEventMessage) {
    //        printf(".");
    //        pubPacket_t *pubPacket = (pubPacket_t *)evt.value.p;
    //        sprintf(buffer, "%f", pubPacket->value);
    //        sprintf(topicBuffer, "%s/%s", THING_NAME,
    //        topicMap[pubPacket->topic]  ); message.payload = (void *)buffer;
    //        message.payloadlen = strlen(buffer) + 1;
    //
    //        client.publish(topicBuffer, message);
    //        if(mpool1.free(pubPacket) == osOK) printf("Free Mem OK");
    //        txCount++;
    //    }
    
    if (qSize > 0) {
      sprintf(buffer, "%f", myQueue[endQueue].value);
      sprintf(topicBuffer, "%s/%s", THING_NAME,
              topicMap[myQueue[endQueue++].topic]);
      qSize--;
      if (endQueue >= qLen){
          endQueue = 0;
      }
        
      message.payload = (void *)buffer;
      message.payloadlen = strlen(&buffer[0]) + 1;

      rc=client.publish(&topicBuffer[0], message);
      if (rc != osOK) {
        thingData.orangeLedState = !thingData.orangeLedState;

          cout << "Client is " << (client.isConnected()?"connected":"disconnected");
         // publish failed try to reconnect
          /*
          socket.close(); 
          wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD,
                          NSAPI_SECURITY_WPA_WPA2);
          socket.open(wifi);
          socket.connect(host, port);
          client.connect(data);
          sprintf(buffer, "Hello again from %s\r\n", THING_NAME);
          message.payload = (void *)buffer;
          message.payloadlen = strlen(buffer) + 1;
          client.publish(topicMap[ANNOUNCE_TOPIC], message);
          */
      }
      (thingData.txCountVal)++;
    }

    if (consoleUpdate) {
/*        displayUpdate(REDLED_TOPIC, static_cast<float>(thingData.redLedState));
        ThisThread::sleep_for(1);
        displayUpdate(BLUELED_TOPIC, static_cast<float>(thingData.blueLedState));
        ThisThread::sleep_for(1); 
        displayUpdate(GREENLED_TOPIC, static_cast<float>(thingData.greenLedState));
        ThisThread::sleep_for(1); 
        displayUpdate(MANUAL_TOPIC, static_cast<float>(thingData.manualModeState));
        ThisThread::sleep_for(1); 
        displayUpdate(LIGHT_SWITCH_TOPIC, static_cast<float>(thingData.lightStwitchState));
        ThisThread::sleep_for(1); 
        displayUpdate(LOCAL_TOPIC,static_cast<float>(thingData.localModeState));
        ThisThread::sleep_for(1); 
        displayUpdate(LIGHT_STATE_TOPIC, static_cast<float>(thingData.lightState));
        ThisThread::sleep_for(1); 
        displayUpdate(ORANGELED_TOPIC, static_cast<float>(thingData.orangeLedState));
        ThisThread::sleep_for(2); 
        displayUpdate(TEMP_THRESH_TOPIC, thingData.tempThreshVal);
        ThisThread::sleep_for(2); 
        displayUpdate(HEATER_STATE_TOPIC, static_cast<float>(thingData.heaterState));
        ThisThread::sleep_for(2); 
        displayUpdate(HEATER_SWITCH_TOPIC, static_cast<float>(thingData.heaterSwitchState));
*/
        consoleUpdate = false;
  }
/*
    if (thingData.txCountVal==0) displayUpdate(REDLED_TOPIC, thingData.redLedState);
    if (thingData.txCountVal==0) displayUpdate(BLUELED_TOPIC, thingData.blueLedState);
    if (thingData.txCountVal==0) displayUpdate(GREENLED_TOPIC, thingData.greenLedState);
    if (thingData.txCountVal==1) displayUpdate(MANUAL_TOPIC, thingData.manualModeState);
    if (thingData.txCountVal==1) displayUpdate(LIGHT_SWITCH_TOPIC, thingData.lightStwitchState);
    if (thingData.txCountVal==1) displayUpdate(LOCAL_TOPIC,thingData.localModeState);
    if (thingData.txCountVal==1) displayUpdate(LIGHT_STATE_TOPIC, thingData.lightState);
    if (thingData.txCountVal==2) displayUpdate(ORANGELED_TOPIC, thingData.orangeLedState);
    if (thingData.txCountVal==2) displayUpdate(TEMP_THRESH_TOPIC, thingData.tempThreshVal);
    if (thingData.txCountVal==2) displayUpdate(HEATER_STATE_TOPIC, thingData.heaterState);
    if (thingData.txCountVal==2) displayUpdate(HEATER_SWITCH_TOPIC, thingData.heaterSwitchState);
*/
      //if (((j++) % 10) == 0)
      //sendPub(LIGHT_LEVEL_TOPIC, j * 1.1f);
    ThisThread::sleep_for(3*tenthSec);
    rc = client.yield(hundredthSec);
    if (rc != 0U) {
        std::cout << "\033[18;0HClient Disconnected"  << endl;
    }
  }

  wifi->disconnect();

  std::cout << R"(
                Done
                )";
}
