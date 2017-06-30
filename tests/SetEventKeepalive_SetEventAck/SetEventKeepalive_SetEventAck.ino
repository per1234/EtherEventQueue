// Used for testing EtherEventQueue.setEventKeepalive() and EtherEventQueue.setEventAck() overloads

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
  EtherEventQueue.setEventKeepalive("asdf");
  const char keepaliveConstCharArray[] = "asdf";
  EtherEventQueue.setEventKeepalive(keepaliveConstCharArray);
  const char keepaliveChar = 'a';
  EtherEventQueue.setEventKeepalive(keepaliveChar);
  const int8_t keepaliveInt8_t = -42;
  EtherEventQueue.setEventKeepalive(keepaliveInt8_t);
  const byte keepaliveByte = 42;
  EtherEventQueue.setEventKeepalive(keepaliveByte);
  const int keepaliveInt = -42;
  EtherEventQueue.setEventKeepalive(keepaliveInt);
  const unsigned int keepaliveUnsignedInt = 42;
  EtherEventQueue.setEventKeepalive(keepaliveUnsignedInt);
  const long keepaliveLong = -42;
  EtherEventQueue.setEventKeepalive(keepaliveLong);
  const unsigned long keepaliveUnsignedLong = 42;
  EtherEventQueue.setEventKeepalive(keepaliveUnsignedLong);
  const double keepaliveUnsignedDouble = 0.42;
  EtherEventQueue.setEventKeepalive(keepaliveUnsignedDouble);
  const float keepaliveFloat = 42;
  EtherEventQueue.setEventKeepalive(keepaliveFloat);
  const String keepaliveUnsignedString = "asdf";
  EtherEventQueue.setEventKeepalive(keepaliveUnsignedString);
  EtherEventQueue.setEventKeepalive(F("asdf"));

  EtherEventQueue.setEventAck("asdf");
  const char ackConstCharArray[] = "asdf";
  EtherEventQueue.setEventAck(ackConstCharArray);
  const char ackChar = 'a';
  EtherEventQueue.setEventAck(ackChar);
  const int8_t ackInt8_t = -42;
  EtherEventQueue.setEventAck(ackInt8_t);
  const byte ackByte = 42;
  EtherEventQueue.setEventAck(ackByte);
  const int ackInt = -42;
  EtherEventQueue.setEventAck(ackInt);
  const unsigned int ackUnsignedInt = 42;
  EtherEventQueue.setEventAck(ackUnsignedInt);
  const long ackLong = -42;
  EtherEventQueue.setEventAck(ackLong);
  const unsigned long ackUnsignedLong = 42;
  EtherEventQueue.setEventAck(ackUnsignedLong);
  const double ackUnsignedDouble = 0.42;
  EtherEventQueue.setEventAck(ackUnsignedDouble);
  const float ackFloat = 42;
  EtherEventQueue.setEventAck(ackFloat);
  const String ackUnsignedString = "asdf";
  EtherEventQueue.setEventAck(ackUnsignedString);
  EtherEventQueue.setEventAck(F("asdf"));
}

void loop() {}
