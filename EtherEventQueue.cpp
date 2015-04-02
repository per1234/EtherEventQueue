//EtherEventQueue outgoing event queue for the EtherEvent authenticated network communication arduino library: http://github.com/per1234/EtherEvent
#include <Arduino.h>
#include "EtherEventQueue.h"  //http://github.com/per1234/EtherEventQueue
#include "EtherEventQueueNodes.h"
#include <SPI.h>  //for the ethernet library
#include "Ethernet.h"
#include "EtherEvent.h"  //http://github.com/per1234/EtherEvent
#include "Numlen.h"  //For finding the length of numbers. Included with the EtherEventQueue library or the latest version available here: http://github.com/per1234/Numlen
#include "Flash.h"  //https://github.com/rkhamilton/Flash - uncomment this line if you have the Flash library installed

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//START user configuration parameters
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DEBUG false  //(false == serial debug output off,  true == serial debug output on)The serial debug output will increase memory usage and communication latency so only enable when in use.
#define Serial if(DEBUG)Serial

const boolean receiveNodesOnly = false;  //restrict event receiving to nodes only
const boolean sendNodesOnly = false;  //restrict event sending to nodes only

const unsigned long nodeTimeout = 270000;  //(ms)the node is timed out if it has been longer than this duration since the last event was received from it
const unsigned long nodeTimeoutSelf = 120000;  //(ms)the device is timed out if it has been longer than this duration since any event was received

const char eventKeepalive[] = "100";  //the library handles these special events differently
const char eventAck[] = "101";
const unsigned int resendDelay = 45000;  //(ms)delay between resends of messages
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//END user configuration parameters
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

