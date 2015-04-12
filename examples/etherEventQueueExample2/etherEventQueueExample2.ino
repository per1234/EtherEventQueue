//Example script for the EtherEventQueue library. Demonstrates advanced features.
//Periodically queues a test event, sends queued events, receives events and prints them to the serial monitor.
#include <SPI.h>  //these libraries are required by EtherEvent
#include "Ethernet.h"
#include "MD5.h"
//#include "Entropy.h"  //uncomment this line if you are using the Entropy library with EtherEvent
#include "EtherEvent.h"  //include the EtherEvent library so its functions can be accessed
#include "EtherEventQueue.h"  //include the EtherEvent library so its functions can be accessed
#include <utility/w5100.h>  //Used for setting the ethernet send connect timeout
//#include "Flash.h"  //uncomment this line if you are using the Flash library with EtherEventQueue

//configuration parameters - modify these values to your desired settings
#define DHCP false  //true==use DHCP to assign an IP address to the device, this will significantly increase memory usage. false==use static IP address.
byte MACaddress[] = {0, 1, 2, 3, 4, 4};  //this can be anything you like as long as it's unique on your network
#if DHCP == false
const IPAddress deviceIP = IPAddress(192, 168, 69, 104);  //IP address to use for the device. This can be any valid address on the network as long as it is unique. If you are using DHCP then this doesn't need to be configured.
#endif
const char password[] = "password";  //EtherEvent password. This must match the password set in EventGhost.
const unsigned int port = 1024;  //TCP port to receive events.
const byte maxQueueSize = 10;  //Maximum number of events to queue. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
const byte maxSendEventLength = 8;  //Maximum event length to send. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
const byte maxSendPayloadLength = 25;  //Maximum payload length to send. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
const byte maxReceivedEventLength = 8;  //Maximum event length to receive. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
const byte maxReceivedPayloadLength = 25;  //Maximum payload length to receive. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
const unsigned int resendDelay = 30000;  //(ms)Delay before resending repeat or confirm type queued events.
const unsigned int nodeTimeoutDuration = 240000;  //(ms)If no event has been received from a node in greater than this duration then it is considered timed out.
const byte etherEventTimeout = 20;  //(ms)The max time to wait for ethernet communication.
const unsigned int W5100timeout = 400;  //(0.1ms)used to set the timeout for the w5100 module.
const byte W5100retransmissionCount = 1;  //Retransmission count. 1 is the minimum value.

const unsigned int queueEventInterval = 4000;  //(ms)Delay between queueing the test events.
const IPAddress sendIP = IPAddress(192, 168, 69, 100);  //The IP address to send the test events to.
const unsigned int sendPort = 1024;  //The port to send the test events to.

EthernetServer ethernetServer(port);  //TCP port to receive on
EthernetClient ethernetClient;  //create the client object for ethernet communication
unsigned long sendTimeStamp;  //used by the example to periodically send an event

void setup() {
  Serial.begin(9600);  //the received event and other information will be displayed in your serial monitor while the sketch is running
#if DHCP == true
  Ethernet.begin(MACaddress);  //let the network assign an IP address
#else
  Ethernet.begin(MACaddress, deviceIP);  //use static IP address
#endif
  ethernetServer.begin();  //begin the server that will be used to receive events
  if (EtherEventQueue.begin(maxQueueSize, maxSendEventLength, maxSendPayloadLength, maxReceivedEventLength, maxReceivedPayloadLength) == false) {  //initialize EtherEventQueue
    Serial.print(F("ERROR: Buffer size exceeds available memory, use smaller values."));
    while (1);  //abort execution of the rest of the program
  }
  EtherEventQueue.setResendDelay(resendDelay);
  EtherEventQueue.setNodeTimeoutDuration(nodeTimeoutDuration);

  EtherEvent.setPassword(password);  //set the EtherEvent password
  EtherEvent.setTimeout(etherEventTimeout);  //set timeout duration
#ifdef ethernet_h
  W5100.setRetransmissionTime(W5100timeout);  //set W5100 timeout duration
  W5100.setRetransmissionCount(W5100retransmissionCount);  //Set W5100 retransmission count
#endif
}

void loop() {
  EtherEventQueue.queueHandler(ethernetClient);  //this will send events from the queue
  if (byte length = EtherEventQueue.availableEvent(ethernetServer)) {  //this checks for a new event and gets the length of the event including the null terminator
    Serial.print(F("Received event length="));
    Serial.println(length);
    char event[length];  //create the event buffer of the correct size
    EtherEventQueue.readEvent(event);  //read the event into the event buffer
    Serial.print(F("Received event: "));
    Serial.println(event);  //now the event is in your buffer
    length = EtherEventQueue.availablePayload();  //receiving the payload works the same as the event
    Serial.print(F("Received payload length="));
    Serial.println(length);
    char payload[length];
    EtherEventQueue.readPayload(payload);
    Serial.print(F("Received payload: "));
    Serial.println(payload);
#ifdef ethernetclientwithremoteIP_h  //this function is only available if the modified Ethernet library is installed
    Serial.print(F("Received from IP: "));
    Serial.println(EtherEventQueue.senderIP());  //this will return 0.0.0.0 if you don't have the modified ethernet library and the flag set in EtherEvent.cpp
#endif
  }

  if (millis() - sendTimeStamp > queueEventInterval) {  //periodically send event
    sendTimeStamp = millis();  //reset the timestamp for the next event send
    Serial.println(F("Attempting event queue"));
    if (EtherEventQueue.queue(sendIP, sendPort, F("123"), 3, F("test payload"), 12, EtherEventQueue.queueTypeRepeat)) {  //queue an event to be sent, EtherEventQueue will continue to attempt to send the event until it is successfully sent or the event overflows from the queue.
      Serial.println(F("Event queue successful"));
    }
    else {
      Serial.println(F("Event queue failed"));
    }
  }
}
