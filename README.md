EtherEventQueue
==========

Outgoing message queue for the EtherEvent authenticated network communication Arduino library.
EtherEvent is easy to use password authenticated network communication between Arduinos and EventGhost Network Event Sender/Receiver plugin, EventGhost TCPEvents plugin, Girder, and NetRemote.

This is an alpha release. It is not thoroughly tested. Feel free to make pull requests or issue reports. Thanks!

#### Required Libraries
- EtherEvent http://github.com/per1234/EtherEvent

#### Related Programs
- UIPEthernet library for ENC28J60 ethernet chip: http://github.com/ntruchsess/arduino_uip
- EventGhost is a free open source automation tool for Windows http://eventghost.com
- TCP Events EventGhost plugin by miljbee: http://www.eventghost.org/forum/viewtopic.php?p=16803 - Improved network event sender/receiver allows sending events to multiple IP addresses
- pfodCHAP - a much more rigorous authentication library: http://forward.com.au/pfod/pfodParserLibraries

#### Installation
- Make sure you have the most recent version of the library: http://github.com/per1234/EtherEventQueue
- 64k is the minimum recommended flash memory capacity of the MCU
- Download EtherEventQueue - Download ZIP button(or Clone in Desktop if you have GitHub Desktop installed)
- Extract the EtherEventQueue-master folder from the downloaded zip file
- Rename the folder EtherEventQueue
- Move the folder to your arduino sketchbook\libraries folder
- Repeat this process with the other required libraries
- EtherEventQueue library configuration parameters(EtherEventQueue.cpp):
  - there are several paramerters that can be configured in the library, they are documented there
- Restart the Arduino IDE
- File>Examples>etherEventQueueExample
- Set the device IP address, this can be any available IP address on the network. DHCP not currently implemented.
- Set the device MAC address. This can be any address not already used on the network
- Set the EtherEvent password. The password must be the same on all connected devices.
- Upload to device
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

`EtherEventQueue.availablePayload()` - Returns the number of chars of payload including null terminator available to read
- Parameter: none
- Returns: Number of chars in the payload including the null terminator at the end of the string.
  - Type: byte

`EtherEventQueue.readEvent(char eventBuffer[])` - Puts the event in the passed array
- Parameter: eventBuffer - size a char array according to the result of availableEvent () and pass it to the readEvent  function. After that it will contain the event.
  - Type: char
- Returns: none

`EtherEventQueue.readPayload(char payloadBuffer[])` - Puts the payload string in the passed array
- Parameter: payloadBuffer - size a char array according to the result of availablePayload () and pass it to the readPayload  function. After that it will contain the payload.
  - Type: char
- Returns: none   

`EtherEventQueue.flushReceiver()` - clear any buffered event and payload data so a new event can be received
- Parameter:none
- Returns:none

`EtherEventQueue.queue(targetIP, port, event, payload, resendFlag)` - Send an event and payload
- Parameter: targetIP - IP address to send the event to
  - Type: IPAddress
- Parameter: port: - port to send the event to
  - Type: unsigned int
- Parameter: event: - string to send as the event(char array).
  - Type: const char
- Parameter: payload:- payload to send with the event(char array). If you don't want a payload then just use "" for this parameter
  - Type: const char
- Parameter: resendFlag - (0 == no resend, 1 == resend) If this is set to 1 then the queue will resend a message until the ack is received or the target IP times out
  - Type: boolean
- Returns: 1 for success, 0 for failure, -1 == success w/ queue overflow
  - Type: byte
  
 `EtherEventQueue.queueHandler(ethernetClient)` - send queued events
 - Parameter: ethernetClient - the EthernetClient object created during the Ethernet library initialization
   - Type: EthernetClient
   
`EtherEventQueue.checkTimeout()` - check for newly timed out nodes
- Parameter: none
-Returns: IP address of the tirst newly timed out node found
  - Type: IPAddress
  
`EtherEventQueue.checkTimein()` - check for newly timed in nodes
- Parameter: none
-Returns: IP address of the tirst newly timed out node found
  - Type: IPAddress
  
`EtherEventQueue.checkTimeoutSelf()` - check if the device has not received any events in longer than the timeout duration
- Parameter: none
-Returns: 0 == not timed out, 1 == timed out
  - Type: boolean
  
 
 #### Process
- queue event
 - only send to nodes that aren't timed out
- send event from queue
 - new events are sent first
 - events to timed out nodes are discarded
 - event sent
 - remove events from the queue that have the resendFlag parameter == 0
 - listen for incoming events
 - if incoming event is an ack then remove the message the ack refers to from the queue