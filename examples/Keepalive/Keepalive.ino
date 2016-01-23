// Example script for the EtherEventQueue library. Demonstrates use of the sendKeepalive function.
// Queues and attempts to send a keepalive event every 20 seconds to node 1.
// The keepalive is an event that is used only to keep nodes from timing out. It is handled internally to update the node timestamp and will not be passed on by EtherEventQueue.
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

const byte etherEventTimeout = 20;  //(ms)The max time to wait for Ethernet communication.
const unsigned int W5x00timeout = 400;  //(0.1ms)used to set the timeout for the W5x00 module.
const byte W5x00retransmissionCount = 1;  //Retransmission count. 1 is the minimum value.

const byte numberOfNodes = 2;
const byte deviceNode = 0;
const byte targetNode = 1;
const IPAddress targetNodeIP = IPAddress(192, 168, 69, 100);  //The IP address to set as node 1, this is where the keepalive event will be sent.
const unsigned int nodeTimeoutDuration = 25000;  //(ms)If no event has been received from a node in greater than this duration then it is considered timed out.
const unsigned int sendPort = 1024;

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
  if (EtherEventQueue.begin(deviceNode, numberOfNodes) == false || EtherEvent.setPassword(password) == false) {  //initialize EtherEventQueue
    Serial.print(F("ERROR: Buffer size exceeds available memory, use smaller values."));
    while (1);  //abort execution of the rest of the program
  }

  EtherEventQueue.setNode(targetNode, targetNodeIP);  //create node 1
  EtherEventQueue.setNodeTimeoutDuration(nodeTimeoutDuration);

  EtherEventQueue.setEventKeepalive(eventKeepalive);
  EtherEventQueue.setSendKeepaliveMargin(keepaliveMargin);
  EtherEventQueue.setSendKeepaliveResendDelay(keepaliveResendDelay);

  EtherEvent.setTimeout(etherEventTimeout);  //set timeout duration
  W5100.setRetransmissionTime(W5x00timeout);  //set W5x00 timeout duration
  W5100.setRetransmissionCount(W5x00retransmissionCount);  //Set W5x00 retransmission count
}


void loop() {
  EtherEventQueue.sendKeepalive(sendPort);  //Queue the keepalive event when it is time to do so.

  if (EtherEventQueue.queueHandler(ethernetClient) == false) {  //This will send events from the queue.
    Serial.println(F("Event send failed"));
  }

  if (useDHCP == true) {
    Ethernet.maintain();  //request renewal of DHCP lease if expired
  }
}