byte nodeCount = sizeof(EtherEventQueueNodes::nodeIP) / sizeof(EtherEventQueueNodes::nodeIP[0]);
const byte eventIDlength = 2;  //number of characters of the message ID that is appended to the start of the raw payload, the event ID must be exactly this length
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//begin
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::begin(char password[], byte nodeDeviceInput, unsigned int portInput, byte queueSizeMaxInput, byte sendEventLengthMaxInput, byte sendPayloadLengthMaxInput, byte receivedEventLengthMaxInput, byte receivedPayloadLengthMaxInput) {
  nodeDevice = nodeDeviceInput;
  nodeState[nodeDevice] = 1;  //start the device as timed in
  port = portInput;

  //buffer sizing - these are dynamically allocated so that the sized can be set via the API
  //size send event queue buffers
  queueSizeMax = min(queueSizeMaxInput, 90); //the current system uses a 2 digit messageID so the range is 10-99, this restricts the queueSizeMax <= 90
  IPqueue = (byte**)malloc(queueSizeMax * sizeof(byte*));  //have to use 4 byte arrays for the IP addresses instead of IPAddress because I can't get IPAddress to work with malloc
  for (byte counter = 0; counter < queueSizeMax; counter++) {
    IPqueue[counter] = (byte*)malloc(4);  //4 bytes/IP address
  }

  portQueue = (unsigned int*)malloc(queueSizeMax * sizeof(unsigned int));
  sendEventLengthMax = sendEventLengthMaxInput;
  eventQueue = (char**)malloc(queueSizeMax * sizeof(char*));
  for (byte counter = 0; counter < queueSizeMax; counter++) {
    eventQueue[counter] = (char*)malloc((sendEventLengthMax + 1) * sizeof(char));
  }
  eventIDqueue = (byte*)malloc(queueSizeMax * sizeof(byte));
  sendPayloadLengthMax = sendPayloadLengthMaxInput;
  payloadQueue = (char**)malloc(queueSizeMax * sizeof(char*));
  for (byte counter = 0; counter < queueSizeMax; counter++) {
    payloadQueue[counter] = (char*)malloc((sendPayloadLengthMax + 1) * sizeof(char));
  }
  resendFlagQueue = (byte*)malloc(queueSizeMax * sizeof(byte));

  //size received event buffers
  receivedEventLengthMax = receivedEventLengthMaxInput;
  receivedEvent = (char*)malloc((receivedEventLengthMax + 1) * sizeof(char));
  receivedPayloadLengthMax = receivedPayloadLengthMaxInput;
  receivedPayload = (char*)malloc((receivedPayloadLengthMax + 1) * sizeof(char));
  EtherEvent.begin(password, receivedEventLengthMax, eventIDlength + receivedPayloadLengthMax); //initialize EtherEvent
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//availableEvent - check for new incoming events, process and buffer them and return the length of the event string
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte EtherEventQueueClass::availableEvent(EthernetServer &ethernetServer) {
  if (receivedEventLength == 0) {  //there is no event buffered
    if (localEventQueueCount > 0) {
      for (int queueStepCount = queueSize - 1; queueStepCount >= 0; queueStepCount--) {  //internal event system: step through the queue from the newest to oldest
        if (getNode(IPqueue[queueStepCount]) == nodeDevice) {  //internal event
          strcpy(receivedEvent, eventQueue[queueStepCount]);
          Serial.print(F("EtherEventQueue.availableEvent: internal event="));
          Serial.println(receivedEvent);
          strcpy(receivedPayload, payloadQueue[queueStepCount]);
          Serial.print(F("EtherEventQueue.availableEvent: internal event payload="));
          Serial.println(receivedPayload);
          receivedIP = IPAddress(IPqueue[queueStepCount]);
          Serial.print(F("EtherEventQueue.availableEvent: receivedIP="));
          Serial.println(receivedIP);
          remove(queueStepCount);  //remove the event from the queue
          queueNewCount--;
          localEventQueueCount--;
          return strlen(receivedEvent);
        }
      }
    }

    if (const byte availableBytesEvent = EtherEvent.availableEvent(ethernetServer)) {  //there is a new event
      Serial.println(F("---------------------------"));
      Serial.print(F("EtherEventQueue.availableEvent: EtherEvent.availableEvent()="));
      Serial.println(availableBytesEvent);
      receivedIP = EtherEvent.senderIP();
      Serial.print(F("EtherEventQueue.availableEvent: remoteIP="));
      Serial.println(receivedIP);

      nodeTimestamp[nodeDevice] = millis();  //set the device timestamp(using the nodeDevice because that part of the array is never used otherwise)
      //update timed out status of the event sender
      byte senderNode = getNode(receivedIP);  //get the node of the senderIP
      if (senderNode >= 0) {  //receivedIP is a node(-1 indicates no node match)
        nodeTimestamp[senderNode] = nodeTimestamp[nodeDevice];  //set the individual timestamp, any communication is considered to be a keepalive - the nodeTimestamp for the device has just been set so I am using that variable so I don't have to call millis() twice for efficiency
      }
      else if (receiveNodesOnly == 1) {  //the event was not received from a node and it is configured to receive events from node IPs only
        Serial.println(F("EtherEventQueue.availableEvent: unauthorized IP"));
        EtherEvent.flushReceiver();  //event has not been read yet so have to flush
        return 0;
      }

      EtherEvent.readEvent(receivedEvent);  //put the event in the buffer
      Serial.print(F("EtherEventQueue.availableEvent: ethernetReadEvent="));
      Serial.println(receivedEvent);

      if (strcmp(receivedEvent, eventKeepalive) == 0) {  //keepalive received
        Serial.println(F("EtherEventQueue.availableEvent: keepalive received"));
        flushReceiver();  //the event has been read so EtherEventQueue has to be flushed
        return 0;  //receive keepalive silently
      }

      byte payloadLength = EtherEvent.availablePayload();
      Serial.print(F("EtherEventQueue.availableEvent: EtherEvent.availablePayload()="));
      Serial.println(payloadLength);
      char receivedPayloadRaw[payloadLength];
      EtherEvent.readPayload(receivedPayloadRaw);  //read the payload to the buffer
      Serial.print(F("EtherEventQueue.availableEvent: rawPayload="));
      Serial.println(receivedPayloadRaw);

      //break the payload down into parts and convert the eventID, code, and target address to byte, the true payload stays as a char array
      char receivedEventID[eventIDlength + 1];
      for (byte count = 0; count < eventIDlength; count++) {
        receivedEventID[count] = receivedPayloadRaw[count];
      }
      receivedEventID[eventIDlength] = 0;  //add the null terminator because there is not one after the id in the string
      Serial.print(F("EtherEventQueue.availableEvent: ethernetReadeventID="));
      Serial.println(receivedEventID);

      if (payloadLength > eventIDlength + 1) {  //there is a true payload
        for (byte count = 0; count < payloadLength - eventIDlength; count++) {
          receivedPayload[count] = receivedPayloadRaw[count + eventIDlength];  //(TODO: just use receivedPayload for the buffer instead of having the raw buffer)
        }
        Serial.print(F("EtherEventQueue.availableEvent: receivedPayload="));
        Serial.println(receivedPayload);
      }

      if (strcmp(receivedEvent, eventAck) == 0) {  //ack handler
        Serial.println(F("EtherEventQueue.availableEvent: ack received"));
        byte receivedPayloadInt = atoi(receivedPayload);  //convert to a byte
        for (byte count = 0; count < queueSize; count++) {  //step through the currently occupied section of the eventIDqueue[]
          if (receivedPayloadInt == eventIDqueue[count] && resendFlagQueue[count] == 2) {  //the ack is for the eventID of this item in the queue and the resend flag indicates it is expecting an ack(non-ack events are not removed because obviously they haven't been sent yet if they're still in the queue so the ack can't possibly be for them)
            Serial.println(F("EtherEventQueue.availableEvent: ack eventID match"));
            remove(count);  //remove the message from the queue
          }
        }
        flushReceiver();  //event and payload have been read so only have to flush EtherEventQueue
        return 0;  //receive ack silently
      }

      Serial.println(F("EtherEventQueue.availableEvent: send ack"));
      queue(receivedIP, port, eventAck, receivedEventID, 0);  //send the ack - the eventID of the received message is the payload

      receivedEventLength = availableBytesEvent;  //there is a new event
    }
  }
  return receivedEventLength;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//availablePayload - returns the number of chars in the payload including the null terminator if there is one
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte EtherEventQueueClass::availablePayload() {
  if (byte length = strlen(receivedPayload)) {  //strlen(receivedPayload)>0
    return length + 1;  //length of the payload + null terminator
  }
  return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//readEvent - places the event into the passed buffer
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::readEvent(char eventBuffer[]) {
  Serial.println(F("EtherEventQueue.readEvent: start"));
  strcpy(eventBuffer, receivedEvent);
  receivedEventLength = 0;  //enable availableEvent() to receive new events
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//readPayload - places the payload into the passed buffer
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::readPayload(char payloadBuffer[]) {
  Serial.println(F("EtherEventQueue.readPayload: start"));
  strcpy(payloadBuffer, receivedPayload);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//senderIP - returns the IP address of the sender of the last event
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPAddress EtherEventQueueClass::senderIP() {
  return receivedIP;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//flushReceiver - dump the last message received so another one can be received
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::flushReceiver() {
  receivedEvent[0] = 0;  //reset the event buffer
  receivedPayload[0] = 0;  //reset the payload buffer
  receivedEventLength = 0;  //enable availableEvent() to receive new events
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//queue - add the relayed outgoing message to the send queue. Returns: 0==fail, 1==success, 2==success w/ queue overflow
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const char event[], const char payload[], byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, char event, char payload version): start"));
  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payload, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const char event[], const __FlashStringHelper* payloadFlashString, byte payloadLength, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, char event, F() payload version): start"));

  //convert __FlashStringHelper* to char
  char payloadChar[payloadLength + 1];  //Size array as needed.
  memcpy_P(payloadChar, payloadFlashString, payloadLength);
  payloadChar[payloadLength] = 0;  //null terminator

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const char event[], int payloadInt, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, char event, int payload version): start"));

  //convert int to char
  char payloadChar[Numlen.numlen(payloadInt) + 1];
  itoa(payloadInt, payloadChar, 10);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const char event[], unsigned int payloadUint, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, char event, unsigned int payload version): start"));

  //convert unsigned int to char
  char payloadChar[Numlen.numlen(payloadUint) + 1];
  sprintf_P(payloadChar, PSTR("%u"), payloadUint);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const char event[], long payloadLong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, char event, long payload version): start"));

  //convert long to char
  char payloadChar[Numlen.numlen(payloadLong) + 1];
  ltoa(payloadLong, payloadChar, 10);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const char event[], unsigned long payloadUlong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, char event, unsigned long payload version): start"));

  //convert unsigned long to char
  char payloadChar[Numlen.numlen(payloadUlong) + 1];
  ultoa(payloadUlong, payloadChar, 10);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}


