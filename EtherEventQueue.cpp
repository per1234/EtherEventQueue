// EtherEventQueue outgoing event queue for the EtherEvent authenticated network communication arduino library: http://github.com/per1234/EtherEvent
#include <Arduino.h>
#include "EtherEventQueue.h"  //http://github.com/per1234/EtherEventQueue
#include <SPI.h>  //for the ethernet library
#include <Ethernet.h>
#include "MD5.h"  //for etherEvent authentication
//#include "Entropy.h"  //Uncomment if you have installed the Entropy library for true random numbers for the EtherEvent authentication process
#include "EtherEvent.h"  //http://github.com/per1234/EtherEvent


//this will disable the compiler warning for F()
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

//configuration:
#define DEBUG 0 // (0==serial debug output off, 1==serial debug output on)The serial debug output will greatly increase communication time
#define Serial if(DEBUG)Serial

const boolean receiveNodesOnly = 0; //restrict event receiving to nodes only
const boolean sendNodesOnly = 0; //restrict event sending to nodes only

const IPAddress nodeIP[] = { //IP addresses on the network, this can be used to filter IPs or to monitor the IPs for timeout/timein and optimize the queue based on this information
  IPAddress(192, 168, 69, 100),
  IPAddress(192, 168, 69, 101),
  IPAddress(192, 168, 69, 102),
  IPAddress(192, 168, 69, 103),
  IPAddress(192, 168, 69, 104),
  IPAddress(192, 168, 69, 105),
  IPAddress(192, 168, 69, 106),
  IPAddress(192, 168, 69, 107),
  IPAddress(192, 168, 69, 108),
  IPAddress(192, 168, 69, 109),
  IPAddress(192, 168, 69, 110)
};

const unsigned long nodeTimeout = 270000; //(ms)the node is timed out if it has been longer than this duration since the last event was received from it
const unsigned long nodeTimeoutSelf = 200000; //(ms)the device is timed out if it has been longer than this duration since any event was received

const char eventPing[] = "100"; //the library handles these special events differently
const char eventAck[] = "101";
const unsigned int resendDelay = 30000; //(ms)delay between resends of messages

IPAddress receivedIP;
IPAddress IPqueue[EtherEventQueue_queueSizeMax];  //queue buffers

void EtherEventQueueClass::begin(byte nodeDeviceValue, unsigned int portValue) {
  EtherEventQueue_nodeDevice = nodeDeviceValue;
  port = portValue;
  //TODO:size the node related buffers= sizeof(nodeIP)/sizeof(IPAddress)
}


