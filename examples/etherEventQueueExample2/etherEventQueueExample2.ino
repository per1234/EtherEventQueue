//Example script for the EtherEventQueue library. Demonstrates advanced features
#include <SPI.h>  //these libraries are required by EtherEvent
#include <Ethernet.h>
#include "MD5.h"
//#include "Entropy.h"  //uncomment this line if you have the Entropy library installed
#include "EtherEvent.h"  //include the EtherEvent library so its functions can be accessed
#include "EtherEventQueue.h"  //include the EtherEvent library so its functions can be accessed
#include <utility/w5100.h>  //Used for setting the ethernet send connect timeout


const unsigned int port = 1024; //EtherEvent TCP port

EthernetServer ethernetServer(port);  //TCP port to receive on
EthernetClient ethernetClient;  //create the client object for ethernet communication

unsigned long sendTimeStamp = 0; //used by the example to periodically send an event

void setup() {
  Serial.begin(9600);  //the received event and other information will be displayed in your serial monitor while the sketch is running
  byte mac[] = {0, 1, 2, 3, 4, 4}; //this can be anything you like, but must be unique on your network
  Ethernet.begin(mac, IPAddress(192, 168, 69, 104));  //leave off the IP parameter for DHCP
  ethernetServer.begin();  //begin the server that will be used to receive events
  EtherEvent.begin("password");  //initialize EtherEvent and set the authentication password
  EtherEvent.setTimeout(500, 1000); //(200, 400)set timeout values
  EtherEventQueue.begin(4, port);  //set the node ID and the EtherEvent TCP port
#ifdef ethernet_h
  W5100.setRetransmissionTime(0x07D0);  //(0xFA)used to set the timeout for the w5100 module this will not work if you are using ENC28J60 instead of W5100
  W5100.setRetransmissionCount(1);  //Retransmission Count 1 is the minimum value
#endif
}

void loop() {
  EtherEventQueue.queueHandler(ethernetClient);  //this will send events from the queue
  if (byte length = EtherEventQueue.availableEvent(ethernetServer)) { //this checks for a new event and gets the length of the event including the null terminator
    Serial.print(F("Received event length="));
    Serial.println(length);
    char event[length];  //create the event buffer of the correct size
    EtherEventQueue.readEvent(event);  //read the event into the event buffer
    Serial.print(F("Received event: "));
    Serial.println(event);  //now the event is in your buffer
    length = EtherEventQueue.availablePayload(); //receiving the payload works the same as the event
    Serial.print(F("Received payload length="));
    Serial.println(length);
    char payload[length];
    EtherEventQueue.readPayload(payload);
    Serial.print(F("Received payload: "));
    Serial.println(payload);
    Serial.print(F("Received from IP: "));
    Serial.println(EtherEventQueue.senderIP());  //this will return 0.0.0.0 if you don't have the modified ethernet library and the flag set in EtherEvent.cpp
  }

  if (millis() - sendTimeStamp > 4000) { //periodically send event
    sendTimeStamp = millis(); //reset the timestamp for the next event send
    Serial.println(F("Attempting event send"));
    if (EtherEventQueue.queue(IPAddress(192, 168, 69, 100), port, "123", "test payload", 2)) { //queue an event to be sent to target IP address, port, event, payload, resendFlag
      Serial.println(F("Event send successful"));
    }
    else {
      Serial.println(F("Event send failed"));
    }
  }
}