#ifdef __FLASH_H__
byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const char event[], _FLASH_STRING payloadFlashString, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, char event, _FLASH_STRING payload version): start"));

  //convert the _FLASH_STRING to char
  byte stringLength =  payloadFlashString.length();
  char payloadChar[stringLength + 1];
  payloadFlashString.copy(payloadChar, stringLength, 0);
  payloadChar[stringLength] = 0;

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}
#endif


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const char event[], const __FlashStringHelper* payloadFlashString, byte payloadLength, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, char event, F() payload version): start"));

  //convert __FlashStringHelper* to char
  char payloadChar[payloadLength + 1];  //Size array as needed.
  memcpy_P(payloadChar, payloadFlashString, payloadLength);
  payloadChar[payloadLength] = 0;  //null terminator

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const char event[], int payloadInt, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, char event, int payload version): start"));

  //convert int to char
  char payloadChar[Numlen.numlen(payloadInt) + 1];
  itoa(payloadInt, payloadChar, 10);

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const char event[], unsigned int payloadUint, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, char event, unsigned int payload version): start"));

  //convert unsigned int to char
  char payloadChar[Numlen.numlen(payloadUint) + 1];
  sprintf_P(payloadChar, PSTR("%u"), payloadUint);

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const char event[], long payloadLong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, char event, long payload version): start"));

  //convert long to char
  char payloadChar[Numlen.numlen(payloadLong) + 1];
  ltoa(payloadLong, payloadChar, 10);

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const char event[], unsigned long payloadUlong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, char event, unsigned long payload version): start"));

  //convert unsigned long to char
  char payloadChar[Numlen.numlen(payloadUlong) + 1];
  ultoa(payloadUlong, payloadChar, 10);

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}


