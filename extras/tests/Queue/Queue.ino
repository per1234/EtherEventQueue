// Used for testing EtherEventQueue.queue() overloading

#define ETHEREVENT_NO_AUTHENTICATION
#define ETHEREVENT_FAST_SEND

#include <SPI.h>
#include <Ethernet.h>
#ifndef ESP8266
#include <MD5.h>
#endif  //ESP8266
#include <EtherEvent.h>
#include <EtherEventQueue.h>

void setup() {
  EtherEventQueue.begin();
  runTests();
}

void loop() {}
