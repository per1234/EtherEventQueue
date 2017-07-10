#include <SPI.h>
#include <Ethernet.h>
#if !defined(ETHEREVENT_NO_AUTHENTICATION) && !defined(ESP8266)
#include <MD5.h>
#endif  //!defined(ETHEREVENT_NO_AUTHENTICATION) && !defined(ESP8266)
#include <EtherEvent.h>
#include <EtherEventQueue.h>

EthernetServer ethernetServer(1024);  //TCP port to receive on
EthernetClient ethernetClient;  //create the client object for Ethernet communication


void setup() {
  byte MACaddress[] = {0, 1, 2, 3, 4, 4};  //this can be anything you like as long as it's unique on your network
  Ethernet.begin(MACaddress, IPAddress(192, 168, 69, 104));  //use static IP address
  ethernetServer.begin();  //begin the server that will be used to receive events

  EtherEventQueue.begin();
  EtherEventQueue.availableEvent(ethernetServer);
  EtherEventQueue.queueHandler(ethernetClient);
}

void loop() {}