#ifdef __FLASH_H__
byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const char event[], _FLASH_STRING payloadFlashString, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, char event, _FLASH_STRING payload version): start"));

  //convert the _FLASH_STRING to char
  byte stringLength =  payloadFlashString.length();
  char payloadChar[stringLength + 1];
  payloadFlashString.copy(payloadChar, stringLength, 0);
  payloadChar[stringLength] = 0;

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}
#endif


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, int event, const char payload[], byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, int event, char payload version): start"));
  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payload, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, int event, const __FlashStringHelper* payloadFlashString, byte payloadLength, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, int event, F() payload version): start"));

  //convert __FlashStringHelper* to char
  char payloadChar[payloadLength + 1];  //Size array as needed.
  memcpy_P(payloadChar, payloadFlashString, payloadLength);
  payloadChar[payloadLength] = 0;  //null terminator

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, int event, int payloadInt, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, int event, int payload version): start"));

  //convert int to char
  char payloadChar[Numlen.numlen(payloadInt) + 1];
  itoa(payloadInt, payloadChar, 10);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, int event, unsigned int payloadUint, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, int event, unsigned int payload version): start"));

  //convert unsigned int to char
  char payloadChar[Numlen.numlen(payloadUint) + 1];
  sprintf_P(payloadChar, PSTR("%u"), payloadUint);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, int event, long payloadLong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, int event, long payload version): start"));

  //convert long to char
  char payloadChar[Numlen.numlen(payloadLong) + 1];
  ltoa(payloadLong, payloadChar, 10);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, int event, unsigned long payloadUlong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, int event, unsigned long payload version): start"));

  //convert unsigned long to char
  char payloadChar[Numlen.numlen(payloadUlong) + 1];
  ultoa(payloadUlong, payloadChar, 10);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}


#ifdef __FLASH_H__
byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, int event, _FLASH_STRING payloadFlashString, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, int event, _FLASH_STRING payload version): start"));

  //convert the _FLASH_STRING to char
  byte stringLength =  payloadFlashString.length();
  char payloadChar[stringLength + 1];
  payloadFlashString.copy(payloadChar, stringLength, 0);
  payloadChar[stringLength] = 0;

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, payloadChar, resendFlag);
}
#endif


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, int event, const char payload[], byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, int event, char payload version): start"));

  //convert event to char
  char eventChar[Numlen.numlen(event) + 1];
  itoa(event, eventChar, 10);

  return queue(targetIP, targetPort, eventChar, payload, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, int event, const __FlashStringHelper* payloadFlashString, byte payloadLength, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, int event, F() payload version): start"));

  //convert __FlashStringHelper* to char
  char payloadChar[payloadLength + 1];  //Size array as needed.
  memcpy_P(payloadChar, payloadFlashString, payloadLength);
  payloadChar[payloadLength] = 0;  //null terminator

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, int event, int payloadInt, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, int event, int payload version): start"));

  //convert int to char
  char payloadChar[Numlen.numlen(payloadInt) + 1];
  itoa(payloadInt, payloadChar, 10);

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, int event, unsigned int payloadUint, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, int event, unsigned int payload version): start"));

  //convert unsigned int to char
  char payloadChar[Numlen.numlen(payloadUint) + 1];
  sprintf_P(payloadChar, PSTR("%u"), payloadUint);

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, int event, long payloadLong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, int event, long payload version): start"));

  //convert long to char
  char payloadChar[Numlen.numlen(payloadLong) + 1];
  ltoa(payloadLong, payloadChar, 10);

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, int event, unsigned long payloadUlong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, int event, unsigned long payload version): start"));

  //convert unsigned long to char
  char payloadChar[Numlen.numlen(payloadUlong) + 1];
  ultoa(payloadUlong, payloadChar, 10);

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}


