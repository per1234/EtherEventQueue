EtherEventQueue
==========

Outgoing event queue for the EtherEvent Arduino library.
EtherEvent provides easy to use password authenticated network communication between Arduinos and other devices running EventGhost, Girder, or any other program compatible with the EventGhost Network Event Sender and Receiver plugins.

This is an alpha release. It is not thoroughly tested. Feel free to make pull requests or issue reports. Thanks!

#### Required Libraries
- EtherEvent http://github.com/per1234/EtherEvent

#### Related Programs
- Modified Ethernet library - allows the event sender's IP address to be recorded: http://github.com/per1234/Ethernet - make sure to choose the correct branch for your Arduino IDE version. If this library is not installed then timestamp for external nodes will not be reset on received event, senderIP function is disabled, and receiveNodesOnly function is disabled.
- UIPEthernet library for ENC28J60 ethernet chip: http://github.com/ntruchsess/arduino_uip
- EventGhost free open source automation tool for Windows http://eventghost.com
- TCP Events EventGhost plugin: http://www.eventghost.org/forum/viewtopic.php?p=16803 download link: http://docs.google.com/uc?id=0B3RTucUBY2bwVW5MQWdvRU90eTA - Improved network event sender/receiver allows sending events to multiple IP addresses
- Flash library to allow passing payload strings stored in flash memory without a string length argument: http://github.com/rkhamilton/Flash 

#### Installation
- 64k is the minimum recommended flash memory capacity of the MCU
- Download the most recent version of EtherEventQueue here: http://github.com/per1234/EtherEventQueue  - Click the "Download ZIP" button(or "Clone in Desktop" if you have GitHub Desktop installed)
- Extract the EtherEventQueue-master folder from the downloaded zip file
- Rename the folder EtherEventQueue
- Move the folder to the libraries folder under your Arduino sketchbook folder as configured in Arduino IDE File>Preferences>Sketchbook location.
- Repeat this process with the EtherEvent library and any other associated libraries you 
- If you are using the Flash library then uncomment #include "Flash.h" in EtherEventQueue.cpp and EtherEventQueue.h
- EtherEventQueue library configuration parameters
  - EtherEventQueueNodes.h - IP addresses of nodes can be defined here, then events can be queued for sending to a node using just the node number and the status of the node will be monitored.
  - EtherEventQueue.cpp
    - DEBUG - Set this to true to enable debug output via serial. This will increase the sketch size dramatically so only enable when needed.
  - EtherEventQueue.h
    - eventKeepalive - The event that can be periodically send to keep nodes from being considered timed out. The default value is "100". This event will not be passed on via availableEvent(). Any event will reset the timeout timer so this event only needs to be used if no other event has been sent within the timeout duration.
    - eventAck - The event that is sent back to the sender's IP address to acknowledge that an event has been received. The default value is "101". The payload of the ack is the ID number of the received event. When an ack is received it will not be passed on via availableEvent(). It is used to remove events that were queued with the resendFlag=2.
- Restart the Arduino IDE
- File>Examples>etherEventQueueExample
 - Set the device IP address, this can be any available IP address on the network. DHCP not currently implemented.
 - Set the device MAC address. This can be any address not already used on the network
 - Set the EtherEvent authentication password.
 - Set the EtherEvent TCP port.
- Upload example sketch to device
- Repeat with other connected devices. The serial monitor will show details of the test communications.

#### Usage
`EtherEventQueue.begin(password[, deviceID][, nodeCount][, queueSizeMax, sendEventLengthMax, sendPayloadLengthMax, receiveEventLengthMax, receivePayloadEventMax])` - Initialize EtherEventQueue.
- Parameter: password - EtherEvent password. This must match the password set in EventGhost.
  - Type: char array
- Parameter(optional): deviceID - The node number of the device. The default value is 0.
  - Type: byte
