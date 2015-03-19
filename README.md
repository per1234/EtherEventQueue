EtherEventQueue
==========

Outgoing event queue for the EtherEvent Arduino library.
EtherEvent provides easy to use password authenticated network communication between Arduinos and other devices running EventGhost, Girder, or any other program compatible with the EventGhost Network Event Sender and Receiver plugins.

This is an alpha release. It is not thoroughly tested. Feel free to make pull requests or issue reports. Thanks!

#### Required Libraries
- EtherEvent http://github.com/per1234/EtherEvent
- Ethernet library modification instructions: http://forum.arduino.cc/index.php?topic=82416.0

#### Related Programs
- UIPEthernet library for ENC28J60 ethernet chip: http://github.com/ntruchsess/arduino_uip
- EventGhost free open source automation tool for Windows http://eventghost.com
- TCP Events EventGhost plugin: http://www.eventghost.org/forum/viewtopic.php?p=16803 - Improved network event sender/receiver allows sending events to multiple IP addresses
- Flash library to allow passing payload strings stored in flash memory without a string length argument: http://github.com/rkhamilton/Flash 

#### Installation
- 64k is the minimum recommended flash memory capacity of the MCU
- Download the most recent version of EtherEventQueue here http://github.com/per1234/EtherEventQueue  - Download ZIP button(or Clone in Desktop if you have GitHub Desktop installed)
- Extract the EtherEventQueue-master folder from the downloaded zip file
- Rename the folder EtherEventQueue
- Move the folder to your arduino sketchbook\libraries folder
- Repeat this process with the other required libraries
- Modify the stock Arduino Ethernet library following these instructions: http://forum.arduino.cc/index.php?topic=82416.0
- uncomment #define SENDERIP_ENABLE in EtherEvent.cpp
- If you are using the Flash library then uncomment #include "Flash.h" in EtherEventQueue.cpp and EtherEventQueue.h
- EtherEventQueue library configuration parameters
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

`EtherEventQueue.senderIP()` - get the IP Address of the sender of the most recently received event.
- Parameter: none
- Returns: IP Address of the sender of the most recent event
  - Type: IPAddress

`EtherEventQueue.flushReceiver()` - clear any buffered event and payload data so a new event can be received
- Parameter:none
- Returns:none

`EtherEventQueue.queue(target, port, event, payload, resendFlag)` - Send an event and payload
- Parameter: target - takes either the IP address or node number of the target device
  - Type: IPAddress/byte
- Parameter: port: - port to send the event to
  - Type: unsigned int
- Parameter: event: - string to send as the event
  - Type: char/int
- Parameter: payload:- payload to send with the event. If you don't want a payload then just use 0 for this parameter
  - Type: char/int8_t/byte/int/unsigned int/long/unsigned long/_FLASH_STRING
- Parameter: resendFlag - (0 == no resend, 1 == resend until successful send, 2 == resend until ack) If this is set to 2 then the queue will resend a message until the ack is received or the target IP times out
  - Type: byte
- Returns: 0 for failure, 1 for success, , 2 for success w/ queue overflow
  - Type: byte
  
`EtherEventQueue.queue(target, port, event, F(payload), payloadLength, resendFlag)` - Send an event and payload - this version of the function accepts payload strings placed in flash memory via the F() macro.
- Parameter: target - takes either the IP address or node number of the target device
  - Type: IPAddress/byte
- Parameter: port: - port to send the event to
  - Type: unsigned int
- Parameter: event: - string to send as the event
  - Type: char/int
- Parameter: payload:- payload to send with the event.
  - Type: F()/__FlashStringHelper
- Parameter: payloadLength:- length of the payload
  - Type: byte
- Parameter: resendFlag - (0 == no resend, 1 == resend until successful send, 2 == resend until ack) If this is set to 2 then the queue will resend a message until the ack is received or the target IP times out
  - Type: byte