#ifdef __FLASH_H__
byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, int event, _FLASH_STRING payloadFlashString, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, int event, _FLASH_STRING payload version): start"));

  //convert the _FLASH_STRING to char
  byte stringLength =  payloadFlashString.length();
  char payloadChar[stringLength + 1];
  payloadFlashString.copy(payloadChar, stringLength, 0);
  payloadChar[stringLength] = 0;

  return queue(targetIP, targetPort, event, payloadChar, resendFlag);
}
#endif


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, const char payload[], byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, F() event, char payload version): start"));
  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, eventLength, payload, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, const __FlashStringHelper* payloadFlashString, byte payloadLength, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, __FlashStringHelper* event, F() payload version): start"));

  //convert __FlashStringHelper* to char
  char payloadChar[payloadLength + 1];  //Size array as needed.
  memcpy_P(payloadChar, payloadFlashString, payloadLength);
  payloadChar[payloadLength] = 0;  //null terminator

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, eventLength, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, int payloadInt, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, F() event, int payload version): start"));

  //convert int to char
  char payloadChar[Numlen.numlen(payloadInt) + 1];
  itoa(payloadInt, payloadChar, 10);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, eventLength, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, unsigned int payloadUint, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, F() event, unsigned int payload version): start"));

  //convert unsigned int to char
  char payloadChar[Numlen.numlen(payloadUint) + 1];
  sprintf_P(payloadChar, PSTR("%u"), payloadUint);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, eventLength, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, long payloadLong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, F() event, long payload version): start"));

  //convert long to char
  char payloadChar[Numlen.numlen(payloadLong) + 1];
  ltoa(payloadLong, payloadChar, 10);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, eventLength, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, unsigned long payloadUlong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, F() event, unsigned long payload version): start"));

  //convert unsigned long to char
  char payloadChar[Numlen.numlen(payloadUlong) + 1];
  ultoa(payloadUlong, payloadChar, 10);

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, eventLength, payloadChar, resendFlag);
}


#ifdef __FLASH_H__
byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, _FLASH_STRING payloadFlashString, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, F() event, _FLASH_STRING payload version): start"));

  //convert the _FLASH_STRING to char
  byte stringLength =  payloadFlashString.length();
  char payloadChar[stringLength + 1];
  payloadFlashString.copy(payloadChar, stringLength, 0);
  payloadChar[stringLength] = 0;

  return queue(EtherEventQueueNodes::nodeIP[targetNode], targetPort, event, eventLength, payloadChar, resendFlag);
}
#endif


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const __FlashStringHelper* eventFlashString, byte eventLength, const char payload[], byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, F() event, char payload version): start"));

  //convert __FlashStringHelper* to char
  char eventChar[eventLength + 1];  //Size array as needed.
  memcpy_P(eventChar, eventFlashString, eventLength);
  eventChar[eventLength] = 0;  //null terminator

  return queue(targetIP, targetPort, eventChar, payload, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, const __FlashStringHelper* payloadFlashString, byte payloadLength, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, F() event, F() payload version): start"));

  //convert __FlashStringHelper* to char
  char payloadChar[ payloadLength + 1 ];  //Size array as needed.
  memcpy_P(payloadChar, payloadFlashString, payloadLength);
  payloadChar[payloadLength] = 0;  //null terminator

  return queue(targetIP, targetPort, event, eventLength, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, int payloadInt, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, F() event, int payload version): start"));

  //convert int to char
  char payloadChar[Numlen.numlen(payloadInt) + 1];
  itoa(payloadInt, payloadChar, 10);

  return queue(targetIP, targetPort, event, eventLength, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, unsigned int payloadUint, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, F() event, unsigned int payload version): start"));

  //convert unsigned int to char
  char payloadChar[Numlen.numlen(payloadUint) + 1];
  sprintf_P(payloadChar, PSTR("%u"), payloadUint);

  return queue(targetIP, targetPort, event, eventLength, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, long payloadLong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, F() event, long payload version): start"));

  //convert long to char
  char payloadChar[Numlen.numlen(payloadLong) + 1];
  ltoa(payloadLong, payloadChar, 10);

  return queue(targetIP, targetPort, event, eventLength, payloadChar, resendFlag);
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, unsigned long payloadUlong, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, F() event, unsigned long payload version): start"));

  //convert unsigned long to char
  char payloadChar[Numlen.numlen(payloadUlong) + 1];
  ultoa(payloadUlong, payloadChar, 10);

  return queue(targetIP, targetPort, event, eventLength, payloadChar, resendFlag);
}


