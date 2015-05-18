EtherEventQueue
==========

Outgoing event queue for the EtherEvent Arduino library.
EtherEvent provides easy to use password authenticated network communication via Ethernet between Arduinos and other devices running EventGhost, Girder, or any other program compatible with the EventGhost Network Event Sender and Receiver plugins.

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
- 64k is the minimum recommended flash memory capacity for use of this library.
- Download the most recent version of EtherEventQueue here: https://github.com/per1234/EtherEventQueue/archive/master.zip
- Extract the **EtherEventQueue-master** folder from the downloaded zip file.
- Rename the folder **EtherEventQueue**.
- Move the folder to the libraries folder under your Arduino sketchbook folder as configured in Arduino IDE **File > Preferences > Sketchbook** location.
- Repeat this process with the EtherEvent library and any other associated libraries you wish to use.
- Restart the Arduino IDE if it is open.
- Running the example sketch:
  - File > Examples > EtherEventQueueExample
  - Set the configuration parameters.
  - Upload to device.
  - Repeat with other connected devices. The serial monitor will show details of the test communications.


#### About Events and Payloads
Events are used to trigger an action. The payload is information that accompanies the event. An example is an event code that triggers the display of the payload. Some events don't require a payload and in this case the payload may be left blank.


#### Usage
For demonstration of library usage see the example sketches.
`EtherEventQueue.begin([deviceID, nodeCount][, queueSizeMax, sendEventLengthMax, sendPayloadLengthMax, receiveEventLengthMax, receivePayloadEventMax])` - Initialize EtherEventQueue.
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

`EtherEventQueue.receivedEventID()` - Returns the event ID of the received event. This is needed for confirming receipt of queueTypeConfirm type events.
- Parameter: none
- Returns: Event ID of the received event.
  - Type: byte

`EtherEventQueue.flushReceiver()` - Clear any buffered event and payload data so a new event can be received.
- Parameter: none
- Returns: none

`EtherEventQueue.queue(target, port, resendFlag, event[, eventLength][, payload[, payloadLength]])` - Send an event and payload
- Parameter: target - Takes either the IP address or node number of the target device. EtherEventQueue can also be used to send internal events by sending to the device IPAddress or node number.
  - Type: IPAddress/4 byte array/byte
- Parameter: port: - Port to send the event to.
  - Type: unsigned int
