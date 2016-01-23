// Example script for the EtherEventQueue library. Demonstrates advanced features.
// Periodically queues a test event, sends queued events, receives events and prints them to the serial monitor.
// Use with the EventGhost-example-trees.

//These libraries are required by EtherEventQueue:
#include <SPI.h>
#include <Ethernet.h>
#include <utility/w5100.h>  //Used for setting the W5x00 retransmission time and count.
#include <MD5.h>
#include <EtherEvent.h>
#include <EtherEventQueue.h>


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
const IPAddress sendIP = IPAddress(192, 168, 69, 100);  //The IP address to send the test events to.
const unsigned int sendPort = 1024;  //The port to send the test events to.


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
  if (EtherEventQueue.begin(maxQueueSize, maxSendEventLength, maxSendPayloadLength, maxReceivedEventLength, maxReceivedPayloadLength) == false || EtherEvent.setPassword(password) == false) {  //initialize EtherEventQueue
    Serial.print(F("ERROR: Buffer size exceeds available memory, use smaller values."));
    while (1);  //abort execution of the rest of the program
  }
  EtherEventQueue.setResendDelay(resendDelay);
  EtherEventQueue.setNodeTimeoutDuration(nodeTimeoutDuration);

  EtherEvent.setTimeout(etherEventTimeout);  //set timeout duration
  W5100.setRetransmissionTime(W5x00timeout);  //set W5x00 timeout duration
  W5100.setRetransmissionCount(W5x00retransmissionCount);  //Set W5x00 retransmission count
}


void loop() {
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
    Serial.print(F("Received from IP: "));
    Serial.println(EtherEvent.senderIP());
#endif  //ethernetclientwithremoteIP_h
  }

  if (millis() - sendTimestamp > queueEventInterval) {  //periodically send event
    sendTimestamp = millis();  //reset the timestamp for the next event send
    Serial.println(F("\nAttempting event queue"));
    if (EtherEventQueue.queue(sendIP, sendPort, EtherEventQueue.eventTypeRepeat, F("test"), F("test payload"))) {  //queue an event to be sent, EtherEventQueue will continue to attempt to send the event until it is successfully sent or the event overflows from the queue.
      Serial.println(F("Event queue successful"));
    }
    else {
      Serial.println(F("Event queue failed"));
    }
  }

  if (useDHCP == true) {
    Ethernet.maintain();  //request renewal of DHCP lease if expired
  }
}