#ifdef __FLASH_H__
byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const __FlashStringHelper* event, byte eventLength, _FLASH_STRING payloadFlashString, byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(node, F() event, _FLASH_STRING payload version): start"));

  //convert the _FLASH_STRING to char
  byte stringLength =  payloadFlashString.length();
  char payloadChar[stringLength + 1];
  payloadFlashString.copy(payloadChar, stringLength, 0);
  payloadChar[stringLength] = 0;

  return queue(targetIP, targetPort, event, eventLength, payloadChar, resendFlag);
}
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//queue
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const char event[], const char payload[], byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(IP, char event, char payload version): start"));
  byte success = 0;
  int targetNode = getNode(targetIP);
  if (sendNodesOnly == 1 && targetNode < 0) {  //not a node
    Serial.println(F("EtherEventQueue.queue: not a node"));
    return 0;
  }
  if (targetNode == nodeDevice) {  //send events to self regardless of timeout state
    Serial.println(F("EtherEventQueue.queue: self send"));
    localEventQueueCount++;
  }
  else if (targetNode != nodeDevice && targetNode >= 0 && millis() - nodeTimestamp[targetNode] > nodeTimeout) {  //not self, is a node and is timed out
    Serial.println(F("EtherEventQueue.queue: timed out node"));
    return 0;  //don't queue events to timed out nodes
  }

  success = 1;  //indicate event successfully queued in return
  queueSize++;

  Serial.print(F("EtherEventQueue.queue: queueSize="));
  Serial.println(queueSize);
  if (queueSize > queueSizeMax) {  //queue overflowed
    queueSize = queueSizeMax;  //had to bump the first item in the queue because there's no room for it
    Serial.println(F("EtherEventQueue.queue: Queue Overflowed"));
    success = 2;  //indicate overflow in the return
    queueOverflowFlag = 1;
    if (getNode(IPqueue[queueSize - 1]) == nodeDevice) {  //the overflowed queue item is a local event
      localEventQueueCount--;
    }
    for (byte count = 0; count < queueSize - 1; count++) {  //shift all messages up the queue and add new item to queue. This is kind of similar to the ack received part where I removed the message from the queue so maybe it could be a function
      IPqueue[count] = IPqueue[count + 1];
      portQueue[count] = portQueue[(count + 1)];
      strcpy(eventQueue[count], eventQueue[count + 1]);
      eventIDqueue[count] = eventIDqueue[count + 1];
      strcpy(payloadQueue[count], payloadQueue[count + 1]);
      resendFlagQueue[count] = resendFlagQueue[count + 1];
    }
    Serial.print(F("EtherEventQueue.queue: post overflow queueSize="));
    Serial.println(queueSize);
  }

  //add the new message to the queue
  IPcopy(IPqueue[queueSize - 1], targetIP);
  portQueue[queueSize - 1] = targetPort;
  strcpy(eventQueue[queueSize - 1], event);
  eventIDqueue[queueSize - 1] = eventIDfind();
  strcpy(payloadQueue[queueSize - 1], payload);
  resendFlagQueue[queueSize - 1] = resendFlag;

  queueNewCount++;

  Serial.print(F("EtherEventQueue.queue: done, queueNewCount="));
  Serial.println(queueNewCount);
  Serial.print(F("EtherEventQueue.queue: IP="));
  Serial.println(IPAddress(IPqueue[queueSize - 1]));
  Serial.print(F("EtherEventQueue.queue: port="));
  Serial.println(portQueue[queueSize - 1]);
  Serial.print(F("EtherEventQueue.queue: event="));
  Serial.println(eventQueue[queueSize - 1]);
  Serial.print(F("EtherEventQueue.queue: payload="));
  Serial.println(payloadQueue[queueSize - 1]);
  Serial.print(F("EtherEventQueue.queue: eventID="));
  Serial.println(eventIDqueue[queueSize - 1]);
  Serial.print(F("EtherEventQueue.queue: resendFlag="));
  Serial.println(resendFlagQueue[queueSize - 1]);
  return success;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//queueHandler - sends out the messages in the queue
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::queueHandler(EthernetClient &ethernetClient) {
  if (queueSize > 0) {  //there are messages in the queue
    if (queueNewCount > 0 || millis() - queueSendTimestamp > resendDelay) {  //it is time
      if (queueNewCount > queueSize) {  //sanity check - if the acks get messed up this can happen
        queueNewCount = queueSize;
      }
      Serial.print(F("EtherEventQueue.queueHandler: queueSize="));
      Serial.println(queueSize);
      Serial.print(F("EtherEventQueue.queueHandler: queueNewCount="));
      Serial.println(queueNewCount);
      byte queueStepSend;  //this is used to store the step to send which may not be the current step because of sending the new messages first
      for (;;) {
        if (queueNewCount == 0) {  //time to send the next one in the queue
          queueSendTimestamp = millis();  //reset the timestamp to delay the next queue resend
          queueStep++;
          if (queueStep >= queueSize) {
            queueStep = 0;
          }
          queueStepSend = queueStep;
        }
        else {  //send new items in the queue immediately
          queueStepSend = queueSize - queueNewCount;  //send the oldest new one first
          queueNewCount--;
        }
        Serial.print(F("EtherEventQueue.queueHandler: queueStepSend="));
        Serial.println(queueStepSend);
        int targetNode = getNode(IPqueue[queueStepSend]);  //get the node of the target IP
        Serial.print(F("EtherEventQueue.queueHandler: targetNode="));
        Serial.println(targetNode);
        if (targetNode == nodeDevice) {  //ignore internal events, they are sent in availableEvent()
          continue;  //move on to the next queue step
        }
        if (targetNode < 0) {  //-1 indicates no node match
          Serial.println(F("EtherEventQueue.queueHandler: non-node targetIP"));
          break;  //non-nodes never timeout
        }

        if (millis() - nodeTimestamp[targetNode] < nodeTimeout || strcmp(eventQueue[queueStepSend], eventKeepalive) == 0) {  //non-timed out node or keepalive
          break;  //continue with the message send
        }
        Serial.print(F("EtherEventQueue.queueHandler: targetNode timed out for queue#="));
        Serial.println(queueStepSend);
        remove(queueStepSend);  //dump messages for dead nodes from the queue
        if (queueSize == 0) {  //no events left to send
          return;
        }
      }

      //set up the raw payload
      char eventID[3];
      itoa(eventIDqueue[queueStepSend], eventID, 10);  //put the message ID on the start of the payload
      char payload[strlen(payloadQueue[queueStepSend]) + eventIDlength + 1];
      strcpy(payload, eventID);
      strcat(payload, payloadQueue[queueStepSend]);  //add the true payload to the payload string

      Serial.print(F("EtherEventQueue.queueHandler: targetIP="));
      Serial.println(IPAddress(IPqueue[queueStepSend]));
      Serial.print(F("EtherEventQueue.queueHandler: event="));
      Serial.println(eventQueue[queueStepSend]);
      Serial.print(F("EtherEventQueue.queueHandler: payload="));
      Serial.println(payload);

      if (EtherEvent.send(ethernetClient, IPAddress(IPqueue[queueStepSend]), portQueue[queueStepSend], eventQueue[queueStepSend], payload) > 0) {
        Serial.println(F("EtherEventQueue.queueHandler: send successful"));
        if (resendFlagQueue[queueStepSend] < 2) {  //the flag indicates not to wait for an ack
          Serial.println(F("EtherEventQueue.queueHandler: resendFlag==0, event removed from queue"));
          remove(queueStepSend);  //remove the message from the queue immediately
        }
        return;
      }
      Serial.println(F("EtherEventQueue.queueHandler: send failed"));
      if (resendFlagQueue[queueStepSend] == 0) {  //the flag indicates not to resend even after failure
        remove(queueStepSend);  //remove keepalives even when send was not successful. This is because the keepalives are sent even to timed out nodes so they shouldn't be queued.
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//flushQueue - removes all events from the queue
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::flushQueue() {
  Serial.println(F("EtherEventQueue.flushQueue: start"));
  queueSize = 0;
  queueNewCount = 0;
  localEventQueueCount = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//checkTimeout - checks all the nodes until it finds a _NEWLY_ timed out node and returns it and then updates the nodeState value for that node. If no nodes are newly timed out then this function returns -1.  Note that this works differently than checkState()
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int8_t EtherEventQueueClass::checkTimeout() {
  for (byte node = 0; node < nodeCount; node++) {
    if (nodeState[node] == 1 && millis() - nodeTimestamp[node] > nodeTimeout) {  //previous state not timed out, and is currently timed out
      Serial.print(F("EtherEventQueue.checkTimeout: timed out node="));
      Serial.println(node);
      nodeState[node] = 0;  //0 indicates the node is timed out
      return node;
    }
  }
  return -1;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//checkTimein - checks all the authorized IPs until it finds a _NEWLY_ timed in node and returns it and then updates the nodeState value for that node. If no nodes are newly timed in then this function returns -1.  Note that this works differently than checkState()
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int8_t EtherEventQueueClass::checkTimein() {
  for (byte node = 0; node < nodeCount; node++) {
    if (nodeState[node] == 0 && millis() - nodeTimestamp[node] < nodeTimeout) {  //node is newly timed out(since the last time the function was run)
      Serial.print(F("EtherEventQueue.checkTimein: timed in node="));
      Serial.println(node);
      nodeState[node] = 1;  //1 indicates the node is not timed out
      return node;
    }
  }
  return -1;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//checkState - checks if the given node is timed out. Note that this doesn't update the nodeState like checkTimeout()/checkTimein().
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean EtherEventQueueClass::checkState(byte node) {
  Serial.print(F("EtherEventQueue.checkTimeoutNode: nodeState for node "));
  Serial.print(node);
  Serial.print(F("="));
  if (nodeTimestamp[node] > nodeTimeout) {  //node is not this device, not already timed out, and is timed out
    Serial.println(F("timed out"));
    return 0;
  }
  Serial.println(F("not timed out"));
  return 1;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//checkQueueOverflow
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean EtherEventQueueClass::checkQueueOverflow() {
  byte queueOverflowFlagValue = queueOverflowFlag; //save the value before resetting it
  queueOverflowFlag = false;  //reset the flag
  return queueOverflowFlagValue;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//private functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//eventIDfind - find a free eventID
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte EtherEventQueueClass::eventIDfind() {
  Serial.println(F("EtherEventQueue.eventIDfind: start"));
  if (queueSize == 0) {  //the queue is empty
    Serial.println(F("EtherEventQueue.eventIDfind: eventID=10"));
    return 10;  //default value if there are no other messages
  }
  if (queueSize > 0) {
    for (byte eventID = 10; eventID <= 99; eventID++) {  //step through all possible eventIDs. They start at 10 so they will always be 2 digit
      byte eventIDduplicate = 0;
      for (byte count = 0; count < queueSize; count++) {  //step through the currently occupied section of the eventIDqueue[]
        if (eventID == eventIDqueue[count]) {  //the eventID is already being used
          eventIDduplicate = 1;
        }
      }
      if (eventIDduplicate == 0) {  //the eventID was unique
        Serial.print(F("EtherEventQueue.eventIDfind: eventID="));
        Serial.println(eventID);
        return eventID;  //skip the rest of the for loop
      }
    }
  }
  return 0;  //this should never happen but it causes a compiler warning without
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//remove - remove the given item from the queue
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::remove(byte queueStep) {
  if (queueSize > 0) {
    queueSize--;
  }
  Serial.print(F("EtherEventQueue.remove: new queue size="));
  Serial.println(queueSize);
  if (queueSize > 0) {  //if the only message in the queue is being removed then it doesn't need to adjust the queue
    for (byte count = queueStep; count < queueSize; count++) {  //move all the messages above the one to remove up in the queue
      IPqueue[count] = IPqueue[count + 1];  //set the target for the message in the queue
      portQueue[count] = portQueue[count + 1];
      strcpy(eventQueue[count], eventQueue[count + 1]);
      eventIDqueue[count] = eventIDqueue[count + 1];
      strcpy(payloadQueue[count], payloadQueue[count + 1]);
      resendFlagQueue[count] = resendFlagQueue[count + 1];
    }
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//IPcopy - converts and copies IPAddress to 4 byte array - IPdestination will contain the converted IPsource after this function is called
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::IPcopy(byte IPdestination[], const IPAddress IPsource) {
  for (byte counter = 0; counter < 4; counter++) {
    IPdestination[counter] = IPsource[counter];
  }
}

EtherEventQueueClass EtherEventQueue;  //This sets up a single global instance of the library so the class doesn't need to be declared in the user sketch and multiple instances are not necessary in this case.
