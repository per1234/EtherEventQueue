// EtherEventQueue - outgoing event queue for the EtherEvent authenticated network communication Arduino library: http://github.com/per1234/EtherEventQueue
#define ETHEREVENT_NO_AUTHENTICATION  //this is to prevent EtherEvent.h from including MD5.h(not needed in this file even with authentication enabled
#define ETHEREVENT_FAST_SEND
#include "EtherEventQueue.h"

#define Serial if(ETHEREVENTQUEUE_DEBUG)Serial  //DEBUG is defined in EtherEventQueue.h

const unsigned long nodeTimeoutDurationDefault = 270000;  //(ms)the node is timed out if it has been longer than this duration since the last event was received from it
const unsigned long sendKeepaliveMarginDefault = 30000;
const unsigned long sendKeepaliveResendDelayDefault = 60000;
const unsigned int resendDelayDefault = 45000;  //(ms)delay between resends of messages

const byte queueSizeMaxDefault = 5;
const byte eventLengthMaxDefault = 15;
const byte payloadLengthMaxDefault = 80;

const byte queueDoubleDecimalPlacesDefault = 3;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//constructor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
EtherEventQueueClass::EtherEventQueueClass() {
  nodeTimeoutDuration = nodeTimeoutDurationDefault;
  sendKeepaliveMargin = sendKeepaliveMarginDefault;
  sendKeepaliveResendDelay = sendKeepaliveResendDelayDefault;
  resendDelay = resendDelayDefault;
  queueDoubleDecimalPlaces = queueDoubleDecimalPlacesDefault;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//begin
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean EtherEventQueueClass::begin() {  //no nodes, default buffer length version - the deviceNode is 0
  return begin(0, 1, queueSizeMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault);
}


boolean EtherEventQueueClass::begin(const byte queueSizeMaxInput, const byte sendEventLengthMaxInput, const unsigned int sendPayloadLengthMaxInput, const byte receivedEventLengthMaxInput, const unsigned int receivedPayloadLengthMaxInput) {  //no nodes version - the deviceNode is 0
  return begin(0, 1, queueSizeMaxInput, sendEventLengthMaxInput, sendPayloadLengthMaxInput, receivedEventLengthMaxInput, receivedPayloadLengthMaxInput);
}


boolean EtherEventQueueClass::begin(const byte nodeDeviceInput, const byte nodeCountInput) {  //default buffer length version - the deviceNode is 0
  return begin(nodeDeviceInput, nodeCountInput, queueSizeMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault);
}


boolean EtherEventQueueClass::begin(const byte nodeDeviceInput, byte nodeCountInput, byte queueSizeMaxInput, const byte sendEventLengthMaxInput, const unsigned int sendPayloadLengthMaxInput, const byte receivedEventLengthMaxInput, const unsigned int receivedPayloadLengthMaxInput) {
#if ETHEREVENTQUEUE_DEBUG == true
  delay(20);  //There needs to be a delay between the calls to Serial.begin() in sketch setup() and here or garbage will be printed to the serial monitor
#endif
  Serial.begin(9600);  //for debugging
  Serial.println(F("\n\n\nEtherEventQueue.begin"));

  nodeDevice = nodeDeviceInput;
  nodeCountInput = max(nodeDevice + 1, nodeCountInput);  //the nodeCount has to be enough to hold the device node number
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
  nodeCount = max(nodeDevice + 1, nodeCountInput);  //set this after the buffers have been realloced so that the old value can be used for free()ing the array items

  setNode(nodeDeviceInput, Ethernet.localIP());  //configure the device node

  //buffer sizing - these are dynamically allocated so that the sized can be set via the API
  //size send event queue buffers
  queueSizeMaxInput = min(queueSizeMaxInput, 90);  //the current system uses a 2 digit messageID so the range is 10-99, this restricts the queueSizeMax <= 90

  queueIndex = (int8_t*)realloc(queueIndex, queueSizeMaxInput * sizeof(int8_t));
  for (byte counter = 0; counter < queueSizeMaxInput; counter++) {
    queueIndex[counter] = -1;  //set all queueIndex priority levels empty
  }

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

  eventTypeQueue = (byte*)realloc(eventTypeQueue, queueSizeMaxInput * sizeof(byte));

  queueSizeMax = queueSizeMaxInput;  //save the new queueSizeMax, this is done at the end of begin() because it needs to remember the previous value for freeing the array items

  //size received event buffers
  receivedEventLengthMax = receivedEventLengthMaxInput;
  receivedEvent = (char*)realloc(receivedEvent, (receivedEventLengthMax + 1) * sizeof(char));
  receivedEvent[0] = 0;  //clear buffer - realloc does not zero initialize so the buffer could contain anything
  receivedPayloadLengthMax = receivedPayloadLengthMaxInput;
  receivedPayload = (char*)realloc(receivedPayload, (receivedPayloadLengthMax + 1) * sizeof(char));
  receivedPayload[0] = 0;  //clear buffer - realloc does not zero initialize so the buffer could contain anything
  if (IPqueue == NULL || portQueue == NULL || eventQueue == NULL || eventIDqueue == NULL || payloadQueue == NULL || eventTypeQueue == NULL || receivedEvent == NULL || receivedPayload == NULL || EtherEvent.begin(receivedEventLengthMax, eventIDlength + receivedPayloadLengthMax) == false) {
    Serial.println(F("memory allocation failed"));
    return false;
  }
  return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//availablePayload - returns the number of chars in the payload including the null terminator if there is one
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int EtherEventQueueClass::availablePayload() {
  Serial.print(F("EtherEventQueue.availablePayload: length="));
  if (const unsigned int length = strlen(receivedPayload)) {  //strlen(receivedPayload)>0
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
byte EtherEventQueueClass::queue(const IPAddress &targetIPAddress, const unsigned int port, const byte eventType, const char event[], const char payload[]) {
  Serial.print(F("EtherEventQueue.queue(convert IPAddress): targetIPAddress="));
  Serial.println(targetIPAddress);
  byte targetIP[4];  //create buffer
  IPcopy(targetIP, targetIPAddress);  //convert
  return queue((const byte*)targetIP, port, eventType, (const char*)event, payload);
}


//convert node to 4 byte array
byte EtherEventQueueClass::queue(const byte targetNode, const unsigned int port, const byte eventType, const char event[], const char payload[]) {
  Serial.println(F("EtherEventQueue.queue(convert node)"));
  if (targetNode >= nodeCount || !nodeIsSet(targetNode)) {  //sanity check
    Serial.println(F("EtherEventQueue.queue(convert node): invalid node number"));
    return false;
  }
  return queue((const byte*)nodeIP[targetNode], port, eventType, (const char*)event, payload);
}


//main queue() function
byte EtherEventQueueClass::queue(const byte targetIP[], const unsigned int port, const byte eventType, const char event[], const char payload[]) {
  Serial.println(F("EtherEventQueue.queue(main)"));
  if ((eventType != eventTypeOnce && eventType != eventTypeRepeat && eventType != eventTypeConfirm && eventType != eventTypeOverrideTimeout) || (eventType == eventTypeConfirm && eventAck == NULL)) { //eventType sanity check
    Serial.println(F("EtherEventQueue.queue: invalid eventType"));
    return false;
  }
  const int targetNode = getNode(targetIP);
  if (targetNode < 0) {  //target is not a node
    if (sendNodesOnlyState == 1) {
      Serial.println(F("EtherEventQueue.queue: not a node"));
      return false;
    }
  }
  //target is a node
  else  if (targetNode == nodeDevice) {  //send events to self regardless of timeout state
    Serial.println(F("EtherEventQueue.queue: self send"));
    internalEventQueueCount++;
  }
  else if (millis() - nodeTimestamp[targetNode] > nodeTimeoutDuration && eventType != eventTypeOverrideTimeout) {  //is a node, not self, is timed out, and is not eventTypeOverrideTimeout
    Serial.println(F("EtherEventQueue.queue: timed out node"));
    return false;  //don't queue events to timed out nodes
  }

  byte success = true;  //set default success value to indicate event successfully queued in return

  Serial.print(F("EtherEventQueue.queue: queueSize="));
  Serial.println(queueSize);

  byte queueSlot;
  if (queueSize == queueSizeMax) {  //queue overflowed
    Serial.println(F("EtherEventQueue.queue: Queue Overflowed"));
    queueSlot = queueIndex[queueSize - 1];
    remove(queueSlot);
    success = queueSuccessOverflow;  //indicate overflow in the return
    queueOverflowFlag = true;  //set the overflow flag for use in checkQueueOverflow()
  }
  else {  //there are empty queue slots
    //find an empty queue slot
    //the queueIndex is a list of the filled queue slots in order of least to most recently queued, a value of -1 in a queueSlot position indicates that the position is empty(and therefore all higher positions)
    //the queuePriorityLevel is the queueIndex position of the next event to send
    for (queueSlot = 0; queueSlot < queueSizeMax; queueSlot++) {
      byte counter = 0;
      for (counter = 0; counter < queueSizeMax; counter++) {
        if (queueIndex[counter] == queueSlot || queueIndex[counter] == -1) {  //the queue slot is filled or not found in the index
          break;
        }
      }
      if (queueIndex[counter] == -1) {  //the queue slot is empty
        break;
      }
    }
  }

  //add the new message to the queue
  queueSize++;
  queueIndex[queueSize - 1] = queueSlot;
  IPcopy(IPqueue[queueSlot], targetIP);
  portQueue[queueSlot] = port;
  strncpy(eventQueue[queueSlot], event, sendEventLengthMax);
  eventQueue[queueSlot][sendEventLengthMax] = 0;  //add null terminator in case event is longer than sendPayloadLengthMax
  eventIDqueue[queueSlot] = eventIDfind();
  strncpy(payloadQueue[queueSlot], payload, sendPayloadLengthMax);
  payloadQueue[queueSlot][sendPayloadLengthMax] = 0;  //add null terminator in case payload is longer than sendPayloadLengthMax
  eventTypeQueue[queueSlot] = eventType;

  queueNewCount++;

  Serial.print(F("EtherEventQueue.queue: done, queueSlot="));
  Serial.println(queueSlot);
  Serial.print(F("EtherEventQueue.queue: queueNewCount="));
  Serial.println(queueNewCount);
  Serial.print(F("EtherEventQueue.queue: IP="));
  Serial.println(IPAddress(IPqueue[queueSlot]));
  Serial.print(F("EtherEventQueue.queue: port="));
  Serial.println(portQueue[queueSlot]);
  Serial.print(F("EtherEventQueue.queue: event="));
  Serial.println(eventQueue[queueSlot]);
  Serial.print(F("EtherEventQueue.queue: payload="));
  Serial.println(payloadQueue[queueSlot]);
  Serial.print(F("EtherEventQueue.queue: eventID="));
  Serial.println(eventIDqueue[queueSlot]);
  Serial.print(F("EtherEventQueue.queue: eventType="));
  Serial.println(eventTypeQueue[queueSlot]);
  return success;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//flushQueue - removes all events from the queue
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::flushQueue() {
  Serial.println(F("EtherEventQueue.flushQueue"));
  queueSize = 0;
  queueNewCount = 0;
  internalEventQueueCount = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//checkTimeout - checks all the nodes until it finds a _NEWLY_ timed out node and returns it and then updates the nodeState value for that node. If no nodes are newly timed out then this function returns -1.  Note that this works differently than checkState()
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int8_t EtherEventQueueClass::checkTimeout() {
  for (byte node = 0; node < nodeCount; node++) {
    if (!nodeIsSet(node)) {  //node has not been set
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
    if (!nodeIsSet(node)) {  //node has not been set
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
int8_t EtherEventQueueClass::checkState(const byte node) {
  Serial.print(F("EtherEventQueue.checkTimeoutNode: nodeState for node "));
  Serial.print(node);
  Serial.print(F("="));
  if (node > nodeCount - 1) {  //sanity check
    Serial.println(F("invalid node number"));
    return -1;
  }
  if (millis() - nodeTimestamp[node] > nodeTimeoutDuration) {  //node is not this device, not already timed out, and is timed out
    Serial.println(F("timed out"));
    return false;
  }
  Serial.println(F("not timed out"));
  return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//checkQueueOverflow
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean EtherEventQueueClass::checkQueueOverflow() {
  Serial.print(F("EtherEventQueue.checkQueueOverflow: queueOverflowFlag="));
  Serial.println(queueOverflowFlag);
  const byte queueOverflowFlagValue = queueOverflowFlag;  //save the value before resetting it
  queueOverflowFlag = false;  //reset the flag
  return queueOverflowFlagValue;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//setResendDelay
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::setResendDelay(const unsigned long resendDelayValue) {
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
void EtherEventQueueClass::setNodeTimeoutDuration(const unsigned long nodeTimeoutDurationValue) {
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
void EtherEventQueueClass::receiveNodesOnly(const boolean receiveNodesOnlyValue) {
  receiveNodesOnlyState = receiveNodesOnlyValue;
  Serial.print(F("EtherEventQueue.receiveNodesOnly: new state="));
  Serial.println(receiveNodesOnlyState);
}
#endif  //ethernetclientwithremoteIP_h


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//sendNodesOnly
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::sendNodesOnly(const boolean sendNodesOnlyValue) {
  sendNodesOnlyState = sendNodesOnlyValue;
  Serial.print(F("EtherEventQueue.receiveNodesOnly: new state="));
  Serial.println(sendNodesOnlyState);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//removeNode
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::removeNode(const byte nodeNumber) {
  Serial.println(F("EtherEventQueue.removeNode"));
  if (nodeNumber >= nodeCount) {  //sanity check
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
IPAddress EtherEventQueueClass::getIP(const byte nodeNumber) {
  Serial.println(F("EtherEventQueue.getIP"));
  if (nodeNumber >= nodeCount) {  //sanity check
    Serial.println(F("EtherEventQueue.getIP: invalid node number"));
    return IPAddress(0, 0, 0, 0);
  }
  else {
    return IPAddress(nodeIP[nodeNumber][0], nodeIP[nodeNumber][1], nodeIP[nodeNumber][2], nodeIP[nodeNumber][3]);
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//setSendKeepaliveMargin
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::setSendKeepaliveMargin(const unsigned long sendKeepaliveMarginInput) {
  Serial.println(F("EtherEventQueue.setSendKeepaliveMargin"));
  sendKeepaliveMargin = min(sendKeepaliveMarginInput, nodeTimeoutDuration);  //sendKeepaliveMargin can't be greater than nodeTimeoutDuration
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
void EtherEventQueueClass::sendKeepalive(const unsigned int port) {
  if (eventKeepalive == NULL) {
    Serial.println(F("EtherEventQueue.sendKeepalive: eventKeepalive not set"));
    return;
  }
  for (byte node = 0; node < nodeCount; node++) {
    if (node == nodeDevice || !nodeIsSet(node)) {  //device node or node has not been set
      continue;
    }
    if (millis() - nodeTimestamp[node] > nodeTimeoutDuration - sendKeepaliveMargin && millis() - sendKeepaliveTimestamp[node] > sendKeepaliveResendDelay) {  //node is newly timed out(since the last time the function was run)
      Serial.print(F("EtherEventQueue.sendKeepalive: sending to node="));
      Serial.println(node);
      queue(node, port, eventTypeOverrideTimeout, eventKeepalive);
      sendKeepaliveTimestamp[node] = millis();
    }
  }
  Serial.println(F("EtherEventQueue.sendKeepalive: no keepalive sent"));
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//setSendKeepaliveResendDelay
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::setSendKeepaliveResendDelay(const unsigned long sendKeepaliveResendDelayInput) {
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
//setEventKeepalive
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean EtherEventQueueClass::setEventKeepalive(const char eventKeepaliveInput[]) {
  Serial.println(F("EtherEventQueue.setEventKeepalive"));
  const byte eventKeepaliveLength = strlen(eventKeepaliveInput);
  eventKeepalive = (char*)realloc(eventKeepalive, (eventKeepaliveLength + 1) * sizeof(*eventKeepalive));  //allocate memory
  if (eventKeepalive == NULL) {
    Serial.println(F("EtherEventQueue.setEventKeepalive: memory allocation failed"));
    return false;
  }
  strcpy(eventKeepalive, eventKeepaliveInput);  //store the event
  return true;
}


boolean EtherEventQueueClass::setEventKeepalive(const int16_t eventKeepaliveInput) {
  char eventKeepaliveInputChar[int16_tLengthMax + 1];
  itoa(eventKeepaliveInput, eventKeepaliveInputChar, 10);
  return setEventKeepalive(eventKeepaliveInputChar);
}


boolean EtherEventQueueClass::setEventKeepalive(const uint16_t eventKeepaliveInput) {
  char eventKeepaliveInputChar[uint16_tLengthMax + 1];
  sprintf_P(eventKeepaliveInputChar, PSTR("%u"), eventKeepaliveInput);
  return setEventKeepalive(eventKeepaliveInputChar);
}


boolean EtherEventQueueClass::setEventKeepalive(const int32_t eventKeepaliveInput) {
  char eventKeepaliveInputChar[int32_tLengthMax + 1];
  ltoa(eventKeepaliveInput, eventKeepaliveInputChar, 10);
  return setEventKeepalive(eventKeepaliveInputChar);
}


boolean EtherEventQueueClass::setEventKeepalive(const uint32_t eventKeepaliveInput) {
  char eventKeepaliveInputChar[uint32_tLengthMax + 1];
  ultoa(eventKeepaliveInput, eventKeepaliveInputChar, 10);
  return setEventKeepalive(eventKeepaliveInputChar);
}


boolean EtherEventQueueClass::setEventKeepalive(const __FlashStringHelper* eventKeepaliveFSH) {
  byte stringLength = EtherEvent.FSHlength(eventKeepaliveFSH);
  char eventKeepaliveChar[stringLength + 1];
  memcpy_P(eventKeepaliveChar, eventKeepaliveFSH, stringLength + 1);  //+1 for the null terminator
  return setEventKeepalive(eventKeepaliveChar);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//setEventAck
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean EtherEventQueueClass::setEventAck(const char eventAckInput[]) {
  Serial.println(F("EtherEventQueue.setEventAck"));
  const byte eventAckLength = strlen(eventAckInput);
  eventAck = (char*)realloc(eventAck, (eventAckLength + 1) * sizeof(*eventAck));  //allocate memory
  if (eventAck == NULL) {
    Serial.println(F("EtherEventQueue.setEventAck: memory allocation failed"));
    return false;
  }
  strcpy(eventAck, eventAckInput);  //store the event
  return true;
}


boolean EtherEventQueueClass::setEventAck(const int16_t eventAckInput) {
  char eventAckInputChar[int16_tLengthMax + 1];
  itoa(eventAckInput, eventAckInputChar, 10);
  return setEventAck(eventAckInputChar);
}


boolean EtherEventQueueClass::setEventAck(const uint16_t eventAckInput) {
  char eventAckInputChar[uint16_tLengthMax + 1];
  sprintf_P(eventAckInputChar, PSTR("%u"), eventAckInput);
  return setEventAck(eventAckInputChar);
}


boolean EtherEventQueueClass::setEventAck(const int32_t eventAckInput) {
  char eventAckInputChar[int32_tLengthMax + 1];
  ltoa(eventAckInput, eventAckInputChar, 10);
  return setEventAck(eventAckInputChar);
}


boolean EtherEventQueueClass::setEventAck(const uint32_t eventAckInput) {
  char eventAckInputChar[uint32_tLengthMax + 1];
  ultoa(eventAckInput, eventAckInputChar, 10);
  return setEventAck(eventAckInputChar);
}


boolean EtherEventQueueClass::setEventAck(const __FlashStringHelper* eventAckFSH) {
  byte stringLength = EtherEvent.FSHlength(eventAckFSH);
  char eventAckChar[stringLength + 1];
  memcpy_P(eventAckChar, eventAckFSH, stringLength + 1);  //+1 for the null terminator
  return setEventAck(eventAckChar);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//setQueueDoubleDecimalPlaces - set the number of decimal places to queue of double/float event/payload
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::setQueueDoubleDecimalPlaces(byte decimalPlaces) {
  queueDoubleDecimalPlaces = decimalPlaces;
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
void EtherEventQueueClass::remove(const byte removeQueueSlot) {
  Serial.print(F("EtherEventQueue.remove: queueSlot="));
  Serial.println(removeQueueSlot);
  if (IPqueue[removeQueueSlot][0] == nodeIP[nodeDevice][0] && IPqueue[removeQueueSlot][1] == nodeIP[nodeDevice][1] && IPqueue[removeQueueSlot][2] == nodeIP[nodeDevice][2] && IPqueue[removeQueueSlot][3] == nodeIP[nodeDevice][3]) {  //the queue item to remove is an internal event
    if (internalEventQueueCount > 0) {  //sanity check
      internalEventQueueCount--;
    }
  }
  if (queueSize > 1) {
    //find the priority level of the queueSlot to be removed
    byte removeQueueSlotPriorityLevel;
    for (removeQueueSlotPriorityLevel = 0; removeQueueSlotPriorityLevel < queueSize; removeQueueSlotPriorityLevel++) {
      if (queueIndex[removeQueueSlotPriorityLevel] == removeQueueSlot) {
        break;
      }
    }

    //move up all queue slots with a larger priority level value than the removed slot
    queueSize--;
    byte counter;
    for (counter = removeQueueSlotPriorityLevel; counter < queueSize; counter++) {
      queueIndex[counter] = queueIndex[counter + 1];
    }
    queueIndex[counter + 1] = -1;  //clear the last slot

    //adjust the removeQueueSlotPriorityLevel to account for the revised queueIndex
    if (queuePriorityLevel >= removeQueueSlotPriorityLevel) {
      if (queuePriorityLevel == 0) {
        queuePriorityLevel = queueSize - 1;
      }
      else {
        queuePriorityLevel--;
      }
    }
  }
  else {  //there is only one item in the queue so priority level == 0
    queueSize = 0;  //make sure that the queueSize will never negative overflow
    queueIndex[0] = -1;  //clear the slot
  }
  Serial.print(F("EtherEventQueue.remove: new queue size="));
  Serial.println(queueSize);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//nodeIsSet - check if the node has been set
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
boolean EtherEventQueueClass::nodeIsSet(const byte nodeNumber) {
  Serial.print(F("EtherEventQueue.nodeIsSet: result="));
  if (nodeIP[nodeNumber][0] == 0 && nodeIP[nodeNumber][1] == 0 && nodeIP[nodeNumber][2] == 0 && nodeIP[nodeNumber][3] == 0) {
    Serial.println(F("false"));
    return false;
  }
  Serial.println(F("true"));
  return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FSHtoa - convert __FlashStringHelper to char and put it in the passed buffer
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void EtherEventQueueClass::FSHtoa(const __FlashStringHelper* FlashString, char charBuffer[], byte maxLength) {
  PGM_P FlashString_P = reinterpret_cast<PGM_P>(FlashString);
  for (byte arrayPosition = 0; arrayPosition < maxLength; arrayPosition++) {
    unsigned char character = pgm_read_byte(FlashString_P++);
    charBuffer[arrayPosition] = character;
    if (character == 0) {
      return;
    }
  }
  charBuffer[maxLength] = 0;
}


EtherEventQueueClass EtherEventQueue;  //This sets up a single global instance of the library so the class doesn't need to be declared in the user sketch and multiple instances are not necessary in this case.