byte EtherEventQueueClass::availableEvent(EthernetServer &ethernetServer) {
  if (byte length = strlen(receivedEvent)) { //there is already a previously received event buffered
    Serial.println(F("EtherEventQueue.availableEvent: previously received event"));
    return length + 1; //number of bytes including the null terminator remaining to be read of the event
  }
  if (availablePayload() > 0) { //don't get another event until the last is fully read or flushed
    return 0;
  }

  for (int queueStepCount = queueSize - 1; queueStepCount >= 0; queueStepCount--) { //internal event system: step through the queue from the newest to oldest
    if (getNode(IPqueue[queueStepCount]) == EtherEventQueue_nodeDevice) { //internal event
      Serial.print(F("EtherEventQueue.availableEvent: internal event="));
      strcpy(receivedEvent, eventQueue[queueStepCount]);
      Serial.println(receivedEvent);
      strcpy(receivedPayload, payloadQueue[queueStepCount]);
      Serial.print(F("EtherEventQueue.availableEvent: internal event payload="));
      Serial.println(receivedPayload);
      Serial.print(F("EtherEventQueue.availableEvent: nodeIP[EtherEventQueue_nodeDevice]="));
      Serial.println(nodeIP[EtherEventQueue_nodeDevice]);
      receivedIP = nodeIP[EtherEventQueue_nodeDevice];
      remove(queueStepCount);  //remove the event from the queue
      queueNewCount--;
      return strlen(receivedEvent);
    }
  }

  if (byte availableBytesEvent = EtherEvent.availableEvent(ethernetServer)) { //there is a new event
    Serial.println(F("---------------------------"));
    Serial.print(F("EtherEventQueue.availableEvent: EtherEvent.availableEvent()="));
    Serial.println(availableBytesEvent);
    receivedIP = EtherEvent.senderIP();
    Serial.print(F("EtherEventQueue.availableEvent: remoteIP="));
    Serial.println(receivedIP);

    nodeTimestamp[EtherEventQueue_nodeDevice] = millis(); //set the general ping timestamp(using the EtherEventQueue_nodeDevice because that part of the array is never used otherwise)
    //update timed out status of the event sender
    byte senderNode = getNode(receivedIP); //get the node of the senderIP
    if (senderNode >= 0) { //receivedIP is a node(-1 indicates no node match)
      nodeTimestamp[senderNode] = millis(); //set the individual timestamp, any communication is considered to be a ping
    }
    else if (receiveNodesOnly == 1) { //receive events from node IPs only
      Serial.println(F("EtherEventQueue.availableEvent: unauthorized IP"));
      EtherEvent.flushReceiver();  //event and payload have not been read yet so only have to flush EtherEvent
      return 0;
    }

    EtherEvent.readEvent(receivedEvent);  //put the event in the buffer
    Serial.print(F("EtherEventQueue.availableEvent: ethernetReadEvent="));
    Serial.println(receivedEvent);

    if (strcmp(receivedEvent, eventPing) == 0) { //ping received
      Serial.println(F("EtherEventQueue.availableEvent: ping received"));
      EtherEvent.flushReceiver();  //the payload has not been read yet so EtherEvent has to be flushed
      flushReceiver();  //the event has been read so EtherEventQueue has to be flushed
      return 0;  //receive ping silently
    }

    byte payloadLength = EtherEvent.availablePayload();
    Serial.print(F("EtherEventQueue.availableEvent: EtherEvent.availablePayload()="));
    Serial.println(payloadLength);
    char receivedPayloadRaw[payloadLength];  //(TODO: get rid of this, just use the regular payload buffer)initialize raw payload buffer
    EtherEvent.readPayload(receivedPayloadRaw);  //read the payload to the buffer
    Serial.print(F("EtherEventQueue.availableEvent: rawPayload="));
    Serial.println(receivedPayloadRaw);

    //break the payload down into parts and convert the eventID, code, and target address to byte, the true payload stays as a char array
    char receivedeventID[EtherEventQueue_eventIDlength + 1];
    for (byte count = 0; count < EtherEventQueue_eventIDlength; count++) {
      receivedeventID[count] = receivedPayloadRaw[count];
    }
    receivedeventID[EtherEventQueue_eventIDlength] = 0; //add the null terminator because there is not one after the id in the string
    Serial.print(F("EtherEventQueue.availableEvent: ethernetReadeventID="));
    Serial.println(receivedeventID);

    if (payloadLength > EtherEventQueue_eventIDlength + 1) { //there is a true payload
      for (byte count = 0; count < payloadLength - EtherEventQueue_eventIDlength; count++) {
        receivedPayload[count] = receivedPayloadRaw[count + EtherEventQueue_eventIDlength]; //(TODO: just use receivedPayload for the buffer instead of having the raw buffer)
      }
      Serial.print(F("EtherEventQueue.availableEvent: receivedPayload="));
      Serial.println(receivedPayload);
    }

    if (strcmp(receivedEvent, eventAck) == 0) { //ack handler
      Serial.println(F("EtherEventQueue.availableEvent: ack received"));
      byte receivedPayloadInt = atoi(receivedPayload); //convert to a byte
      for (byte count = 0; count < queueSize; count++) { //step through the currently occupied section of the eventIDqueue[]
        if (receivedPayloadInt == eventIDqueue[count]) { //this is the message the ack is for
          Serial.println(F("EtherEventQueue.availableEvent: ack eventID match"));
          remove(count);  //remove the message from the queue
        }
      }
      flushReceiver();  //event and payload have been read so only have to flush EtherEventQueue
      return 0;  //receive ack silently
    }

    Serial.println(F("EtherEventQueue.availableEvent: send ack"));
    queue(receivedIP, port, eventAck, receivedeventID, 0);  //send the ack - the eventID of the received message is the payload

    return availableBytesEvent;  //there is a new event, return the number of bytes
  }
  return 0;
}


byte EtherEventQueueClass::availablePayload() { //returns the number of chars in the payload including the null terminator if there is one
  if (byte length = strlen(receivedPayload)) { //strlen(receivedPayload)>0
    return length + 1; //length of the payload + null terminator
  }
  return 0;
}


void EtherEventQueueClass::readEvent(char eventBuffer[]) {
  Serial.println(F("EtherEventQueue.readEvent: start"));
  strcpy(eventBuffer, receivedEvent);
  eventBuffer[strlen(receivedEvent)] = 0; //null terminator - is this needed?
  receivedEvent[0] = 0; //reset the event buffer
}