- Parameter(optional): nodeCount - The maximum number of nodes(including the device's node). The minimum value is 1 as one node is required for the device.  The default value is 1.
  - Type: byte
- Parameter(optional): queueSizeMax - Maximum number of events to queue. Longer entries will be truncated to this length. The default value is 5.
  - Type: byte
- Parameter(optional): sendEventLengthMax - Maximum event length to send. Longer entries will be truncated to this length. The default value is 15.
  - Type: byte
- Parameter(optional): sendPayloadLengthMax - Maximum payload length to send. Longer entries will be truncated to this length. The default value is 80.
  - Type: byte
- Parameter(optional): receiveEventLengthMax - Maximum event length to receive. Longer entries will be truncated to this length. The default value is 15.
  - Type: byte
- Parameter(optional): receivePayloadEventMax - Maximum payload length to receive. Longer entries will be truncated to this length. The default value is 80.
  - Type: byte
- Returns: boolean - true = success, false = memory allocation failure

`EtherEventQueue.availableEvent(ethernetServer)` - Returns the number of chars of event including null terminator available to read. availableEvent() will not receive a new event until the last event has been read(via readEvent()) or flushed(via flushReceiver()).
- Parameter: ethernetServer - The EthernetServer object created in the Ethernet setup of the user's sketch.
  - Type: EthernetServer
- Returns: Number of chars in the event including the null terminator at the end of the string.
  - Type: byte

`EtherEventQueue.availablePayload()` - Returns the number of chars of payload including null terminator available to read. availableEvent() must be called first.
- Parameter: none
- Returns: Number of chars in the payload including the null terminator at the end of the string.
  - Type: byte

`EtherEventQueue.readEvent(eventBuffer)` - Puts the event in the passed array. availableEvent() must be called first.
- Parameter: eventBuffer - Size a char array according to the result of availableEvent () and pass it to the readEvent  function. After that it will contain the event.
  - Type: char
- Returns: none

`EtherEventQueue.readPayload(payloadBuffer)` - Puts the payload string in the passed array. availableEvent() must be called first.
- Parameter: payloadBuffer - Size a char array according to the result of availablePayload () and pass it to the readPayload  function. After that it will contain the payload.
  - Type: char
- Returns: none

`EtherEventQueue.senderIP()` - Get the IP Address of the sender of the most recently received event. This function is only available if the modified Ethernet library is installed.
- Parameter: none
- Returns: IP Address of the sender of the most recent event.
  - Type: IPAddress

`EtherEventQueue.receivedEventID()` - Returns the event ID of the received event. This is needed for confirming receipt of queueTypeConfirm type events.
- Parameter: none
- Returns: Event ID of the received event.
  - Type: byte

`EtherEventQueue.flushReceiver()` - Clear any buffered event and payload data so a new event can be received.
- Parameter: none
- Returns: none

`EtherEventQueue.queue(target[, port], event[, eventLength], payload[, payloadLength], resendFlag)` - Send an event and payload
- Parameter: target - takes either the IP address or node number of the target device
  - Type: IPAddress/4 byte array/byte
- Parameter: port: - Port to send the event to.
  - Type: unsigned int
- Parameter: event: - string to send as the event
  - Type: char/int8_t/byte/int/unsigned int/long/unsigned long/_FLASH_STRING/__FlashStringHelper(F() macro)
- Parameter: eventLength - Length of the event. This parameter should only be used if event is of type __FlashStringHelper(F() macro).
  - Type: byte
- Parameter: payload - payload to send with the event. If you don't want a payload then just use "" for this parameter
  - Type: char/int8_t/byte/int/unsigned int/long/unsigned long/_FLASH_STRING/__FlashStringHelper(F() macro)
- Parameter: payloadLength:- length of the payload. This parameter should only be used if event is of type type __FlashStringHelper(F() macro).
  - Type: byte
- Parameter: resendFlag - (EtherEventQueue.queueTypeOnce == no resend, EtherEventQueue.queueTypeResend == resend until successful send, 
  - Values: EtherEventQueue.queueTypeOnce - Make one attempt at sending the event and then remove it from the queue.
            EtherEventQueue.queueTypeResend - Resend until successful send, then remove from queue.
            EtherEventQueue.queueTypeConfirm == Resend a message until the ack is received, the target IP times out, or the event overflows from the queue. The ack is the eventAck with the eventID of the event to confirm for a payload.
  - Type: byte
- Returns: false == failure, true == successfully queued, EtherEventQueue.queueSuccessOverflow == successfully queued w/ queue overflow
  - Type: byte
  
`EtherEventQueue.queueHandler(ethernetClient)` - Send queued events.
- Parameter: ethernetClient - The EthernetClient object created during the Ethernet library initialization.
  - Type: EthernetClient
- Returns: none
   
 `EtherEventQueue.flushQueue()` - Remove all events from the queue.
 - Returns: none

`EtherEventQueue.checkTimeout()` - Check for newly timed out nodes.
- Parameter: none
- Returns: Node number of the tirst newly timed out node found or -1 if no timed out node found.
  - Type: int8_t
  
`EtherEventQueue.checkTimein()` - Check for newly timed in nodes.
- Parameter: none
- Returns: Node number of the tirst newly timed out node found or -1 if no timed out node found.
  - Type: int8_t
  
`EtherEventQueue.checkState(node)` - Check if no events have been received from the given node in longer than the timeout duration. The device is considered timed out when no events have received in longer than the timeout duration.
- Parameter: node - The node number of the node to be checked.
  - Type: byte
- Returns: true == not timed out, false == timed out, -1 == invalid node number
  - Type: int8_t

`EtherEventQueue.checkQueueOverflow()` - Check if the event queue has overflowed since the last time checkQueueOverflow() was called.
- Parameter: none
- Returns: false == queue has not overflowed since the last check, true == queue has overflowed since the last check
  - Type: boolean
  
`EtherEventQueue.getNode(IP)` - Get the node number of an IP address. Nodes can be defined in EtherEventQueueNodes.h.
- Parameter: IP - The IP address to determine the node number of
  - Type: IPAddress or 4 byte array
- Returns: node number, -1 == no match
  - Type: int8_t
  
`EtherEventQueue.setResendDelay(resendDelay)` - Set the event resend delay.
- Parameter: resendDelay - (ms)The delay before resending resend or confirm type queued events.
  -Type: unsigned long
- Returns: none
  
`EtherEventQueue.getResendDelay()` - Returns the value of the queued event resend delay.
- Parameter: none
- Returns: resendDelay - (ms)The delay before resending resend or confirm type queued events.
  -Type: unsigned long
  
`EtherEventQueue.setNodeTimeoutDuration(nodeTimeoutDuration)` - Set the node timeout duration.
- Parameter: nodeTimeoutDuration - (ms)The amout of time without receiving an event from a node before it is considered timed out.
  -Type: unsigned long
- Returns: none
  
`EtherEventQueue.getNodeTimeoutDuration()` - Returns the value of the node timeout duration.
- Parameter: none
- Returns: nodeTimeoutDuration - (ms)The amout of time without receiving an event from a node before it is considered timed out.
  -Type: unsigned long

`EtherEventQueue.receiveNodesOnly(receiveNodesOnlyValue)` - Receive events from nodes only. This feature is turned off by default.
- Parameter: receiveNodesOnlyValue - true == receive from nodes only, false == receive from any IP address.
  - Type: boolean
- Returns: none

`EtherEventQueue.sendNodesOnly(sendNodesOnlyValue)` - Send events to nodes only. This feature is turned off by default.
- Parameter: sendNodesOnlyValue - true == semd to nodes only, false == send to any IP address.
  - Type: boolean
- Returns: none

`EtherEventQueue.setNode(nodeNumber, nodeIP)` - Set the IP address of a node.
- Parameter: nodeNumber - The number of the node to set.
  - Type: byte
- Parameter: nodeIP - The IP Address of the node to set.
  - Type: IPAddress or 4 byte array.
  - Returns: true == success, false == invalid nodeNumber

`EtherEventQueue.removeNode(node)` - Remove a node.
- Parameter: nodeNumber - The number or IP address of the node to remove.
  - Type: byte, 4 byte array, or IPAddress
- Returns: none

`EtherEventQueue.getIP(nodeNumber)` - Returns the IP address of the given node.
- Parameter: nodeNumber - The number of the node to return the IP address of.
  - Type: byte
- Returns: IP address of the given node.
  -Type: IPAddress

`EtherEventQueue.sendKeepalive(port)` - Sends keepalive to the first node that is within the keepalive margin of being timed out.
- Parameter: port - The port to send keepalives to.
  - Type: unsigned int
- Returns: none

`EtherEventQueue.getSendKeepaliveMargin()` - Returns the keepalive margin value.
- Parameter: none
- Returns: keepalive margin - (ms)the amount of time before the end of the timeout duration to send the keepalive.
  -Type: unsigned long

`EtherEventQueue.setSendKeepaliveMargin(keepaliveMargin)` - Sets the keepalive margin value.
- Parameter: keepaliveMargin - (ms)the amount of time before the end of the timeout duration to send the keepalive.
  - Type: unsigned long
- Returns: none

`EtherEventQueue.setSendKeepaliveResendDelay(sendKeepaliveResendDelay)` - Sets the keepalive resend delay.
- Parameter: sendKeepaliveResendDelay - (ms)the amount of time before the keepalive is resent after it was unsuccessfully sent.
  - Type: unsigned long
- Returns: none

`EtherEventQueue.getSendKeepaliveResendDelay()` - Returns the sendKeepaliveResendDelay value.
- Parameter: none
- Returns: The sendKeepaliveResendDelay value
  - Type: unsigned long

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
