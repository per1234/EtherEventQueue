//example script for the EtherEventQueue library
#include <SPI.h>  //these libraries are required by EtherEvent
#include "Ethernet.h"
#include "MD5.h"
//#include <Entropy.h>  //uncomment this line if you have the Entropy library installed
#include "EtherEvent.h"  //include the EtherEvent library so its functions can be accessed
#include "EtherEventQueue.h"  //include the EtherEventQueue library so its functions can be accessed
//#include "Flash.h"  //uncomment this line if you have the Flash library installed

EthernetServer ethernetServer(1024);  //TCP port to receive on
EthernetClient ethernetClient;  //create the client object for ethernet communication

unsigned long sendTimeStamp; //used by the example to periodically send an event

void setup() {
  Serial.begin(9600);  //the received event and other information will be displayed in your serial monitor while the sketch is running
  byte mac[] = {0, 1, 2, 3, 4, 4}; //this can be anything you like, but must be unique on your network
  Ethernet.begin(mac, IPAddress(192, 168, 69, 104));  //leave off the IP parameter for DHCP
  ethernetServer.begin();  //begin the server that will be used to receive events
  if (EtherEventQueue.begin("password", 4, 1024) == false) { //set the EtherEvent password, node ID, and the EtherEvent TCP port
    Serial.print(F("ERROR: Buffer size exceeds available memory, use smaller values."));
    while (1);  //abort execution of the rest of the program
  }
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
  }

  if (millis() - sendTimeStamp > 4000) { //periodically send event
    sendTimeStamp = millis(); //reset the timestamp for the next event send
    Serial.println(F("Attempting event queue"));
    if (EtherEventQueue.queue(IPAddress(192, 168, 69, 100), 1024, "123", "test payload", 2)) { //queue an event to be sent to target IP address, port, event, payload, resendFlag
      Serial.println(F("Event queue successful"));
    }
    else {
      Serial.println(F("Event queue failed"));
    }
  }
}