- Parameter: resendFlag - (EtherEventQueue.queueTypeOnce == no resend, EtherEventQueue.queueTypeResend == resend until successful send,
  - Values: EtherEventQueue.queueTypeOnce - Make one attempt at sending the event and then remove it from the queue.
            EtherEventQueue.queueTypeResend - Resend until successful send, then remove from queue.
            EtherEventQueue.queueTypeConfirm == Resend a message until the ack is received, the target IP times out, or the event overflows from the queue. The ack is the eventAck with the eventID of the event to confirm for a payload. Received acks are handled internally by EtherEventQueue and will not be passed on.
  - Type: byte
- Parameter: event: - string to send as the event
  - Type: char/int8_t/byte/int/unsigned int/long/unsigned long/_FLASH_STRING/__FlashStringHelper(F() macro)
- Parameter(optional): eventLength - Length of the event. This parameter should only be used if event is of type __FlashStringHelper(F() macro).
  - Type: byte
- Parameter: payload - payload to send with the event. The payload is not optional when the event is of type __FlashStringHelper(F() macro).
  - Type: char/int8_t/byte/int/unsigned int/long/unsigned long/_FLASH_STRING/__FlashStringHelper(F() macro)
- Parameter: payloadLength:- length of the payload. This parameter should only be used if event is of type type __FlashStringHelper(F() macro).
  - Type: byte
- Returns: false == failure, true == successfully queued, EtherEventQueue.queueSuccessOverflow == successfully queued w/ queue overflow
  - Type: byte

`EtherEventQueue.queueHandler(ethernetClient)` - Send queued events.
- Parameter: ethernetClient - The EthernetClient object created during the Ethernet library initialization.
  - Type: EthernetClient
- Returns: none

 `EtherEventQueue.flushQueue()` - Remove all events from the queue.
 - Returns: none

`EtherEventQueue.checkQueueOverflow()` - Check if the event queue has overflowed since the last time checkQueueOverflow() was called.
- Parameter: none
- Returns: false == queue has not overflowed since the last check, true == queue has overflowed since the last check
  - Type: boolean

`EtherEventQueue.setResendDelay(resendDelay)` - Set the event resend delay.
- Parameter: resendDelay - (ms)The delay before resending resend or confirm type queued events.
  -Type: unsigned long
- Returns: none

`EtherEventQueue.getResendDelay()` - Returns the value of the queued event resend delay.
- Parameter: none
- Returns: resendDelay - (ms)The delay before resending resend or confirm type queued events.
  -Type: unsigned long

`EtherEventQueue.setNode(nodeNumber, nodeIP)` - Set the IP address of a node.
- Parameter: nodeNumber - The number of the node to set.
  - Type: byte
- Parameter: nodeIP - The IP Address of the node to set.
  - Type: IPAddress or 4 byte array.
  - Returns: true == success, false == invalid nodeNumber

`EtherEventQueue.removeNode(node)` - Remove a node.
- Parameter: nodeNumber - The number of the node to remove.
  - Type: byte
- Returns: none

`EtherEventQueue.getIP(nodeNumber)` - Returns the IP address of the given node.
- Parameter: nodeNumber - The number of the node to return the IP address of.
  - Type: byte
- Returns: IP address of the given node.
  -Type: IPAddress

`EtherEventQueue.getNode(IP)` - Get the node number of an IP address. Nodes can be defined in EtherEventQueueNodes.h.
- Parameter: IP - The IP address to determine the node number of
  - Type: IPAddress or 4 byte array
- Returns: node number, -1 == no match
  - Type: int8_t

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

`EtherEventQueue.sendKeepalive(port)` - Sends keepalive to the first node that is within the keepalive margin of being timed out. The keepalive is an event that is used only to keep nodes from timing out, it is handled internally update the node timestamp and will not be passed on by EtherEventQueue.
- Parameter: port - The port to send keepalive to.
  - Type: unsigned int
- Returns: none

`EtherEventQueue.setSendKeepaliveMargin(keepaliveMargin)` - Sets the keepalive margin value.
- Parameter: keepaliveMargin - (ms)the amount of time before the end of the timeout duration to send the keepalive.
  - Type: unsigned long
- Returns: none

`EtherEventQueue.getSendKeepaliveMargin()` - Returns the keepalive margin value.
- Parameter: none
- Returns: keepalive margin - (ms)the amount of time before the end of the timeout duration to send the keepalive.
  -Type: unsigned long

`EtherEventQueue.setSendKeepaliveResendDelay(sendKeepaliveResendDelay)` - Sets the keepalive resend delay.
- Parameter: sendKeepaliveResendDelay - (ms)the amount of time before the keepalive is resent after it was unsuccessfully sent.
  - Type: unsigned long
- Returns: none

`EtherEventQueue.getSendKeepaliveResendDelay()` - Returns the sendKeepaliveResendDelay value.
- Parameter: none
- Returns: The sendKeepaliveResendDelay value
  - Type: unsigned long

`EtherEventQueue.setEventKeepalive(eventKeepalive[, eventKeepaliveLength])` - Defines the keepalive event.
- Parameter: eventKeepaliveInput - The event that is used as a keepalive.
  - Type: char array, int, unsigned int, long, unsigned long, F()/__FlashStringHelper, _FLASH_STRING
- Parameter: eventKeepaliveInputLength - The length of the event - use this parameter only if using F()/__FlashStringHelper type eventKeepalive.
  - Type: byte
- Returns: true == success, false == memory allocation failure.
  - Type: boolean

`EtherEventQueue.setEventAck(eventAck[, eventAckLength])` - Defines the event receipt confirmation event for use with queueTypeConfirm type events.
- Parameter: eventAckInput - The event that is used as an ack.
  - Type: char array, int, unsigned int, long, unsigned long, F()/__FlashStringHelper, _FLASH_STRING
- Parameter: eventKeepaliveInputLength - The length of the event - use this parameter only if using F()/__FlashStringHelper type eventAck.
  - Type: byte
- Returns: true == success, false == memory allocation failure.
  - Type: boolean


#### Configuration Parameters
There are a couple of flags that can be set in the source files to enable extra features:
- Debug - set `#define DEBUG true` in **EtherEventQueue.h** to get debug output in the serial monitor, this will slow down communication so only enable debug output when needed.
- If you are using the Flash library then uncomment `//#include "Flash.h"` in **EtherEventQueue.cpp** and **EtherEventQueue.h**.

#### Process
An overview of the event queue process:
- queue() - Put event in the queue.
  - Non-keepalive events addressed to timed out nodes(other than self) are not queued.
  - Events addressed to non-nodes are always queued unless non-node sending is disabled.
- queueHandler() - Send event from queue.
  - Newly queued events(haven't attempted to send yet) are sent first FIFO.
  - If there are no newly queued events then the sent events in the queue that have not been acked yet are resent FIFO.
  - Events other than keepalive to timed out nodes are discarded.
  - Send event.
  - If the send is successful then remove events from the queue that have the resendFlag parameter == queueTypeOnce or queueTypeRepeat.
  - If the send is not successful then remove events from the queue that have the resendFlag parameter == queueTypeOnce.
- availableEvent() - Check for incoming events.
  - Check the queue for internal events(addressed to self).
  - Check for external events(from network).
  - If the event is from a node then the device and node timestamps are updated.
  - If incoming event is an ack then remove the message the ack refers to from the queue.