void EtherEventQueueClass::readPayload(char payloadBuffer[]) {
  Serial.println(F("EtherEventQueue.readPayload: start"));
  strcpy(payloadBuffer, receivedPayload);
  payloadBuffer[strlen(receivedPayload)] = 0; //null terminator - is this needed?
  receivedPayload[0] = 0; //reset the payload buffer
}


IPAddress EtherEventQueueClass::senderIP() {
  return receivedIP;
}


void EtherEventQueueClass::flushReceiver() { //dump the last message received so another one can be received
  Serial.println(F("EtherEventQueue.flushReceiver: start"));
  receivedEvent[0] = 0; //reset the event buffer
  receivedPayload[0] = 0; //reset the payload buffer
}


byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const char event[], const char payload[], boolean resendFlag) { //add the relayed outgoing message to the send queue. Returns: 0==fail, 1==success, 2==success w/ queue overflow - this version takes node number and converts to IP
  Serial.println(F("EtherEventQueue.queue(node version): start"));
  return queue(nodeIP[targetNode], targetPort, event, payload, resendFlag);
}

byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const char event[], const char payload[], boolean resendFlag) { //add the relayed outgoing message to the send queue. Returns: 0==fail, 1==success, 2==success w/ queue overflow
  Serial.println(F("---------------------------"));
  Serial.println(F("EtherEventQueue.queue(IP version): start"));
  byte success = 0;
  int targetNode = getNode(targetIP);
  if (sendNodesOnly == 1 && targetNode < 0) { //not a node
    Serial.println(F("EtherEventQueue.queue: not a node"));
    return 0;
  }
  if (targetNode == EtherEventQueue_nodeDevice) { //send events to self regardless of timeout state
    Serial.println(F("EtherEventQueue.queue: self send"));
  }
  else if (targetNode != EtherEventQueue_nodeDevice && targetNode >= 0 && millis() - nodeTimestamp[targetNode] > nodeTimeout) { //not self, is a node and is timed out
    Serial.println(F("EtherEventQueue.queue: timed out node"));
    return 0;  //don't queue events to timed out nodes
  }

  success = 1; //indicate event successfully queued in return
  byte eventID = eventIDfind(); //get an event ID
  queueSize++;
  Serial.println(F("EtherEventQueue.queue: new size="));
  if (queueSize > EtherEventQueue_queueSizeMax) { //queue overflowed
    queueSize = EtherEventQueue_queueSizeMax; //had to bump the first item in the queue because there's no room for it
    Serial.println(F("EtherEventQueue.queue: Queue Overflowed"));  //I don't really want to send another message into the queue because that will recursive loop
    success = 2; //indicate overflow in the return
    for (byte count = 0; count < queueSize - 1; count++) { //shift all messages up the queue and add new item to queue. This is kind of similar to the ack received part where I removed the message from the queue so maybe it could be a function
      IPqueue[count] = IPqueue[count + 1];
      portQueue[count] = portQueue[count + 1];
      strcpy(eventQueue[count], eventQueue[count + 1]);
      eventIDqueue[count] = eventIDqueue[count + 1];
      strcpy(payloadQueue[count], payloadQueue[count + 1]);
      resendFlagQueue[count] = resendFlagQueue[count + 1];
    }
    Serial.print(F("EtherEventQueue.queue: new size="));
    Serial.println(queueSize);
  }

  //add the new message to the queue
  IPqueue[queueSize - 1][0] = targetIP[0]; //put the new code in the array after the most recent entry
  IPqueue[queueSize - 1][1] = targetIP[1]; //put the new code in the array after the most recent entry
  IPqueue[queueSize - 1][2] = targetIP[2]; //put the new code in the array after the most recent entry
  IPqueue[queueSize - 1][3] = targetIP[3]; //put the new code in the array after the most recent entry
  portQueue[queueSize - 1] = targetPort;
  strcpy(eventQueue[queueSize - 1], event); //set the eventID for the message in the queue
  eventIDqueue[queueSize - 1] = eventID;
  strcpy(payloadQueue[queueSize - 1], payload); //put the new payload in the queue
  resendFlagQueue[queueSize - 1] = resendFlag;

  queueNewCount++;

  Serial.print(F("EtherEventQueue.queue: done, queueNewCount="));
  Serial.println(queueNewCount);
  Serial.print(F("EtherEventQueue.queue: IP="));
  Serial.println(IPqueue[queueSize - 1]);
  Serial.print(F("EtherEventQueue.queue: port="));
  Serial.println(portQueue[queueSize - 1]);
  Serial.print(F("EtherEventQueue.queue: event="));
  Serial.println(eventQueue[queueSize - 1]);
  Serial.print(F("EtherEventQueue.queue: eventID="));
  Serial.println(eventIDqueue[queueSize - 1]);
  Serial.print(F("EtherEventQueue.queue: resendFlag="));
  Serial.println(resendFlagQueue[queueSize - 1]);

  return success;
}


