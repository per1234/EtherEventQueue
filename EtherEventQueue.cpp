//EtherEventQueue outgoing event queue for the EtherEvent authenticated network communication arduino library: http://github.com/per1234/EtherEvent
#include <Arduino.h>
#include "EtherEventQueue.h"
#include <SPI.h>  //for the ethernet library
#include "Ethernet.h"
#include "EtherEvent.h"
//#include "Flash.h"  //uncomment this line if you have the Flash library installed

#define DEBUG false  //(false == serial debug output off,  true == serial debug output on)The serial debug output will increase memory usage and communication latency so only enable when in use.
#define Serial if(DEBUG)Serial

const unsigned long nodeTimeoutDurationDefault = 270000;  //(ms)the node is timed out if it has been longer than this duration since the last event was received from it
const unsigned long sendKeepaliveMarginDefault = nodeTimeoutDurationDefault - 30000;
const unsigned long sendKeepaliveResendDelayDefault = 60000;
const unsigned int resendDelayDefault = 45000;  //(ms)delay between resends of messages
const byte eventIDlength = 2;  //number of characters of the message ID that is appended to the start of the raw payload, the event ID must be exactly this length

const byte queueSizeMaxDefault = 5;
const byte eventLengthMaxDefault = 15;
const byte payloadLengthMaxDefault = 80;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//constructor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
EtherEventQueueClass::EtherEventQueueClass() {
  nodeTimeoutDuration = nodeTimeoutDurationDefault;
  sendKeepaliveMargin = sendKeepaliveMarginDefault;
  sendKeepaliveResendDelay = sendKeepaliveResendDelayDefault;
  resendDelay = resendDelayDefault;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//begin
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean EtherEventQueueClass::begin() {  //no nodes, default buffer length version - the deviceNode is 0
  return begin(0, 1, queueSizeMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault);
}


boolean EtherEventQueueClass::begin(byte queueSizeMaxInput, byte sendEventLengthMaxInput, byte sendPayloadLengthMaxInput, byte receivedEventLengthMaxInput, byte receivedPayloadLengthMaxInput) {  //no nodes version - the deviceNode is 0
  return begin(0, 1, queueSizeMaxInput, sendEventLengthMaxInput, sendPayloadLengthMaxInput, receivedEventLengthMaxInput, receivedPayloadLengthMaxInput);
}


boolean EtherEventQueueClass::begin(byte nodeDeviceInput, byte nodeCountInput) {  //default buffer length version - the deviceNode is 0
  return begin(nodeDeviceInput, nodeCountInput, queueSizeMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault);
}


boolean EtherEventQueueClass::begin(byte nodeDeviceInput, byte nodeCountInput, byte queueSizeMaxInput, byte sendEventLengthMaxInput, byte sendPayloadLengthMaxInput, byte receivedEventLengthMaxInput, byte receivedPayloadLengthMaxInput) {
#if DEBUG == true
  delay(20);  //There needs to be a delay between the calls to Serial.begin() in sketch setup() and here or garbage will be printed to the serial monitor
#endif
  Serial.begin(9600);  //for debugging
  Serial.println(F("\n\n\nEtherEventQueue.begin"));
  nodeDevice = nodeDeviceInput;
  nodeCountInput = max(nodeDevice + 1, nodeCountInput); //the nodeCount has to be enough to hold the device node number
  for (byte counter = 0; counter < nodeCount; counter++) {  //free previously allocated array items - this has to be done for arrays only because realloc doesn't work with the array items
    free(nodeIP[counter]);
  }
  nodeIP = (byte**)realloc(nodeIP, nodeCountInput * sizeof(byte*));  //have to use 4 byte arrays for the IP addresses instead of IPAddress because I can't get IPAddress to work with malloc
  for (byte nodeCounter = 0; nodeCounter < nodeCountInput; nodeCounter++) {
    nodeIP[nodeCounter] = (byte*)malloc(4 * sizeof(byte));  //4 bytes/IP address
    //zero initialize the IP Address - this will indicate that the node has not yet been configured
    for (byte counter = 0; counter < 4; counter++) {
      nodeIP[nodeCounter][counter] = 0;
    }
  }

  nodeState = (byte*)realloc(nodeState, nodeCountInput * sizeof(byte));
  nodeTimestamp = (unsigned long*)realloc(nodeTimestamp, nodeCountInput * sizeof(unsigned long));
  sendKeepaliveTimestamp = (unsigned long*)realloc(sendKeepaliveTimestamp, nodeCountInput * sizeof(unsigned long));
  nodeCount = max(nodeDevice + 1, nodeCountInput); //set this after the buffers have been realloced so that the old value can be used for free()ing the array items

  setNode(nodeDeviceInput, Ethernet.localIP());  //configure the device node

  //buffer sizing - these are dynamically allocated so that the sized can be set via the API
  //size send event queue buffers
  queueSizeMaxInput = min(queueSizeMaxInput, 90);  //the current system uses a 2 digit messageID so the range is 10-99, this restricts the queueSizeMax <= 90

  for (byte counter = 0; counter < queueSizeMax; counter++) {  //free previously allocated array items - this has to be done for arrays only because realloc doesn't work with the array items
    free(IPqueue[counter]);
  }
  IPqueue = (byte**)realloc(IPqueue, queueSizeMaxInput * sizeof(byte*));  //have to use 4 byte arrays for the IP addresses instead of IPAddress because I can't get IPAddress to work with malloc
  for (byte counter = 0; counter < queueSizeMaxInput; counter++) {
    IPqueue[counter] = (byte*)malloc(4 * sizeof(byte));  //4 bytes/IP address
  }

  portQueue = (unsigned int*)realloc(portQueue, queueSizeMaxInput * sizeof(unsigned int));

  for (byte counter = 0; counter < queueSizeMax; counter++) {
    free(eventQueue[counter]);
  }
  eventQueue = (char**)realloc(eventQueue, queueSizeMaxInput * sizeof(char*));
  sendEventLengthMax = sendEventLengthMaxInput;
  for (byte counter = 0; counter < queueSizeMaxInput; counter++) {
    eventQueue[counter] = (char*)malloc((sendEventLengthMax + 1) * sizeof(char));
  }

  eventIDqueue = (byte*)realloc(eventIDqueue, queueSizeMaxInput * sizeof(byte));

  for (byte counter = 0; counter < queueSizeMax; counter++) {
    free(payloadQueue[counter]);
  }
  payloadQueue = (char**)realloc(payloadQueue, queueSizeMaxInput * sizeof(char*));
  sendPayloadLengthMax = sendPayloadLengthMaxInput;
  for (byte counter = 0; counter < queueSizeMaxInput; counter++) {
    payloadQueue[counter] = (char*)malloc((sendPayloadLengthMax + 1) * sizeof(char));
  }

  resendFlagQueue = (byte*)realloc(resendFlagQueue, queueSizeMaxInput * sizeof(byte));

  queueSizeMax = queueSizeMaxInput;  //save the new queueSizeMax, this is done at the end of begin() because it needs to remember the previous value for freeing the array items

  //size received event buffers
  receivedEventLengthMax = receivedEventLengthMaxInput;
  receivedEvent = (char*)realloc(receivedEvent, (receivedEventLengthMax + 1) * sizeof(char));
  receivedEvent[0] = 0;  //clear buffer - realloc does not zero initialize so the buffer could contain anything
  receivedPayloadLengthMax = receivedPayloadLengthMaxInput;
  receivedPayload = (char*)realloc(receivedPayload, (receivedPayloadLengthMax + 1) * sizeof(char));
  receivedPayload[0] = 0;  //clear buffer - realloc does not zero initialize so the buffer could contain anything
  if (IPqueue == NULL || portQueue == NULL || eventQueue == NULL || eventIDqueue == NULL || payloadQueue == NULL || resendFlagQueue == NULL || receivedEvent == NULL || receivedPayload == NULL || EtherEvent.begin(receivedEventLengthMax, eventIDlength + receivedPayloadLengthMax) == false) {
    Serial.println(F("memory allocation failed"));
    return false;
  }
  return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//availableEvent - check for new incoming events, process and buffer them and return the length of the event string
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte EtherEventQueueClass::availableEvent(EthernetServer &ethernetServer) {
  if (receivedEventLength == 0) {  //there is no event buffered
    if (localEventQueueCount > 0) {
      for (int8_t queueStepCount = queueSize - 1; queueStepCount >= 0; queueStepCount--) {  //internal event system: step through the queue from the newest to oldest
        if (getNode(IPqueue[queueStepCount]) == nodeDevice) {  //internal event
          strcpy(receivedEvent, eventQueue[queueStepCount]);
          Serial.print(F("EtherEventQueue.availableEvent: internal event="));
          Serial.println(receivedEvent);
          strcpy(receivedPayload, payloadQueue[queueStepCount]);
          Serial.print(F("EtherEventQueue.availableEvent: internal event payload="));
          Serial.println(receivedPayload);
#ifdef ethernetclientwithremoteIP_h  //this function is only available if the modified Ethernet library is installed
          receivedIP = IPAddress(IPqueue[queueStepCount]);
          Serial.print(F("EtherEventQueue.availableEvent: receivedIP="));
          Serial.println(receivedIP);
#endif
          remove(queueStepCount);  //remove the event from the queue
          queueNewCount--;  //queueHandler doesn't handle internal events so they will always be new events
          return strlen(receivedEvent);
        }
      }
    }

    if (const byte availableBytesEvent = EtherEvent.availableEvent(ethernetServer)) {  //there is a new event
      Serial.println(F("---------------------------"));
      Serial.print(F("EtherEventQueue.availableEvent: EtherEvent.availableEvent()="));
      Serial.println(availableBytesEvent);
#ifdef ethernetclientwithremoteIP_h  //this function is only available if the modified Ethernet library is installed
      receivedIP = EtherEvent.senderIP();
      Serial.print(F("EtherEventQueue.availableEvent: remoteIP="));
      Serial.println(receivedIP);
#endif

      nodeTimestamp[nodeDevice] = millis();  //set the device timestamp(using the nodeDevice because that part of the array is never used otherwise)
#ifdef ethernetclientwithremoteIP_h  //this function is only available if the modified Ethernet library is installed
      //update timestamp of the event sender
      byte senderNode = getNode(receivedIP);  //get the node of the senderIP
      if (senderNode >= 0) {  //receivedIP is a node(-1 indicates no node match)
        nodeTimestamp[senderNode] = nodeTimestamp[nodeDevice];  //set the individual timestamp, any communication is considered to be a keepalive - the nodeTimestamp for the device has just been set so I am using that variable so I don't have to call millis() twice for efficiency
        sendKeepaliveTimestamp[senderNode] = millis() - sendKeepaliveResendDelay;
        if (nodeState[senderNode] == nodeStateUnknown) {
          nodeState[senderNode] = nodeStateActive;  //set the node state to active
        }
      }
      else if (receiveNodesOnlyState == 1) {  //the event was not received from a node and it is configured to receive events from node IPs only
        Serial.println(F("EtherEventQueue.availableEvent: unauthorized IP"));
        EtherEvent.flushReceiver();  //event has not been read yet so have to flush
        return 0;
      }
#endif

      EtherEvent.readEvent(receivedEvent);  //put the event in the buffer
      Serial.print(F("EtherEventQueue.availableEvent: event="));
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
      char receivedEventIDchar[eventIDlength + 1];
      for (byte count = 0; count < eventIDlength; count++) {
        receivedEventIDchar[count] = receivedPayloadRaw[count];
      }
      receivedEventIDchar[eventIDlength] = 0;  //add the null terminator because there is not one after the id in the string
      receivedEventIDvalue = atoi(receivedEventIDchar);
      Serial.print(F("EtherEventQueue.availableEvent: eventID="));
      Serial.println(receivedEventIDvalue);

      if (payloadLength > eventIDlength + 1) {  //there is a true payload
        for (byte count = 0; count < payloadLength - eventIDlength; count++) {
          receivedPayload[count] = receivedPayloadRaw[count + eventIDlength];
        }
        Serial.print(F("EtherEventQueue.availableEvent: receivedPayload="));
        Serial.println(receivedPayload);
      }
      else { //no true payload
        receivedPayload[0] = 0;  //clear the payload buffer
      }

      if (strcmp(receivedEvent, eventAck) == 0) {  //ack handler
        Serial.println(F("EtherEventQueue.availableEvent: ack received"));
        byte receivedPayloadInt = atoi(receivedPayload);  //convert to a byte
        for (byte count = 0; count < queueSize; count++) {  //step through the currently occupied section of the eventIDqueue[]
          if (receivedPayloadInt == eventIDqueue[count] && resendFlagQueue[count] == queueTypeConfirm) {  //the ack is for the eventID of this item in the queue and the resend flag indicates it is expecting an ack(non-ack events are not removed because obviously they haven't been sent yet if they're still in the queue so the ack can't possibly be for them)
            Serial.println(F("EtherEventQueue.availableEvent: ack eventID match"));
            remove(count);  //remove the message from the queue
          }
        }
        flushReceiver();  //event and payload have been read so only have to flush EtherEventQueue
        return 0;  //receive ack silently
      }

      receivedEventLength = availableBytesEvent;  //there is a new event
    }
  }
  return receivedEventLength;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//availablePayload - returns the number of chars in the payload including the null terminator if there is one
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte EtherEventQueueClass::availablePayload() {
  Serial.print(F("EtherEventQueue.availablePayload: length="));
  if (byte length = strlen(receivedPayload)) {  //strlen(receivedPayload)>0
    Serial.println(length + 1);
    return length + 1;  //length of the payload + null terminator
  }
  Serial.println(0);
  return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//readEvent - places the event into the passed buffer
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::readEvent(char eventBuffer[]) {
  Serial.println(F("EtherEventQueue.readEvent"));
  strcpy(eventBuffer, receivedEvent);
  receivedEventLength = 0;  //enable availableEvent() to receive new events
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//readPayload - places the payload into the passed buffer
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::readPayload(char payloadBuffer[]) {
  Serial.println(F("EtherEventQueue.readPayload"));
  strcpy(payloadBuffer, receivedPayload);
}


#ifdef ethernetclientwithremoteIP_h  //this function is only available if the modified Ethernet library is installed
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//senderIP - Returns the IP address of the sender of the last event.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPAddress EtherEventQueueClass::senderIP() {
  Serial.print(F("EtherEventQueue.senderIP: senderIP="));
  Serial.println(receivedIP);
  return receivedIP;
}
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//receivedEventID
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte EtherEventQueueClass::receivedEventID() {
  return receivedEventIDvalue;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//flushReceiver - dump the last message received so another one can be received
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::flushReceiver() {
  Serial.println(F("EtherEventQueue.flushReceiver"));
  receivedEvent[0] = 0;  //reset the event buffer
  receivedPayload[0] = 0;  //reset the payload buffer
  receivedEventLength = 0;  //enable availableEvent() to receive new events
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//queue - add the relayed outgoing message to the send queue. Returns: 0==fail, 1==success, 2==success w/ queue overflow
/////////////////////////////////////////////////////////////////////////////////////////////////////////////


//convert IPAddress to 4 byte array
byte EtherEventQueueClass::queue(const IPAddress &targetIPAddress, unsigned int port,  const char event[], const char payload[], byte resendFlag) {
  Serial.print(F("EtherEventQueue.queue(convert IPAddress): targetIPAddress="));
  Serial.println(targetIPAddress);
  byte targetIP[4];  //create buffer
  IPcopy(targetIP, targetIPAddress);  //convert
  return queue(targetIP, port, event, payload, resendFlag);
}


//convert node to 4 byte array
byte EtherEventQueueClass::queue(byte targetNode, unsigned int targetPort, const char event[], const char payload[], byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(convert node)"));
  if (targetNode >= nodeCount) { //sanity check
    Serial.println(F("EtherEventQueue.queue(convert node): invalid node number"));
    return false;
  }
  return queue(nodeIP[targetNode], targetPort, event, payload, resendFlag);
}


//main queue() function
byte EtherEventQueueClass::queue(const byte targetIP[], unsigned int targetPort, const char event[], const char payload[], byte resendFlag) {
  Serial.println(F("EtherEventQueue.queue(main)"));
  int targetNode = getNode(targetIP);
  if (targetNode < 0) { //target is not a node
    if (sendNodesOnlyState == 1) {
      Serial.println(F("EtherEventQueue.queue: not a node"));
      return false;
    }
  }
  //target is a node
  else  if (targetNode == nodeDevice) {  //send events to self regardless of timeout state
    Serial.println(F("EtherEventQueue.queue: self send"));
    localEventQueueCount++;
  }
  else if (targetNode != nodeDevice && millis() - nodeTimestamp[targetNode] > nodeTimeoutDuration) {  //not self, is a node and is timed out
    Serial.println(F("EtherEventQueue.queue: timed out node"));
    return false;  //don't queue events to timed out nodes
  }

  byte success = true;  //indicate event successfully queued in return

  Serial.print(F("EtherEventQueue.queue: queueSize="));
  Serial.println(queueSize);
  if (queueSize == queueSizeMax) {  //queue overflowed
    Serial.println(F("EtherEventQueue.queue: Queue Overflowed"));
    remove(0);  //remove the oldest queued item
    success = queueSuccessOverflow;  //indicate overflow in the return
    queueOverflowFlag = true;
  }

  //add the new message to the queue
  queueSize++;
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
  if (queueSize > 0 && queueSize > localEventQueueCount) {  //there are events in the queue and there are non-local events
    if (queueNewCount > 0 || millis() - queueSendTimestamp > resendDelay) {  //it is time
      if (queueNewCount > queueSize) {  //sanity check - if the acks get messed up this can happen
        queueNewCount = queueSize;
      }
      Serial.print(F("EtherEventQueue.queueHandler: queueSize="));
      Serial.println(queueSize);
      Serial.print(F("EtherEventQueue.queueHandler: queueNewCount="));
      Serial.println(queueNewCount);
      byte queueStepSend;  //this is used to store the step to send which may not be the current step because of sending the new messages first
      for (byte counter = 0; counter < queueSize; counter++) {  //the maximum number of iterations is the queueSize
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
        Serial.print(F("EtherEventQueue.queueHandler: nodeDevice="));
        Serial.println(nodeDevice);
        if (targetNode == nodeDevice) {  //ignore internal events, they are sent in availableEvent()
          Serial.println(F("EtherEventQueue.queueHandler: nodeDevice=targetNode"));
          continue;  //move on to the next queue step
        }
        if (targetNode < 0) {  //-1 indicates no node match
          Serial.println(F("EtherEventQueue.queueHandler: non-node targetIP"));
          break;  //non-nodes never timeout
        }

        if (millis() - nodeTimestamp[targetNode] < nodeTimeoutDuration || strcmp(eventQueue[queueStepSend], eventKeepalive) == 0) {  //non-timed out node or keepalive
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

      if (EtherEvent.send(ethernetClient, IPqueue[queueStepSend], portQueue[queueStepSend], eventQueue[queueStepSend], payload) > 0) {
        Serial.println(F("EtherEventQueue.queueHandler: send successful"));
        nodeTimestamp[nodeDevice] = millis();  //set the device timestamp(using the nodeDevice because that part of the array is never used otherwise)
        //update timestamp of the target node
        byte targetNode = getNode(IPqueue[queueStepSend]);  //get the node of the senderIP
        if (targetNode >= 0) {  //receivedIP is a node(-1 indicates no node match)
          nodeTimestamp[targetNode] = nodeTimestamp[nodeDevice];  //set the individual timestamp, any communication is considered to be a keepalive - the nodeTimestamp for the device has just been set so I am using that variable so I don't have to call millis() twice for efficiency
          sendKeepaliveTimestamp[targetNode] = millis() - sendKeepaliveResendDelay;
          if (nodeState[targetNode] == nodeStateUnknown) {
            nodeState[targetNode] = nodeStateActive;  //set the node state to active
          }

        }

        if (resendFlagQueue[queueStepSend] != queueTypeConfirm) {  //the flag indicates not to wait for an ack
          Serial.println(F("EtherEventQueue.queueHandler: resendFlag != queueTypeConfirm, event removed from queue"));
          remove(queueStepSend);  //remove the message from the queue immediately
        }
        return;
      }
      Serial.println(F("EtherEventQueue.queueHandler: send failed"));
      if (resendFlagQueue[queueStepSend] == queueTypeOnce) {  //the flag indicates not to resend even after failure
        remove(queueStepSend);  //remove keepalives even when send was not successful. This is because the keepalives are sent even to timed out nodes so they shouldn't be queued.
      }
    }
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//flushQueue - removes all events from the queue
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::flushQueue() {
  Serial.println(F("EtherEventQueue.flushQueue"));
  queueSize = 0;
  queueNewCount = 0;
  localEventQueueCount = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//checkTimeout - checks all the nodes until it finds a _NEWLY_ timed out node and returns it and then updates the nodeState value for that node. If no nodes are newly timed out then this function returns -1.  Note that this works differently than checkState()
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int8_t EtherEventQueueClass::checkTimeout() {
  for (byte node = 0; node < nodeCount; node++) {
    if (nodeIP[node][0] == 0 && nodeIP[node][1] == 0 && nodeIP[node][2] == 0 && nodeIP[node][3] == 0) { //node has not been set
      continue;
    }
    if (nodeState[node] == nodeStateActive && millis() - nodeTimestamp[node] > nodeTimeoutDuration) {  //previous state not timed out, and is currently timed out
      Serial.print(F("EtherEventQueue.checkTimeout: timed out node="));
      Serial.println(node);
      nodeState[node] = nodeStateTimedOut;  //set the node state to inactive
      return node;
    }
  }
  Serial.println(F("EtherEventQueue.checkTimeout: no newly timed out nodes"));
  return -1;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//checkTimein - checks all the authorized IPs until it finds a _NEWLY_ timed in node and returns it and then updates the nodeState value for that node. If no nodes are newly timed in then this function returns -1.  Note that this works differently than checkState()
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int8_t EtherEventQueueClass::checkTimein() {
  for (byte node = 0; node < nodeCount; node++) {
    if (nodeIP[node][0] == 0 && nodeIP[node][1] == 0 && nodeIP[node][2] == 0 && nodeIP[node][3] == 0) { //node has not been set
      continue;
    }
    if (nodeState[node] == nodeStateTimedOut && millis() - nodeTimestamp[node] < nodeTimeoutDuration) {  //node is newly timed in(since the last time the function was run)
      Serial.print(F("EtherEventQueue.checkTimein: timed in node="));
      Serial.println(node);
      nodeState[node] = nodeStateActive;  //set the node state to active
      return node;
    }
  }
  Serial.println(F("EtherEventQueue.checkTimein: no newly timed in nodes"));
  return -1;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//checkState - checks if the given node is timed out. Note that this doesn't update the nodeState like checkTimeout()/checkTimein().
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int8_t EtherEventQueueClass::checkState(byte node) {
  Serial.print(F("EtherEventQueue.checkTimeoutNode: nodeState for node "));
  Serial.print(node);
  Serial.print(F("="));
  if (node > nodeCount - 1) { //sanity check
    Serial.println(F("invalid node number"));
    return -1;
  }
  if (nodeTimestamp[node] > nodeTimeoutDuration) {  //node is not this device, not already timed out, and is timed out
    Serial.println(F("timed out"));
    return nodeStateTimedOut;
  }
  Serial.println(F("not timed out"));
  return nodeStateActive;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//checkQueueOverflow
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean EtherEventQueueClass::checkQueueOverflow() {
  Serial.print(F("EtherEventQueue.checkQueueOverflow: queueOverflowFlag="));
  Serial.println(queueOverflowFlag);
  byte queueOverflowFlagValue = queueOverflowFlag;  //save the value before resetting it
  queueOverflowFlag = false;  //reset the flag
  return queueOverflowFlagValue;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//setResendDelay
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::setResendDelay(unsigned long resendDelayValue) {
  Serial.print(F("EtherEventQueue.setResendDelay: resendDelay="));
  Serial.println(resendDelayValue);
  resendDelay = resendDelayValue;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//getResendDelay
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long EtherEventQueueClass::getResendDelay() {
  Serial.print(F("EtherEventQueue.getResendDelay: resendDelay="));
  Serial.println(resendDelay);
  return resendDelay;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//setNodeTimeoutDuration
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::setNodeTimeoutDuration(unsigned long nodeTimeoutDurationValue) {
  Serial.print(F("EtherEventQueue.setNodeTimeoutDuration: nodeTimeoutDuration="));
  Serial.println(nodeTimeoutDurationValue);
  nodeTimeoutDuration = nodeTimeoutDurationValue;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//getNodeTimeoutDuration
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long EtherEventQueueClass::getNodeTimeoutDuration() {
  Serial.print(F("EtherEventQueue.getNodeTimeoutDuration: nodeTimeoutDuration="));
  Serial.println(nodeTimeoutDuration);
  return nodeTimeoutDuration;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//receiveNodesOnly
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef ethernetclientwithremoteIP_h
void EtherEventQueueClass::receiveNodesOnly(boolean receiveNodesOnlyValue) {
  receiveNodesOnlyState = receiveNodesOnlyValue;
  Serial.print(F("EtherEventQueue.receiveNodesOnly: new state="));
  Serial.println(receiveNodesOnlyState);
}
#endif


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//sendNodesOnly
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::sendNodesOnly(boolean sendNodesOnlyValue) {
  sendNodesOnlyState = sendNodesOnlyValue;
  Serial.print(F("EtherEventQueue.receiveNodesOnly: new state="));
  Serial.println(sendNodesOnlyState);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//removeNode
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::removeNode(byte nodeNumber) {
  Serial.println(F("EtherEventQueue.removeNode"));
  if (nodeNumber >= nodeCount) { //sanity check
    Serial.println(F("EtherEventQueue.removeNode: invalid node number"));
  }
  else {
    for (byte counter = 0; counter < 4; counter++) {
      nodeIP[nodeNumber][counter] = 0;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//getIP
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPAddress EtherEventQueueClass::getIP(byte nodeNumber) {
  Serial.println(F("EtherEventQueue.getIP"));
  if (nodeNumber >= nodeCount) { //sanity check
    Serial.println(F("EtherEventQueue.getIP: invalid node number"));
  }
  else {
    return IPAddress(nodeIP[nodeNumber][0], nodeIP[nodeNumber][1], nodeIP[nodeNumber][2], nodeIP[nodeNumber][3]);
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//setSendKeepaliveMargin
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::setSendKeepaliveMargin(unsigned long sendKeepaliveMarginInput) {
  Serial.println(F("EtherEventQueue.setSendKeepaliveMargin"));
  sendKeepaliveMargin = min(sendKeepaliveMarginInput, nodeTimeoutDuration);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//getSendKeepaliveMargin
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long EtherEventQueueClass::getSendKeepaliveMargin() {
  Serial.println(F("EtherEventQueue.getSendKeepaliveMargin"));
  return sendKeepaliveMargin;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//sendKeepalive
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::sendKeepalive(unsigned int port) {
  for (byte node = 0; node < nodeCount; node++) {
    if (node == nodeDevice || (nodeIP[node][0] == 0 && nodeIP[node][1] == 0 && nodeIP[node][2] == 0 && nodeIP[node][3] == 0)) { //device node or node has not been set
      continue;
    }
    if (millis() - nodeTimestamp[node] > nodeTimeoutDuration - sendKeepaliveMargin && millis() - sendKeepaliveTimestamp[node] > sendKeepaliveResendDelay) { //node is newly timed out(since the last time the function was run)
      Serial.print(F("EtherEventQueue.sendKeepalive: sending to node="));
      Serial.println(node);
      queue(node, port, eventKeepalive, "", queueTypeOnce);
    }
  }
  Serial.println(F("EtherEventQueue.sendKeepalive: no keepalive sent"));
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//setSendKeepaliveResendDelay
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::setSendKeepaliveResendDelay(unsigned long sendKeepaliveResendDelayInput) {
  Serial.print(F("EtherEventQueue.setSendKeepaliveResendDelay: sendKeepaliveResendDelayInput="));
  Serial.println(sendKeepaliveResendDelayInput);
  sendKeepaliveResendDelay = sendKeepaliveResendDelayInput;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//getSendKeepaliveResendDelay
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long EtherEventQueueClass::getSendKeepaliveResendDelay() {
  Serial.print(F("EtherEventQueue.getSendKeepaliveResendDelay, sendKeepaliveResendDelay="));
  Serial.println(sendKeepaliveResendDelay);
  return sendKeepaliveResendDelay;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//private functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//eventIDfind - find a free eventID
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
byte EtherEventQueueClass::eventIDfind() {
  Serial.println(F("EtherEventQueue.eventIDfind"));
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
  Serial.print(F("EtherEventQueue.remove: queueStep="));
  Serial.println(queueStep);
  if (queueSize > 1) {
    queueSize--;
    if (getNode(IPqueue[queueStep]) == nodeDevice) {  //the removed queue item is a local event
      localEventQueueCount--;
    }
    for (byte count = queueStep; count < queueSize; count++) {  //move all the messages above the one to remove up in the queue
      IPcopy(IPqueue[count], IPqueue[count + 1]);
      portQueue[count] = portQueue[count + 1];
      strcpy(eventQueue[count], eventQueue[count + 1]);
      eventIDqueue[count] = eventIDqueue[count + 1];
      strcpy(payloadQueue[count], payloadQueue[count + 1]);
      resendFlagQueue[count] = resendFlagQueue[count + 1];
    }
  }
  else {
    queueSize = 0;  //if there is only one event in the queue then nothing needs to be shifted
  }
  Serial.print(F("EtherEventQueue.remove: new queue size="));
  Serial.println(queueSize);
}


EtherEventQueueClass EtherEventQueue;  //This sets up a single global instance of the library so the class doesn't need to be declared in the user sketch and multiple instances are not necessary in this case.
