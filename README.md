EtherEventQueue
==========

Outgoing event queue for the EtherEvent Arduino library.
EtherEvent provides easy to use password authenticated network communication between Arduinos and other devices running EventGhost, Girder, or any other program compatible with the EventGhost Network Event Sender and Receiver plugins.

This is an alpha release. It is not thoroughly tested. Feel free to make pull requests or issue reports. Thanks!

#### Required Libraries
- EtherEvent http://github.com/per1234/EtherEvent
- Modified Ethernet library - allows the event sender's IP address to be recorded: http://github.com/per1234/Ethernet - make sure to choose the correct branch for your Arduino IDE version

#### Related Programs
- UIPEthernet library for ENC28J60 ethernet chip: http://github.com/ntruchsess/arduino_uip
- EventGhost free open source automation tool for Windows http://eventghost.com
- TCP Events EventGhost plugin: http://www.eventghost.org/forum/viewtopic.php?p=16803 download link: http://docs.google.com/uc?id=0B3RTucUBY2bwVW5MQWdvRU90eTA - Improved network event sender/receiver allows sending events to multiple IP addresses
- Flash library to allow passing payload strings stored in flash memory without a string length argument: http://github.com/rkhamilton/Flash 

#### Installation
- 64k is the minimum recommended flash memory capacity of the MCU
- Download the most recent version of EtherEventQueue here http://github.com/per1234/EtherEventQueue  - Download ZIP button(or Clone in Desktop if you have GitHub Desktop installed)
- Extract the EtherEventQueue-master folder from the downloaded zip file
- Rename the folder EtherEventQueue
- Move the folder to your arduino sketchbook\libraries folder
- Repeat this process with the other required libraries
- If you are using the Flash library then uncomment #include "Flash.h" in EtherEventQueue.cpp and EtherEventQueue.h
- EtherEventQueue library configuration parameters
  - EtherEventQueueNodes.h - IP addresses of nodes can be defined here, then events can be queued for sending to a node using just the node number and the status of the node will be monitored.
  - EtherEventQueue.cpp
    - DEBUG - Set this to true to enable debug output via serial. This will increase the sketch size dramatically so only enable when needed.
    - receiveNodesOnly - Set this to true to only allow events to be received from IP addresses defined in EtherEventQueueNodes.h
    - sendNodesOnly - Set this to true to only allow events to be sent to IP addresses defined in EtherEventQueueNodes.h
    - nodeTimeout - (ms)the node is timed out if it has been longer than this duration since the last event was received from it
    - nodeTimeoutSelf  - (ms)the device is timed out if it has been longer than this duration since any event was received
    - eventKeepalive - The event that can be periodically send to keep nodes from being considered timed out. The default value is "100". This event will not be passed on via availableEvent(). Any event will reset the timeout timer so this event only needs to be used if no other event has been sent within the timeout duration.
    - eventAck - The event that is sent back to the sender's IP address to acknowledge that an event has been received. The default value is "101". The payload of the ack is the ID number of the received event. When an ack is received it will not be passed on via availableEvent(). It is used to remove events that were queued with the resendFlag=2.
    - resendDelay - The delay between resends of messages queued with resendFlag=1 or resendFlag=2.
- Restart the Arduino IDE
- File>Examples>etherEventQueueExample
 - Set the device IP address, this can be any available IP address on the network. DHCP not currently implemented.
 - Set the device MAC address. This can be any address not already used on the network
 - Set the EtherEvent authentication password.
 - Set the EtherEvent TCP port.
- Upload example sketch to device
- Repeat with other connected devices. The serial monitor will show details of the test communications.

#### Usage
`EtherEventQueue.begin(password, deviceID, port[, queueSizeMax, sendEventLengthMax, sendPayloadLengthMax, receiveEventLengthMax, receivePayloadEventMax])` - Initialize EtherEventQueue
- Parameter: password - EtherEvent password. This must match the password set in EventGhost.
  - Type: char array
- Parameter: deviceID - The node number of the device. The device IP address must be in the node array in EtherEventQueue.cpp
  - Type: byte
- Parameter: port - The port being used for events. This is used only for sending acks, queue() allows sending to any port, the device Ethernet port is configured with the EthernetServer initialization
  - Type: unsigned int
- Parameter(optional): queueSizeMax - Maximum number of events to queue. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
  - Type: byte
- Parameter(optional): sendEventLengthMax - Maximum event length to send. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
  - Type: byte
- Parameter(optional): sendPayloadLengthMax - Maximum payload length to send. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
  - Type: byte
- Parameter(optional): receiveEventLengthMax - Maximum event length to receive. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
  - Type: byte
- Parameter(optional): receivePayloadEventMax - Maximum payload length to receive. Longer entries will be truncated to this length. If this parameter is not passed then the default will be used.
  - Type: byte
- Returns: boolean - true = success, false = memory allocation failure

`EtherEventQueue.availableEvent(ethernetServer)` - Returns the number of chars of event including null terminator available to read. availableEvent() will not receive a new event until the last event has been read(via readEvent()) or flushed(via flushReceiver()).
- Parameter: ethernetServer - the EthernetServer object created in the Ethernet setup of the user's sketch
  - Type: EthernetServer
- Returns: Number of chars in the event including the null terminator at the end of the string.
  - Type: byte

`EtherEventQueue.availablePayload()` - Returns the number of chars of payload including null terminator available to read. availableEvent() must be called first.
- Parameter: none
- Returns: Number of chars in the payload including the null terminator at the end of the string.
  - Type: byte

`EtherEventQueue.readEvent(eventBuffer)` - Puts the event in the passed array. availableEvent() must be called first.
- Parameter: eventBuffer - size a char array according to the result of availableEvent () and pass it to the readEvent  function. After that it will contain the event.
  - Type: char
- Returns: none

`EtherEventQueue.readPayload(payloadBuffer)` - Puts the payload string in the passed array. availableEvent() must be called first.
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
- Parameter: resendFlag - (0 == no resend, 1 == resend until successful send, 2 == resend until ack) If this is set to 2 then the queue will resend a message until the ack is received or the target IP times out or the queue exceeds the maximum queue size and the oldest queued events are removed.
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
  
`EtherEventQueue.getNode(IP)` - Get the node number of an IP address. Nodes can be defined in EtherEventQueueNodes.h.
- Parameter: IP - the IP address to determine the node number of
  - Type: IPAddress or byte
- Returns: node number, -1 == no match
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