void EtherEventQueueClass::queueHandler(EthernetClient &ethernetClient) { //ethernetQueueHandler - sends out the messages in the queue-------------
  if (queueSize > 0) { //there are messages in the queue
    if (queueNewCount > 0 || millis() - queueSendTimestamp > resendDelay) { //it is time
      Serial.print(F("EtherEventQueue.queueHandler: queueSize="));
      Serial.println(queueSize);
      Serial.print(F("EtherEventQueue.queueHandler: queueNewCount="));
      Serial.println(queueNewCount);
      byte queueStepSend;  //this is used to store the step to send which may not be the current step because of sending the new messages first
      for (;;) {
        if (queueNewCount == 0) { //time to send the next one in the queue
          queueSendTimestamp = millis(); //reset the timestamp to delay the next queue resend
          queueStep++;
          if (queueStep >= queueSize) {
            queueStep = 0;
          }
          queueStepSend = queueStep;
        }
        else if (queueNewCount > 0) { //send the new items in the queue immediately
          queueStepSend = queueSize - queueNewCount; //send the oldest new one first
          queueNewCount--;
        }
        Serial.print(F("EtherEventQueue.queueHandler: queueStepSend="));
        Serial.println(queueStepSend);
        int targetNode = getNode(IPqueue[queueStepSend]); //get the node of the target IP
        Serial.print(F("EtherEventQueue.queueHandler: targetNode="));
        Serial.println(targetNode);
        if (targetNode == EtherEventQueue_nodeDevice) { //ignore internal events
          continue;  //move on to the next queue step
        }
        if (targetNode < 0) { //-1 indicates no node match
          Serial.println(F("EtherEventQueue.queueHandler: non-node targetIP"));
          break;  //non-nodes never timeout
        }

        if (millis() - nodeTimestamp[targetNode] < nodeTimeout || strcmp(eventQueue[queueStepSend], eventPing) == 0) { //non-timed out node or ping
          break;  //continue with the message send
        }
        Serial.print(F("EtherEventQueue.queueHandler: targetNode timed out for queue#="));
        Serial.println(queueStepSend);
        remove(queueStepSend);  //dump messages for dead nodes from the queue
        if (queueSize == 0) { //no events left to send
          return;
        }
      }

      //set up the raw payload
      char eventID[3];
      itoa(eventIDqueue[queueStepSend], eventID, 10); //put the message ID on the start of the payload
      char payload[strlen(payloadQueue[queueStepSend]) + EtherEventQueue_eventIDlength + 1];
      strcpy(payload, eventID);
      strcat(payload, payloadQueue[queueStepSend]); //add the true payload to the payload string

      Serial.print(F("EtherEventQueue.queueHandler: targetIP="));
      Serial.println(IPqueue[queueStepSend]);
      Serial.print(F("EtherEventQueue.queueHandler: event="));
      Serial.println(eventQueue[queueStepSend]);
      Serial.print(F("EtherEventQueue.queueHandler: payload="));
      Serial.println(payload);

      if (EtherEvent.send(ethernetClient, IPqueue[queueStepSend], portQueue[queueStepSend], eventQueue[queueStepSend], payload) > 0) {
        Serial.println(F("EtherEventQueue.queueHandler: send successful"));
        if (resendFlagQueue[queueStepSend] == 0) {
          Serial.println(F("EtherEventQueue.queueHandler: resendFlag==0, event removed from queue"));
          remove(queueStepSend);  //remove the message from the queue immediately
        }
        return;
      }
      Serial.println(F("EtherEventQueue.queueHandler: send failed"));
    }
  }
}


void EtherEventQueueClass::flushQueue() {
  Serial.println(F("EtherEventQueue.flushQueue: start"));
  queueSize = 0;
  queueNewCount = 0;
}


