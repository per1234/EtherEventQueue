EtherEventQueue
==========

Outgoing event queue for the EtherEvent [Arduino](http://arduino.cc) library. [EtherEvent](http://github.com/per1234/EtherEvent) provides easy to use password authenticated network communication via Ethernet between Arduinos and [EventGhost](http://eventghost.com), or any other program compatible with the EventGhost Network Event Sender and Receiver plugins.


#### Required Libraries
- EtherEvent: http://github.com/per1234/EtherEvent


#### Related Programs
- EventGhost free open source automation tool for Windows http://eventghost.com


<a id="installation"></a>
#### Installation
- 64kB is the minimum recommended flash memory capacity for use of this library with authentication. With authentication disabled it will work with 32kB
- Download the most recent version of EtherEventQueue here: https://github.com/per1234/EtherEventQueue/archive/master.zip
- Using Arduino IDE 1.0.x:
  - **Sketch > Import Library... > Add Library... >** select the downloaded file **> Open**
- Using Arduino IDE 1.5+:
  - **Sketch > Include Library > Add ZIP Library... >** select the downloaded file **> Open**
- Repeat this process with the [EtherEvent](http://github.com/per1234/EtherEvent) library and any other associated libraries.
- Running the example sketch:
  - **File > Examples > EtherEventQueue > BasicUsage**
  - Set the configuration parameters in the sketch for your network.
  - Upload to device.
  - Repeat with other connected devices.
  - Details of the test communications will be printed to the Serial Monitor.


#### Why Would I Want to Queue Events?
Sometimes when your device tries to send an event the target might not be available to receive the event. This could be caused by the target being temporarily offline or busy. EtherEventQueue offers the option to place events in a queue to be re-sent later. Another benefit is that you can queue multiple events at the same time and then send them out one at a time so that your device will not be tied up by sending them all at one time. You can also define a list of nodes, network addresses that are monitored. Events to nodes are only queued when they are online.


<a id="usage"></a>
#### Usage
For demonstration of library usage see the example sketches at **File > Examples > EtherEventQueue** and the EventGhost tree files in **examples\EventGhost-example-trees**.

##### `#include <EtherEventQueue.h>`
Allow access to the functions of the EtherEventQueue library.

##### `#define ETHEREVENT_NO_AUTHENTICATION`
Add this line above the `#include <EtherEventQueue.h>` and `#include <EtherEvent.h>` lines in your sketch to disable password authentication. Requires [my version of the TCPEvents plugin](https://github.com/per1234/TCPEvents) with the password fields left blank in the configurations for communication with EventGhost. With authentication disabled the MD5 library is not required, no need to set the password, memory usage is decreased significantly, and event transmission speed is increased. See the NoAuthentication example for a demonstration.

##### `#define ETHEREVENT_FAST_SEND`
Increase sending speed at the expense of increased memory use. Add this line above the `#include <EtherEventQueue.h>` and `#include <EtherEvent.h>` lines in your sketch. This significantly increases the speed of sending __FlashStringHelper (`F()` macro) events/payloads but also increases the sketch size and SRAM usage during the send process. ETHEREVENT_FAST_SEND also increases the speed of sending some other event/payload types.

##### `EtherEventQueue.begin([deviceID, nodeCount][, queueSizeMax, sendEventLengthMax, sendPayloadLengthMax, receiveEventLengthMax, receivePayloadLengthMax])`
Initialize EtherEventQueue.
- Parameter(optional): **deviceID** - The node number of the device. The default value is 0.
  - Type: byte
- Parameter(optional): **nodeCount** - The maximum number of nodes(including the device's node). The minimum value is 1 as one node is required for the device.  The default value is 1.
  - Type: byte
- Parameter(optional): **queueSizeMax** - Maximum number of events to queue. The default value is 5.
  - Type: byte
- Parameter(optional): **sendEventLengthMax** - Maximum event length to send. Longer events will be truncated to this length. The default value is 15.
  - Type: byte
- Parameter(optional): **sendPayloadLengthMax** - Maximum payload length to send. Longer payloads will be truncated to this length. The default value is 80.
  - Type: unsigned int
- Parameter(optional): **receiveEventLengthMax** - Maximum event length to receive. Longer events will be truncated to this length. The default value is 15.
  - Type: byte
- Parameter(optional): **receivePayloadLengthMax** - Maximum payload length to receive. Longer payloads will be truncated to this length. The default value is 80.
  - Type: unsigned int
- Returns: boolean - `true` = success, `false` = memory allocation failure

##### `EtherEventQueue.availableEvent(ethernetServer[, cookie])`
Returns the number of chars of the event, including null terminator, available to read. `EtherEventQueue.availableEvent()` will not receive a new event until the previously received event has been read (via `EtherEventQueue.readEvent()`) or flushed (via `EtherEventQueue.flushReceiver()`).
- Parameter: **ethernetServer** - The EthernetServer object created in the Ethernet setup of the user's sketch.
  - Type: EthernetServer
- Parameter(optional): **cookie** - Cookie value to use in the authentication process. This can be used to provide a truly random cookie for enhanced security. If this parameter is not specified then a pseudorandom cookie will be generated with `random()`.
  - Type: long
- Returns: Buffer size required to receive the event. This is the length of the received event, including null terminator.
  - Type: byte

##### `EtherEventQueue.availablePayload()`
Returns the number of chars of the payload, including null terminator, available to read. `EtherEventQueue.availableEvent()` must be called first.
- Returns: Buffer size required to receive the payload. This is the length of the received payload, including null terminator.
  - Type: unsigned int

##### `EtherEventQueue.readEvent(eventBuffer)`
Puts the event in the passed array. `EtherEventQueue.availableEvent()` must be called first.
- Parameter: **eventBuffer** - Size a char array according to the result of `EtherEventQueue.availableEvent()` and pass it to `EtherEventQueue.readEvent()`. After that it will contain the event.
  - Type: char array
- Returns: none

##### `EtherEventQueue.readPayload(payloadBuffer)`
Puts the payload string in the passed array. `EtherEventQueue.availableEvent()` must be called first.
- Parameter: **payloadBuffer** - Size a char array according to the result of `EtherEventQueue.availablePayload()` and pass it to `EtherEventQueue.readPayload()`. After that it will contain the payload.
  - Type: char array
- Returns: none

##### `EtherEventQueue.receivedEventID()`
Returns the event ID of the received event. This is used for confirming receipt(ACK) of `EtherEventQueue.eventTypeConfirm` type events.
- Returns: Event ID of the received event.
  - Type: byte

##### `EtherEventQueue.flushReceiver()`
Clear any buffered event and payload data so a new event can be received. The buffers are automatically cleared when the event and payload are read so this only needs to be done if there is an available event or payload that you don't want to read.
- Returns: none

##### `EtherEventQueue.queue(target, port, eventType, event[, payload])`
Send an event and payload
- Parameter: **target** - Either the IP address or node number of the target device. EtherEventQueue can also be used to send internal events by sending to the device's IPAddress or node number.
  - Type: IPAddress, 4 byte array, byte
- Parameter: **port** - Port to send the event to.
  - Type: unsigned int
- Parameter: **eventType**
  - Values:
    - `EtherEventQueue.eventTypeOnce` - Make one attempt at sending the event and then remove it from the queue.
    - `EtherEventQueue.eventTypeRepeat` - Resend until successful, then remove from queue.
    - `EtherEventQueue.eventTypeConfirm` - Resend until the ACK is received, the target node times out, or the event overflows from the queue. The ACK is the eventAck with the eventID of the event to confirm for a payload. Received ACKs are handled internally by EtherEventQueue and will not be passed on. eventAck must be set via `EtherEventQueue.setEventAck()` before this event type can be used.
    - `EtherEventQueue.eventTypeOverrideTimeout` - Similar to eventTypeOnce but the event will be sent to nodes even if they are timed out.
  - Type: byte
- Parameter: **event** - string to send as the event
  - Type: char array, char, int8_t, byte, int, unsigned int, long, unsigned long, __FlashStringHelper (`F()` macro), String, IPAddress, float, double
- Parameter: **payload** - payload to send with the event.
  - Type: char array, char, int8_t, byte, int, unsigned int, long, unsigned long, __FlashStringHelper (`F()` macro), String, IPAddress, float, double
- Returns: `false` = failure, `true` = successfully queued, `EtherEventQueue.queueSuccessOverflow` = successfully queued w/ queue overflow
  - Type: byte

##### `EtherEventQueue.setQueueDoubleDecimalPlaces(decimalPlaces)`
Set the number of decimal places to preserve when queuing double or float type events and payloads.
- Parameter: **decimalPlaces** - The default value is 3.
  - Type: byte
- Returns: none

##### `EtherEventQueue.queueHandler(ethernetClient)`
Send queued events.
- Parameter: **ethernetClient** - The EthernetClient object created during the Ethernet library initialization.
  - Type: EthernetClient
- Returns: `true` = event sent successfully or no send required, `false` = event send failed
  - Type: boolean

##### `EtherEventQueue.flushQueue()`
Remove all events from the queue.
 - Returns: none

##### `EtherEventQueue.checkQueueOverflow()`
Check if the event queue has overflowed since the last time `EtherEventQueue.checkQueueOverflow()` was called. When more events are added to the queue than the queueSizeMax value set in `EtherEventQueue.begin()` the oldest item in the queue is removed.
- Returns: `false` = queue has not overflowed since the last check, `true` = queue has overflowed since the last check
  - Type: boolean

##### `EtherEventQueue.setResendDelay(resendDelay)`
Set the event resend delay.
- Parameter: **resendDelay** - (ms) The delay before resending `EtherEventQueue.eventTypeRepeat` or `EtherEventQueue.eventTypeConfirm` type queued events.
  - Type: unsigned long
- Returns: none

##### `EtherEventQueue.getResendDelay()`
Returns the value of the queued event resend delay.
- Returns: resendDelay - (ms) The delay before resending `EtherEventQueue.eventTypeRepeat` or `EtherEventQueue.eventTypeConfirm` type queued events.
  - Type: unsigned long

##### `EtherEventQueue.setNode(nodeNumber, nodeIP)`
Define a node.
- Parameter: **nodeNumber** - The number of the node to set.
  - Type: byte
- Parameter: **nodeIP** - The IP Address of the node to set.
  - Type: IPAddress or 4 byte array.
- Returns: `true` = success, `false` = invalid nodeNumber
  - Type: boolean

##### `EtherEventQueue.removeNode(node)`
Remove a node.
- Parameter: **nodeNumber** - The number of the node to remove.
  - Type: byte
- Returns: none

##### `EtherEventQueue.getIP(nodeNumber)`
Returns the IP address of the given node.
- Parameter: **nodeNumber** - The number of the node to return the IP address of.
  - Type: byte
- Returns: IP address of the given node.
  - Type: IPAddress

##### `EtherEventQueue.getNode(IP)`
Get the node number of an IP address.
- Parameter: **IP** - The IP address to determine the node number of.
  - Type: IPAddress or 4 byte array
- Returns: Node number or -1 for no match for the supplied IP address.
  - Type: int8_t

##### `EtherEventQueue.checkTimeout()`
Check for newly timed out nodes.
- Returns: Node number of the first newly timed out node found or -1 if no timed out node found.
  - Type: int8_t

##### `EtherEventQueue.checkTimein()`
Check for nodes that were previously timed out but have "timed  in" by sending an event.
- Returns: Node number of the first newly timed in node found or -1 if no timed in node found.
  - Type: int8_t

##### `EtherEventQueue.checkState(node)`
Check the state of the given node. The node is considered timed out when no events have been received from it in longer than the timeout duration.
- Parameter: **node** - The node number of the node to be checked.
  - Type: byte
- Returns: `true` = not timed out, `false` = timed out, -1 = invalid node number
  - Type: int8_t

##### `EtherEventQueue.setNodeTimeoutDuration(nodeTimeoutDuration)`
Set the node timeout duration.
- Parameter: **nodeTimeoutDuration** - (ms) The amount of time without receiving an event from a node before it is considered timed out.
  - Type: unsigned long
- Returns: none

##### `EtherEventQueue.getNodeTimeoutDuration()`
Returns the value of the node timeout duration.
- Returns: nodeTimeoutDuration - (ms) The amount of time without receiving an event from a node before it is considered timed out.
  - Type: unsigned long

##### `EtherEventQueue.receiveNodesOnly(receiveNodesOnlyValue)`
Receive events from nodes only. This feature is turned off by default.
- Parameter: **receiveNodesOnlyValue** - `true` = receive from nodes only, `false` = receive from any IP address.
  - Type: boolean
- Returns: none

##### `EtherEventQueue.sendNodesOnly(sendNodesOnlyValue)`
Send events to nodes only. This feature is turned off by default.
- Parameter: **sendNodesOnlyValue** - `true` = send to nodes only, `false` = send to any IP address.
  - Type: boolean
- Returns: none

##### `EtherEventQueue.sendKeepalive(port)`
Sends a keepalive to the first node that is within the keepalive margin of being timed out. The keepalive is an event that is used only to keep nodes from timing out. It is handled internally to update the node timestamp and will not be passed on by EtherEventQueue.
- Parameter: **port** - The port to send the keepalive to.
  - Type: unsigned int
- Returns: none

##### `EtherEventQueue.setSendKeepaliveMargin(keepaliveMargin)`
Sets the keepalive margin value.
- Parameter: **keepaliveMargin** - (ms) The amount of time before the end of the timeout duration to send the keepalive. Note this function will not allow keepaliveMargin to be set to a value greater than nodeTimeoutDuration.
  - Type: unsigned long
- Returns: none

##### `EtherEventQueue.getSendKeepaliveMargin()`
Returns the keepalive margin value.
- Returns: keepalive margin - (ms) The amount of time before the end of the timeout duration to send the keepalive.
  - Type: unsigned long

##### `EtherEventQueue.setSendKeepaliveResendDelay(sendKeepaliveResendDelay)`
Sets the keepalive resend delay.
- Parameter: **sendKeepaliveResendDelay** - (ms) The amount of time before the keepalive is resent after it was unsuccessfully sent.
  - Type: unsigned long
- Returns: none

##### `EtherEventQueue.getSendKeepaliveResendDelay()`
Returns the sendKeepaliveResendDelay value.
- Returns: The sendKeepaliveResendDelay value
  - Type: unsigned long

##### `EtherEventQueue.setEventKeepalive(eventKeepalive)`
Defines the keepalive event.
- Parameter: **eventKeepaliveInput** - The event that is used as a keepalive.
  - Type: char array, byte, int, unsigned int, long, unsigned long, double, float, char, __FlashStringHelper (`F()` macro), String
- Returns: `true` = success, `false` = memory allocation failure.
  - Type: boolean

##### `EtherEventQueue.setEventAck(eventAck)`
Defines the event receipt confirmation event for use with eventTypeConfirm type events.
- Parameter: **eventAckInput** - The event that is used as an ACK.
  - Type: char array, byte, int, unsigned int, long, unsigned long, double, float, char, __FlashStringHelper (`F()` macro), String
- Returns: `true` = success, `false` = memory allocation failure.
  - Type: boolean


<a id="troubleshooting"></a>
#### Troubleshooting
- Debug output: Set `#define ETHEREVENTQUEUE_DEBUG true` in **EtherEventQueue.h** to get debug output in the serial monitor, this will slow down communication and increase memory usage so only enable debug output when needed.
- When EventGhost receives an event containing `.` it will cause the standard prefix to disappear from that event. This is a limitation of EventGhost and not EtherEvent.


<a id="process"></a>
#### Process
An overview of the event queue process:
- `EtherEventQueue.queue()` - Put event in the queue.
  - Events other than keepalive or `EtherEventQueue.eventTypeOverrideTimeout` type addressed to timed out nodes(other than self) are not queued.
  - Events addressed to non-nodes are always queued unless non-node sending is disabled (`EtherEventQueue.sendNodesOnly(true)`).
- `EtherEventQueue.queueHandler()` - Send event from queue.
  - Send newly queued event FIFO (first in, first out).
  - If there are no newly queued events then resend `EtherEventQueue.eventTypeRepeat` type events that were previously not successfully sent or `EtherEventQueue.eventTypeConfirm` type events that have not been ACKed yet FIFO.
  - Events other than keepalive to timed out nodes are discarded.
  - If the send is successful then remove events from the queue that have are `EtherEventQueue.eventTypeOnce` or `EtherEventQueue.eventTypeRepeat`.
  - If the send is not successful then remove events from the queue that are `EtherEventQueue.eventTypeOnce`.
- `EtherEventQueue.availableEvent()` - Check for incoming events.
  - Check the queue for internal events(addressed to self).
  - Check for external events(from network).
  - If the event is from a node then the device and node timestamps are updated.
  - If incoming event is an ACK then remove the message the ACK refers to from the queue.


#### Contributing
Pull requests or issue reports are welcome! Please see the [contribution rules](https://github.com/per1234/EtherEventQueue/blob/master/.github/CONTRIBUTING.md) for instructions.
