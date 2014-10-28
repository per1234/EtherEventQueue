EtherEventQueue
==========

Outgoing event queue for the EtherEvent Arduino library.
EtherEvent provides easy to use password authenticated network communication between Arduinos and other devices running EventGhost, Girder, or any other program compatible with the EventGhost Network Event Sender and Receiver plugins.

This is an alpha release. It is not thoroughly tested. Feel free to make pull requests or issue reports. Thanks!

#### Required Libraries
- EtherEvent http://github.com/per1234/EtherEvent
- Ethernet library modification instructions: http://forum.arduino.cc/index.php?/topic,82416.0.html

#### Related Programs
- UIPEthernet library for ENC28J60 ethernet chip: http://github.com/ntruchsess/arduino_uip
- EventGhost free open source automation tool for Windows http://eventghost.com
- TCP Events EventGhost plugin: http://www.eventghost.org/forum/viewtopic.php?p=16803 - Improved network event sender/receiver allows sending events to multiple IP addresses

#### Installation
- 64k is the minimum recommended flash memory capacity of the MCU
- Download the most recent version of EtherEventQueue here http://github.com/per1234/EtherEventQueue  - Download ZIP button(or Clone in Desktop if you have GitHub Desktop installed)
- Extract the EtherEventQueue-master folder from the downloaded zip file
- Rename the folder EtherEventQueue
- Move the folder to your arduino sketchbook\libraries folder
- Repeat this process with the other required libraries
- Modify the stock Arduino Ethernet library following these instructions: http://forum.arduino.cc/index.php?/topic,82416.0.html
- uncomment #define SENDERIP_ENABLE in EtherEvent.cpp
- EtherEventQueue library configuration parameters(EtherEventQueue.cpp):
  - there are several paramerters that can be configured in the library, they are documented there
- Restart the Arduino IDE
- File>Examples>etherEventQueueExample
 - Set the device IP address, this can be any available IP address on the network. DHCP not currently implemented.
 - Set the device MAC address. This can be any address not already used on the network
 - Set the EtherEvent authentication password.
 - Set the EtherEvent TCP port.
- Upload example sketch to device
- Repeat with other connected devices. The serial monitor will show details of the test communications.

#### Usage
`EtherEventQueue.begin(deviceID, port)` - Initialize EtherEventQueue
- Parameter: deviceID - the node number of the device. The device IP address must be in the node array in EtherEventQueue.cpp
  - Type: byte
- Parameter: port - the port being used for events. This is used only for sending acks, queue() allows sending to any port, the device Ethernet port is configured with the EthernetServer initialization
  - Type: unsigned int
- Returns: none

`EtherEventQueue.availableEvent(ethernetServer)` - Returns the number of chars of event including null terminator available to read.
- Parameter: ethernetServer - the EthernetServer object created in the Ethernet setup of the user's sketch
  - Type: EthernetServer
- Returns: Number of chars in the event including the null terminator at the end of the string.
  - Type: byte

`EtherEventQueue.availablePayload()` - Returns the number of chars of payload including null terminator available to read. availableEvent() must be called first.
- Parameter: none
- Returns: Number of chars in the payload including the null terminator at the end of the string.
  - Type: byte

`EtherEventQueue.readEvent(char eventBuffer[])` - Puts the event in the passed array. availableEvent() must be called first.
- Parameter: eventBuffer - size a char array according to the result of availableEvent () and pass it to the readEvent  function. After that it will contain the event.
  - Type: char
- Returns: none

`EtherEventQueue.readPayload(char payloadBuffer[])` - Puts the payload string in the passed array. availableEvent() must be called first.
- Parameter: payloadBuffer - size a char array according to the result of availablePayload () and pass it to the readPayload  function. After that it will contain the payload.
  - Type: char
- Returns: none

`EtherEventQueue.senderIP()` - get the IP Address of the sender of the most recent event. availableEvent() must be called first.
- Parameter: none
- Returns: IP Address of the sender of the most recent event
  - Type: IPAddress

`EtherEventQueue.flushReceiver()` - clear any buffered event and payload data so a new event can be received
- Parameter:none
- Returns:none

`EtherEventQueue.queue(target, port, event, payload, resendFlag)` - Send an event and payload
- Parameter: target - takes either the IP address or node number of the target device
  - Type: IPAddress
- Parameter: port: - port to send the event to
  - Type: unsigned int
- Parameter: event: - string to send as the event(char array).
  - Type: const char
- Parameter: payload:- payload to send with the event(char array). If you don't want a payload then just use "" for this parameter
  - Type: const char
- Parameter: resendFlag - (0 == no resend, 1 == resend until successful send, 2 == resend until ack) If this is set to 2 then the queue will resend a message until the ack is received or the target IP times out
  - Type: boolean
- Returns: 0 for failure, 1 for success, , 2 for success w/ queue overflow
  - Type: byte
  
`EtherEventQueue.queueHandler(ethernetClient)` - send queued events
- Parameter: ethernetClient - the EthernetClient object created during the Ethernet library initialization
  - Type: EthernetClient
  - Returns: none
   
 `EtherEventQueue.flushQueue()` - remove all events from the queue
 - Returns: none

`EtherEventQueue.checkTimeout()` - check for newly timed out nodes
- Parameter: none
-Returns: node number of the tirst newly timed out node found or -1 if no timed out node found
  - Type: int
  
`EtherEventQueue.checkTimein()` - check for newly timed in nodes
- Parameter: none
-Returns: node number of the tirst newly timed out node found or -1 if no timed out node found
  - Type: int
  
`EtherEventQueue.checkState(node)` - check if the device has not received any events in longer than the timeout duration
- Parameter: node - the node number of the node to be checked
-Returns: 0 == not timed out, 1 == timed out
  - Type: boolean
  
 
 #### Process
- queue event
 - non-ping events addressed to timed out nodes(other than self) are not queued
 - events addressed to non-nodes are always queued unless non-node sending is disabled
- send event from queue
 - newly queued events(haven't been sent yet) are sent first FIFO
 - if there are no newly queued events then the sent events in the queue that have not been acked yet are resent FIFO
 - events other than pings to timed out nodes are discarded
 - event sent
 - remove events from the queue that have the resendFlag parameter == 0
 - incoming events
  - check the queue for internal events(addressed to self)
  - check for external events(from network)
    - if the event is from a node then the timestamp is updated
    - if incoming event is an ack then remove the message the ack refers to from the queue