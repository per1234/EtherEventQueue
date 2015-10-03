// Example script for the EtherEventQueue library. Demonstrates use of the nodes.
// Periodically queues a keepalive event, sends queued events, receives events and prints them to the serial monitor.
// The keepalive is an event that is used only to keep nodes from timing out. It is handled internally to update the node timestamp and will not be passed on by EtherEventQueue.
// Use with the EventGhost-example-trees.

//These libraries are required by EtherEventQueue:
#include <SPI.h>
#include <Ethernet.h>
#include <utility/w5100.h>  //Used for setting the W5x00 retransmission time and count.
#include "MD5.h"
#include "EtherEvent.h"
#include "EtherEventQueue.h"


//configuration parameters - modify these values to your desired settings
const boolean useDHCP = false;  //true==use DHCP to assign an IP address to the device, this will significantly increase memory usage. false==use static IP address.
byte MACaddress[] = {0, 1, 2, 3, 4, 4};  //this can be anything you like as long as it's unique on your network
const IPAddress deviceIP = IPAddress(192, 168, 69, 104);  //IP address to use for the device. This can be any valid address on the network as long as it is unique. If you are using DHCP then this doesn't need to be configured.
const char password[] = "password";  //EtherEvent password. This must match the password set in EventGhost.
const unsigned int port = 1024;  //TCP port to receive events.

const byte maxQueueSize = 10;  //Maximum number of events to queue. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
const byte maxSendEventLength = 8;  //Maximum event length to send. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
const byte maxSendPayloadLength = 25;  //Maximum payload length to send. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
const byte maxReceivedEventLength = 8;  //Maximum event length to receive. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
const byte maxReceivedPayloadLength = 25;  //Maximum payload length to receive. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.

const unsigned int resendDelay = 30000;  //(ms)Delay before resending repeat or confirm type queued events.
const unsigned long nodeTimeoutDuration = 240000;  //(ms)If no event has been received from a node in greater than this duration then it is considered timed out.

const byte etherEventTimeout = 20;  //(ms)The max time to wait for Ethernet communication.
const unsigned int W5x00timeout = 400;  //(0.1ms)used to set the timeout for the W5x00 module.
const byte W5x00retransmissionCount = 1;  //Retransmission count. 1 is the minimum value.

const unsigned int queueEventInterval = 4000;  //(ms)Delay between queueing the test events.
const byte numberOfNodes = 2;
const byte deviceNode = 0;
const byte targetNode = 1;
const IPAddress targetNodeIP = IPAddress(192, 168, 69, 100);  //The IP address to set as node 1, this is where the keepalive event will be sent.
const unsigned int sendPort = 1024;  //The port to send the test events to.

const char eventKeepalive[] = "yo";
const unsigned int keepaliveMargin = 5000;
const unsigned int keepaliveResendDelay = 15000;


EthernetServer ethernetServer(port);  //TCP port to receive on
EthernetClient ethernetClient;  //create the client object for Ethernet communication
unsigned long sendTimestamp;  //used by the example to periodically send an event


void setup() {
  Serial.begin(9600);  //the received event and other information will be displayed in your serial monitor while the sketch is running
  if (useDHCP == true) {
    Ethernet.begin(MACaddress);  //let the network assign an IP address
  }
  else {
    Ethernet.begin(MACaddress, deviceIP);  //use static IP address
  }
  ethernetServer.begin();  //begin the server that will be used to receive events
  if (EtherEventQueue.begin(deviceNode, numberOfNodes, maxQueueSize, maxSendEventLength, maxSendPayloadLength, maxReceivedEventLength, maxReceivedPayloadLength) == false || EtherEvent.setPassword(password) == false) {  //initialize EtherEventQueue
    Serial.print(F("ERROR: Buffer size exceeds available memory, use smaller values."));
    while (1);  //abort execution of the rest of the program
  }
  EtherEventQueue.setResendDelay(resendDelay);
  EtherEventQueue.setNodeTimeoutDuration(nodeTimeoutDuration);
  EtherEventQueue.setEventKeepalive(F("yo"));

  EtherEventQueue.setNode(targetNode, targetNodeIP);  //create node 1


  EtherEvent.setTimeout(etherEventTimeout);  //set timeout duration
  W5100.setRetransmissionTime(W5x00timeout);  //set W5x00 timeout duration
  W5100.setRetransmissionCount(W5x00retransmissionCount);  //Set W5x00 retransmission count
}


void loop() {
  EtherEventQueue.sendKeepalive(sendPort);  //send the keepalive event when it is time to do so

  if (EtherEventQueue.queueHandler(ethernetClient) == false) {  //this will send events from the queue
    Serial.println(F("\nEvent send failed"));
  }

  if (byte length = EtherEventQueue.availableEvent(ethernetServer)) {  //this checks for a new event and gets the length of the event including the null terminator
    Serial.print(F("\nReceived event length="));
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
    IPAddress receivedIP = EtherEvent.senderIP();
    Serial.print(F("Received from IP address: "));
    Serial.println(receivedIP);
    int8_t receivedNode = EtherEventQueue.getNode(receivedIP);
    if (receivedNode != -1) {  //-1 indicates no matching node
      Serial.print(F("Received from node: "));
      Serial.println(receivedNode);
    }
    else {
      Serial.println(F("IP address is not a node"));
    }
#endif  //ethernetclientwithremoteIP_h
  }

  if (millis() - sendTimestamp > queueEventInterval) {  //periodically send event
    Serial.println(F("\nAttempting event queue"));

    Serial.print(F("Target node: "));
    Serial.println(targetNode);
    Serial.print(F("Target IP address: "));
    Serial.println(EtherEventQueue.getIP(targetNode));
    Serial.print(F("Target node state: "));

    int8_t nodeState = EtherEventQueue.checkState(targetNode);
    if (nodeState == true) {
      Serial.println(F("Active"));
    }
    if (nodeState == false) {
      Serial.println(F("Timed Out"));
    }

    if (EtherEventQueue.queue(targetNode, sendPort, EtherEventQueue.eventTypeRepeat, F("test"), F("test payload"))) {  //queue an event to be sent, EtherEventQueue will continue to attempt to send the event until it is successfully sent or the event overflows from the queue.
      Serial.println(F("Event queue successful"));
    }
    else {
      Serial.println(F("Event queue failed"));
    }
    sendTimestamp = millis();  //reset the timestamp for the next event send
  }

  int8_t timedOutNode = EtherEventQueue.checkTimeout();
  if (timedOutNode != -1) {  //-1 indicates no nodes are newly timed out
    Serial.print(F("\nNewly timed out node: "));
    Serial.println(timedOutNode);
  }

  int8_t timedInNode = EtherEventQueue.checkTimein();
  if (timedInNode != -1) {  //-1 indicates no nodes are newly timed in
    Serial.print(F("\nNewly timed in node: "));
    Serial.println(timedInNode);
  }

  if (useDHCP == true) {
    Ethernet.maintain();  //request renewal of DHCP lease if expired
  }
}