int EtherEventQueueClass::checkTimeout() { //checks all the nodes until it finds a _NEWLY_ timed out node and returns it or none and returns -1.  Note that this works differently than checkTimeoutSelf
  for (byte node = 0; node < sizeof(nodeIP) / sizeof(IPAddress); node++) {
    if (nodeState[node] == 0 && millis() - nodeTimestamp[node] > nodeTimeout) { //node is not this device, not already timed out, and is timed out
      Serial.print(F("EtherEventQueue.checkTimeout: timed out node="));
      Serial.println(node);
      nodeState[node] = 1; //1 indicates the node is timed out
      return node;
    }
  }
  return -1;
}


int EtherEventQueueClass::checkTimein() { //checks all the authorized IPs until it finds a _NEWLY_ timed out node and returns it or none and returns -1.  Note that this works differently than checkTimeoutSelf
  for (byte node = 0; node < sizeof(nodeIP) / sizeof(IPAddress); node++) {
    if (nodeState[node] == 1 && millis() - nodeTimestamp[node] < nodeTimeout) { //node is newly timed out(since the last time the function was run)
      Serial.print(F("EtherEventQueue.checkTimein: timed out node="));
      Serial.println(node);
      nodeState[node] = 0; //1 indicates the node is timed out
      return node;
    }
  }
  return -1;
}


boolean EtherEventQueueClass::checkState(byte node) { //checks if the given node is timed out. Note that this doesn't update the nodeState.
  Serial.print(F("EtherEventQueue.checkTimeoutNode: nodeState for node "));
  Serial.print(node);
  Serial.print(F("="));
  if (nodeTimestamp[node] > nodeTimeout) { //node is not this device, not already timed out, and is timed out
    Serial.println(F("timed out"));
    return 1;
  }
  Serial.println(F("not timed out"));
  return 0;
}


//----------------private functions------------------------------

int EtherEventQueueClass::getNode(const IPAddress IPvalue) {
  Serial.println(F("EtherEventQueue.getNode: start"));
  for (byte node = 0; node < sizeof(nodeIP) / sizeof(IPAddress); node++) { //step through all the nodes
    byte octet;
    for (octet = 0; octet < 4; octet++) {
      if (nodeIP[node][octet] != IPvalue[octet]) { //mismatch
        octet = 0;
        break;
      }
    }
    if (octet == 4) { //match
      Serial.print(F("EtherEventQueue.getNode: node found="));
      Serial.println(node);
      return node;
    }
  }
  Serial.println(F("EtherEventQueue.getNode: node not found"));
  return -1;  //no match
}


byte EtherEventQueueClass::eventIDfind() { //find a free eventID - this is similar to the eventID check in the ack received code so maybe make a function
  Serial.println(F("EtherEventQueue.eventIDfind: start"));
  if (queueSize == 0) { //the queue is empty
    Serial.println(F("EtherEventQueue.eventIDfind: eventID=10"));
    return 10;  //default value if there are no other messages
  }
  if (queueSize > 0) {
    for (byte eventID = 10; eventID < EtherEventQueue_queueSizeMax + 10; eventID++) { //step through all possible eventIDs. They start at 10 so they will always be 2 digit
      byte eventIDduplicate = 0;
      for (byte count = 0; count < queueSize; count++) { //step through the currently occupied section of the eventIDqueue[]
        if (eventID == eventIDqueue[count]) { //the eventID is already being used
          eventIDduplicate = 1;
        }
      }
      if (eventIDduplicate == 0) { //the eventID was unique
        Serial.print(F("EtherEventQueue.eventIDfind: eventID="));
        Serial.println(eventID);
        return eventID;  //skip the rest of the for loop
      }
    }
  }
  return 0;  //this should never happen but it causes a compiler warning without
}


void EtherEventQueueClass::remove(byte queueStep) { //remove the given item from the queue
  if (queueSize > 0) {
    queueSize--;
  }
  Serial.print(F("EtherEventQueue.remove: new queue size="));
  Serial.println(queueSize);
  if (queueSize > 0) { //if this is the only message in the queue being removed then it doesn't need to adjust the queue
    for (byte count = queueStep; count < queueSize - 1; count++) { //move all the messages above the one to remove up in the queue
      IPqueue[count] = IPqueue[count + 1]; //set the target for the message in the queue
      portQueue[count] = portQueue[count + 1];
      strcpy(eventQueue[count], eventQueue[count + 1]);
      eventIDqueue[count] = eventIDqueue[count + 1];
      strcpy(payloadQueue[count], payloadQueue[count + 1]);
      resendFlagQueue[count] = resendFlagQueue[count + 1];
    }
  }
}


EtherEventQueueClass EtherEventQueue; //This sets up a single global instance of the library so the class doesn't need to be declared in the user sketch and multiple instances are not necessary in this case.