- Returns: 0 for failure, 1 for success, , 2 for success w/ queue overflow
  - Type: byte
  
`EtherEventQueue.queue(target, port, F(event), eventLength, payload, resendFlag)` - Send an event and payload - this version of the function accepts event strings placed in flash memory via the F() macro.
- Parameter: target - takes either the IP address or node number of the target device
  - Type: IPAddress/byte
- Parameter: port: - port to send the event to
  - Type: unsigned int
- Parameter: event: - string to send as the event
  - Type: F()/__FlashStringHelper
- Parameter: eventLength:- length of the event
  - Type: byte
- Parameter: payload:- payload to send with the event. If you don't want a payload then just use 0 for this parameter
  - Type: char/int8_t/byte/int/unsigned int/long/unsigned long/_FLASH_STRING
- Parameter: resendFlag - (0 == no resend, 1 == resend until successful send, 2 == resend until ack) If this is set to 2 then the queue will resend a message until the ack is received or the target IP times out
  - Type: byte
- Returns: 0 for failure, 1 for success, , 2 for success w/ queue overflow
  - Type: byte
  
`EtherEventQueue.queue(target, port, F(event), eventLength, F(payload), payloadLength, resendFlag)` - Send an event and payload - this version of the function accepts event and payload strings placed in flash memory via the F() macro.
- Parameter: target - takes either the IP address or node number of the target device
  - Type: IPAddress/byte
- Parameter: port: - port to send the event to
  - Type: unsigned int
- Parameter: event: - string to send as the event
  - Type: F()/__FlashStringHelper
- Parameter: eventLength:- length of the event
  - Type: byte
- Parameter: payload:- payload to send with the event.
  - Type: F()/__FlashStringHelper
- Parameter: payloadLength:- length of the payload
  - Type: byte
- Parameter: resendFlag - (0 == no resend, 1 == resend until successful send, 2 == resend until ack) If this is set to 2 then the queue will resend a message until the ack is received or the target IP times out
  - Type: byte
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
- Returns: node number of the tirst newly timed out node found or -1 if no timed out node found
  - Type: int8_t
  
`EtherEventQueue.checkTimein()` - check for newly timed in nodes
- Parameter: none
- Returns: node number of the tirst newly timed out node found or -1 if no timed out node found
  - Type: int8_t
  
`EtherEventQueue.checkState(node)` - check if the device has not received any events in longer than the timeout duration
- Parameter: node - the node number of the node to be checked
  - Type: byte
- Returns: 0 == not timed out, 1 == timed out
  - Type: boolean

`EtherEventQueue.checkQueueOverflow()` - Check if the event queue has overflowed since the last time checkQueueOverflow() was called.
- Parameter: none
- Returns: 0 == queue has not overflowed since the last check, 1 == queue has overflowed since the last check
  - Type: boolean
  
`EtherEventQueue.getNode(IP)` - check if the device has not received any events in longer than the timeout duration
- Parameter: IP - the IP address to determine the node number of
  - Type: IPAddress
- Returns: 0 == not timed out, 1 == timed out
  - Type: int8_t


 #### Process
An overview of the event queue process:
- queue() - put event in the queue
  - non-keepalive events addressed to timed out nodes(other than self) are not queued
  - events addressed to non-nodes are always queued unless non-node sending is disabled
- queueHandler() - send event from queue
  - newly queued events(haven't attempted to send yet) are sent first FIFO
  - if there are no newly queued events then the sent events in the queue that have not been acked yet are resent FIFO
  - events other than keepalive to timed out nodes are discarded
  - send event
  - if the send is successful then remove events from the queue that have the resendFlag parameter == 0 or 1
  - if the send is not successful then remove events from the queue that have the resendFlag parameter == 0
- availableEvent() - check for incoming events
  - check the queue for internal events(addressed to self)
  - check for external events(from network)
  - if the event is from a node then the device and node timestamps are updated
  - if incoming event is an ack then remove the message the ack refers to from the queue
