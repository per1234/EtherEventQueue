// EtherEventQueue - outgoing event queue for the EtherEvent authenticated network communication Arduino library: http://github.com/per1234/EtherEventQueue
#ifndef EtherEventQueue_h
#define EtherEventQueue_h

#include <Arduino.h>

#if defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD)
#include <avr/dtostrf.h>
#endif

#include <EtherEvent.h>


#define ETHEREVENTQUEUE_DEBUG false  //(false == serial debug output off,  true == serial debug output on)The serial debug output will increase memory usage and communication latency so only enable when needed.
#define ETHEREVENTQUEUE_SERIAL if(ETHEREVENTQUEUE_DEBUG)Serial  //I have to use a different name for Serial in this file otherwise the debug statement control also affects any other file that includes this file.
#if ETHEREVENTQUEUE_DEBUG == true
#pragma message "EtherEventQueue debug output enabled"
#endif  //ETHEREVENTQUEUE_DEBUG == true


class EtherEventQueueClass {
  public:
    //public constants
    static const byte eventTypeOnce = 0;
    static const byte eventTypeRepeat = 1;
    static const byte eventTypeConfirm = 2;
    static const byte eventTypeOverrideTimeout = 3;

    static const byte queueSuccessOverflow = 2;


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //constructor
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    EtherEventQueueClass() {
      nodeTimeoutDuration = nodeTimeoutDurationDefault;
      sendKeepaliveMargin = sendKeepaliveMarginDefault;
      sendKeepaliveResendDelay = sendKeepaliveResendDelayDefault;
      resendDelay = resendDelayDefault;
      queueDoubleDecimalPlaces = queueDoubleDecimalPlacesDefault;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //begin
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    boolean begin() {  //no nodes, default buffer length version - the deviceNode is 0
      return begin(0, 1, queueSizeMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault);
    }


    boolean begin(const byte queueSizeMaxInput, const byte sendEventLengthMaxInput, const unsigned int sendPayloadLengthMaxInput, const byte receivedEventLengthMaxInput, const unsigned int receivedPayloadLengthMaxInput) {  //no nodes version - the deviceNode is 0
      return begin(0, 1, queueSizeMaxInput, sendEventLengthMaxInput, sendPayloadLengthMaxInput, receivedEventLengthMaxInput, receivedPayloadLengthMaxInput);
    }


    boolean begin(const byte nodeDeviceInput, const byte nodeCountInput) {  //default buffer length version - the deviceNode is 0
      return begin(nodeDeviceInput, nodeCountInput, queueSizeMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault, eventLengthMaxDefault, payloadLengthMaxDefault);
    }


    boolean begin(const byte nodeDeviceInput, byte nodeCountInput, byte queueSizeMaxInput, const byte sendEventLengthMaxInput, const unsigned int sendPayloadLengthMaxInput, const byte receivedEventLengthMaxInput, const unsigned int receivedPayloadLengthMaxInput) {
#if ETHEREVENTQUEUE_DEBUG == true
      delay(20);  //There needs to be a delay between the calls to ETHEREVENTQUEUE_SERIAL.begin() in sketch setup() and here or garbage will be printed to the serial monitor
#endif
      ETHEREVENTQUEUE_SERIAL.begin(9600);  //for debugging
      ETHEREVENTQUEUE_SERIAL.println(F("\n\n\nEtherEventQueue.begin"));

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
        ETHEREVENTQUEUE_SERIAL.println(F("memory allocation failed"));
        return false;
      }
      return true;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //availableEvent - Check for new incoming events, process and buffer them and return the length of the event string. This function was moved to the header file so that the authentication disable system would work
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef ETHEREVENT_NO_AUTHENTICATION
    byte availableEvent(EthernetServer &ethernetServer, long cookieInput = false) {
#else
    byte availableEvent(EthernetServer &ethernetServer) {
#endif
      if (receivedEventLength == 0) {  //there is no event buffered
        if (internalEventQueueCount > 0) {
          for (int8_t queueStepCount = queueSize - 1; queueStepCount >= 0; queueStepCount--) {  //internal event system: step through the queue from the newest to oldest
            if (IPqueue[queueStepCount][0] == nodeIP[nodeDevice][0] && IPqueue[queueStepCount][1] == nodeIP[nodeDevice][1] && IPqueue[queueStepCount][2] == nodeIP[nodeDevice][2] && IPqueue[queueStepCount][3] == nodeIP[nodeDevice][3]) {  //internal event
              strcpy(receivedEvent, eventQueue[queueStepCount]);
              ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availableEvent: internal event="));
              ETHEREVENTQUEUE_SERIAL.println(receivedEvent);
              strcpy(receivedPayload, payloadQueue[queueStepCount]);
              ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availableEvent: internal event payload="));
              ETHEREVENTQUEUE_SERIAL.println(receivedPayload);
              remove(queueStepCount);  //remove the event from the queue
              queueNewCount--;  //queueHandler doesn't handle internal events so they will always be new events
              return strlen(receivedEvent);
            }
          }
        }

#ifndef ETHEREVENT_NO_AUTHENTICATION
        if (const int availableBytesEvent = EtherEvent.availableEvent(ethernetServer, cookieInput) > 0) {  //there is a new event
#else  //ETHEREVENT_NO_AUTHENTICATION
        if (const int availableBytesEvent = EtherEvent.availableEvent(ethernetServer) > 0) {  //there is a new event
#endif  //ETHEREVENT_NO_AUTHENTICATION
          ETHEREVENTQUEUE_SERIAL.println(F("---------------------------"));
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availableEvent: EtherEvent.availableEvent()="));
          ETHEREVENTQUEUE_SERIAL.println(availableBytesEvent);
#ifdef ethernetclientwithremoteIP_h  //this function is only available if the modified Ethernet library is installed
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availableEvent: remoteIP="));
          ETHEREVENTQUEUE_SERIAL.println(EtherEvent.senderIP());
#endif  //ethernetclientwithremoteIP_h

          nodeTimestamp[nodeDevice] = millis();  //set the device timestamp(using the nodeDevice because that part of the array is never used otherwise)
#ifdef ethernetclientwithremoteIP_h  //this function is only available if the modified Ethernet library is installed
          //update timestamp of the event sender
          const int8_t senderNode = getNode(EtherEvent.senderIP());  //get the node of the senderIP
          if (senderNode >= 0) {  //receivedIP is a node(-1 indicates no node match)
            nodeTimestamp[senderNode] = nodeTimestamp[nodeDevice];  //set the individual timestamp, any communication is considered to be a received keepalive - the nodeTimestamp for the device has just been set so I am using that variable so I don't have to call millis() twice for efficiency
            sendKeepaliveTimestamp[senderNode] = nodeTimestamp[nodeDevice] - sendKeepaliveResendDelay;  //Treat successful receive of any event as a sent keepalive so delay the send of the next keepalive. -sendKeepaliveResendDelay is so that sendKeepalive() will be able to queue the eventKeepalive according to "millis() - nodeTimestamp[node] > nodeTimeoutDuration - sendKeepaliveMargin" without being blocked by the "millis() - sendKeepaliveTimestamp[node] > sendKeepaliveResendDelay", it will not cause immediate queue of eventKeepalive because nodeTimestamp[targetNode] has just been set. The nodeTimestamp for the device has just been set so I am using that variable so I don't have to call millis() again
            if (nodeState[senderNode] == nodeStateUnknown) {
              nodeState[senderNode] = nodeStateActive;  //set the node state to active
            }
          }
          else if (receiveNodesOnlyState == 1) {  //the event was not received from a node and it is configured to receive events from node IPs only
            ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.availableEvent: unauthorized IP"));
            EtherEvent.flushReceiver();  //event has not been read yet so have to flush
            return 0;
          }
#endif  //ethernetclientwithremoteIP_h

          EtherEvent.readEvent(receivedEvent);  //put the event in the buffer
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availableEvent: event="));
          ETHEREVENTQUEUE_SERIAL.println(receivedEvent);

          if (eventKeepalive != NULL && strcmp(receivedEvent, eventKeepalive) == 0) {  //keepalive received
            ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.availableEvent: keepalive received"));
            flushReceiver();  //the event has been read so EtherEventQueue has to be flushed
            return 0;  //receive keepalive silently
          }

          const unsigned int payloadLength = EtherEvent.availablePayload();
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availableEvent: EtherEvent.availablePayload()="));
          ETHEREVENTQUEUE_SERIAL.println(payloadLength);
          char receivedPayloadRaw[payloadLength];
          EtherEvent.readPayload(receivedPayloadRaw);  //read the payload to the buffer
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availableEvent: rawPayload="));
          ETHEREVENTQUEUE_SERIAL.println(receivedPayloadRaw);

          //break the payload down into parts and convert the eventID to byte, the true payload stays as a char array
          //the first part of the raw payload is the eventID
          char receivedEventIDchar[eventIDlength + 1];
          for (byte count = 0; count < eventIDlength; count++) {
            receivedEventIDchar[count] = receivedPayloadRaw[count];
          }
          receivedEventIDchar[eventIDlength] = 0;  //add the null terminator because there is not one after the id in the string
          receivedEventIDvalue = atoi(receivedEventIDchar);
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availableEvent: eventID="));
          ETHEREVENTQUEUE_SERIAL.println(receivedEventIDvalue);

          if (payloadLength > eventIDlength + 1) {  //there is a true payload
            for (unsigned int count = 0; count < payloadLength - eventIDlength; count++) {
              receivedPayload[count] = receivedPayloadRaw[count + eventIDlength];
            }
            ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availableEvent: receivedPayload="));
            ETHEREVENTQUEUE_SERIAL.println(receivedPayload);
          }
          else {  //no true payload
            receivedPayload[0] = 0;  //clear the payload buffer
          }

          if (eventAck != NULL && strcmp(receivedEvent, eventAck) == 0) {  //ack handler
            ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.availableEvent: ack received"));
            const byte receivedPayloadInt = atoi(receivedPayload);  //convert to a byte
            for (byte count = 0; count < queueSize; count++) {  //step through the currently occupied section of the eventIDqueue[]
              if (receivedPayloadInt == eventIDqueue[count] && eventTypeQueue[count] == eventTypeConfirm) {  //the ack is for the eventID of this item in the queue and the resend flag indicates it is expecting an ack(non-ack events are not removed because obviously they haven't been sent yet if they're still in the queue so the ack can't possibly be for them)
                ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.availableEvent: ack eventID match"));
                remove(count);  //remove the message from the queue
                if (queueNewCount > queueSize) {  //sanity check - if the ack incorrectly has the eventID of a new queue item then the queueNewCount value will be greater than the number of new queue items
                  queueNewCount = queueSize;
                }
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
    unsigned int availablePayload() {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availablePayload: length="));
      if (const unsigned int length = strlen(receivedPayload)) {  //strlen(receivedPayload)>0
        ETHEREVENTQUEUE_SERIAL.println(length + 1);
        return length + 1;  //length of the payload + null terminator
      }
      ETHEREVENTQUEUE_SERIAL.println(0);
      return 0;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //readEvent - places the event into the passed buffer
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void readEvent(char eventBuffer[]) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.readEvent"));
      strcpy(eventBuffer, receivedEvent);
      receivedEventLength = 0;  //enable availableEvent() to receive new events
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //readPayload - places the payload into the passed buffer
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void readPayload(char payloadBuffer[]) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.readPayload"));
      strcpy(payloadBuffer, receivedPayload);
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //receivedEventID
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    byte receivedEventID() {
      return receivedEventIDvalue;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //flushReceiver - dump the last message received so another one can be received
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void flushReceiver() {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.flushReceiver"));
      receivedEvent[0] = 0;  //reset the event buffer
      receivedPayload[0] = 0;  //reset the payload buffer
      receivedEventLength = 0;  //enable availableEvent() to receive new events
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //queue - add the relayed outgoing message to the send queue. Returns: 0==fail, 1==success, 2==success w/ queue overflow
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //convert IPAddress to 4 byte array
    byte queue(const IPAddress &targetIPAddress, const unsigned int port, const byte eventType, const char event[], const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queue(convert IPAddress): targetIPAddress="));
      ETHEREVENTQUEUE_SERIAL.println(targetIPAddress);
      byte targetIP[4];  //create buffer
      IPcopy(targetIP, targetIPAddress);  //convert
      return queue((const byte*)targetIP, port, eventType, (const char*)event, payload);
    }


    //convert node to 4 byte array
    byte queue(const byte targetNode, const unsigned int port, const byte eventType, const char event[], const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(convert node)"));
      if (targetNode >= nodeCount || !nodeIsSet(targetNode)) {  //sanity check
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(convert node): invalid node number"));
        return false;
      }
      return queue((const byte*)nodeIP[targetNode], port, eventType, (const char*)event, payload);
    }


    byte queue(byte targetIP[], const unsigned int port, const byte eventType, const char event[], const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(convert non-const byte array target to const byte array)"));
      return queue((const byte*)targetIP, port, eventType, event, payload);
    }


    //convert event
    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, char event[], const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(char event)"));
      return queue(target, port, eventType, (const char*)event, payload);
    }


    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const int8_t event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(int8_t event)"));
      return queue(target, port, eventType, (int)event, payload);  //Convert event to int. Needed to fix ambiguous overload warning.
    }


    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const byte event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(byte event)"));
      return queue(target, port, eventType, (unsigned int)event, payload);  //Convert event to int. Needed to fix ambiguous overload warning.
    }


    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const int event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(int event)"));
      char eventChar[intLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf(eventChar, "%i", event);
#else  //__ARDUINO_X86__
      itoa(event, eventChar, 10);
#endif  //__ARDUINO_X86__
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }


    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const unsigned int event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(unsigned int event)"));
      char eventChar[unsignedIntLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf (eventChar, "%u", event);
#else  //__ARDUINO_X86__
      utoa(event, eventChar, 10);
#endif  //__ARDUINO_X86__
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }


    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const long event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(long event)"));
      char eventChar[longLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf (eventChar, "%li", event);
#else  //__ARDUINO_X86__
      ltoa(event, eventChar, 10);
#endif  //__ARDUINO_X86__
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }


    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const unsigned long event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(unsigned long event)"));
      char eventChar[unsignedLongLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf (eventChar, "%lu", event);
#else  //__ARDUINO_X86__
      ultoa(event, eventChar, 10);
#endif  //__ARDUINO_X86__
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }


    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const __FlashStringHelper* event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(F() event)"));
      const byte eventLength = EtherEvent.FSHlength(event);
      char eventChar[eventLength + 1];
      memcpy_P(eventChar, event, eventLength + 1);  //+1 for the null terminator
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }


    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const String &event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(String event)"));
#ifdef __ARDUINO_X86__
      //x86 boards don't have c_str()
      byte stringLength = event.length();
      char eventChar[stringLength + 1];
      for (byte counter = 0; counter < stringLength; counter++) {
        eventChar[counter] = event[counter];
      }
      eventChar[stringLength] = 0;
      return queue(target, port, eventType, (const char*)eventChar, payload);
#else  //__ARDUINO_X86__
      return queue(target, port, eventType, (const char*)event.c_str(), payload);
#endif  //__ARDUINO_X86__
    }


    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const IPAddress &event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(IPAddress event)"));
      char eventChar[IPAddressLengthMax + 1];
      EtherEvent.IPtoa(event, eventChar);
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }


    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const double event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(double event)"));
      char eventChar[doubleIntegerLengthMax + 1 + queueDoubleDecimalPlaces + 1];  //max integer length + decimal point + decimal places setting + null terminator
#ifdef __ARDUINO_X86__
      sprintf (eventChar, "%.*f", queueDoubleDecimalPlaces, event);
#else  //__ARDUINO_X86__
      dtostrf(event, queueDoubleDecimalPlaces + 2, queueDoubleDecimalPlaces, eventChar);
#endif  //__ARDUINO_X86__
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }


    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const float event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(float event)"));
      return queue(target, port, eventType, (double)event, payload);  //needed to fix ambiguous compiler warning
    }


    //convert payload
    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const event_t event, char payload[]) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(char payload)"));
      return queue(target, port, eventType, event, (const char*)payload);
    }


    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const event_t event, const int8_t payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(char payload)"));
      return queue(target, port, eventType, event, (int)payload);
    }


    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const event_t event, const byte payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(char payload)"));
      return queue(target, port, eventType, event, (unsigned int)payload);
    }


    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const event_t event, const int payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(int payload)"));
      char payloadChar[intLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf(payloadChar, "%i", payload);
#else  //__ARDUINO_X86__
      itoa(payload, payloadChar, 10);
#endif  //__ARDUINO_X86__
      return queue(target, port, eventType, event, (const char*)payloadChar);
    }


    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const event_t event, const unsigned int payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(unsigned int payload)"));
      char payloadChar[unsignedIntLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf (payloadChar, "%u", payload);
#else  //__ARDUINO_X86__
      utoa(payload, payloadChar, 10);
#endif  //__ARDUINO_X86__
      return queue(target, port, eventType, event, payloadChar);
    }


    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const event_t event, const long payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(long payload)"));
      char payloadChar[longLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf (payloadChar, "%li", payload);
#else  //__ARDUINO_X86__
      ltoa(payload, payloadChar, 10);
#endif  //__ARDUINO_X86__
      return queue(target, port, eventType, event, (const char*)payloadChar);
    }


    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const event_t event, const unsigned long payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(unsigned long payload)"));
      char payloadChar[unsignedLongLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf (payloadChar, "%lu", payload);
#else  //__ARDUINO_X86__
      ultoa(payload, payloadChar, 10);
#endif  //__ARDUINO_X86__
      return queue(target, port, eventType, event, (const char*)payloadChar);
    }


    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, event_t event, const __FlashStringHelper* payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(F() payload)"));
      const unsigned int payloadLength = EtherEvent.FSHlength(payload);
      char payloadChar[payloadLength + 1];
      memcpy_P(payloadChar, payload, payloadLength + 1);  //+1 for the null terminator
      return queue(target, port, eventType, event, (const char*)payloadChar);
    }


    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, event_t event, const String &payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(String payload)"));
#ifdef __ARDUINO_X86__
      //x86 boards don't have c_str()
      byte stringLength = payload.length();
      char payloadChar[stringLength + 1];
      for (byte counter = 0; counter < stringLength; counter++) {
        payloadChar[counter] = payload[counter];
      }
      payloadChar[stringLength] = 0;
      return queue(target, port, eventType, event, (const char*)payloadChar);
#else  //__ARDUINO_X86__
      return queue(target, port, eventType, event, (const char*)payload.c_str());
#endif  //__ARDUINO_X86__
    }


    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, event_t event, const IPAddress &payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(String payload)"));
      char payloadChar[IPAddressLengthMax + 1];
      EtherEvent.IPtoa(payload, payloadChar);
      return queue(target, port, eventType, event, (const char*)payloadChar);
    }


    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, event_t event, const double payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(double payload)"));
      char payloadChar[doubleIntegerLengthMax + 1 + queueDoubleDecimalPlaces + 1];  //max integer length + decimal point + decimal places setting + null terminator
#ifdef __ARDUINO_X86__
      sprintf (payloadChar, "%.*f", queueDoubleDecimalPlaces, payload);
#else  //__ARDUINO_X86__
      dtostrf(payload, queueDoubleDecimalPlaces + 2, queueDoubleDecimalPlaces, payloadChar);
#endif  //__ARDUINO_X86__
      return queue(target, port, eventType, event, (const char*)payloadChar);
    }


    //main queue() function
    byte queue(const byte targetIP[], const unsigned int port, const byte eventType, const char event[], const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(main)"));
      if ((eventType != eventTypeOnce && eventType != eventTypeRepeat && eventType != eventTypeConfirm && eventType != eventTypeOverrideTimeout) || (eventType == eventTypeConfirm && eventAck == NULL)) {  //eventType sanity check
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue: invalid eventType"));
        return false;
      }
      const int targetNode = getNode(targetIP);
      if (targetNode < 0) {  //target is not a node
        if (sendNodesOnlyState == 1) {
          ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue: not a node"));
          return false;
        }
      }
      //target is a node
      else  if (targetNode == nodeDevice) {  //send events to self regardless of timeout state
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue: self send"));
        internalEventQueueCount++;
      }
      else if (millis() - nodeTimestamp[targetNode] > nodeTimeoutDuration && eventType != eventTypeOverrideTimeout) {  //is a node, not self, is timed out, and is not eventTypeOverrideTimeout
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue: timed out node"));
        return false;  //don't queue events to timed out nodes
      }

      byte success = true;  //set default success value to indicate event successfully queued in return

      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queue: queueSize="));
      ETHEREVENTQUEUE_SERIAL.println(queueSize);

      byte queueSlot;
      if (queueSize == queueSizeMax) {  //queue overflowed
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue: Queue Overflowed"));
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

      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queue: done, queueSlot="));
      ETHEREVENTQUEUE_SERIAL.println(queueSlot);
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queue: queueNewCount="));
      ETHEREVENTQUEUE_SERIAL.println(queueNewCount);
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queue: IP="));
      ETHEREVENTQUEUE_SERIAL.println(IPAddress(IPqueue[queueSlot]));
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queue: port="));
      ETHEREVENTQUEUE_SERIAL.println(portQueue[queueSlot]);
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queue: event="));
      ETHEREVENTQUEUE_SERIAL.println(eventQueue[queueSlot]);
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queue: payload="));
      ETHEREVENTQUEUE_SERIAL.println(payloadQueue[queueSlot]);
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queue: eventID="));
      ETHEREVENTQUEUE_SERIAL.println(eventIDqueue[queueSlot]);
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queue: eventType="));
      ETHEREVENTQUEUE_SERIAL.println(eventTypeQueue[queueSlot]);
      return success;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //queueHandler - Sends out the messages in the queue. This function was moved to the header file so that the authentication disable system would work
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    boolean queueHandler(EthernetClient &ethernetClient) {
      if (queueSize > internalEventQueueCount && (queueNewCount > 0 || millis() - queueSendTimestamp > resendDelay)) {  //there are events in the queue that are non-internal events and it is time(if there are new queue items then send immediately or if resend wait for the resendDelay)
        ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: queueSize="));
        ETHEREVENTQUEUE_SERIAL.println(queueSize);
        ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: queueNewCount="));
        ETHEREVENTQUEUE_SERIAL.println(queueNewCount);
        byte queueSlotSend = 0;  //This is used to store the slot. Initialized to 0 to fix "may be uninitialized" compiler warning.
        int8_t targetNode;
        for (byte counter = 0; counter < queueSize; counter++) {  //the maximum number of iterations is the queueSize
          if (queueNewCount == 0) {  //time to send the next one in the queue
            //find the next largest priority level value
            if (queuePriorityLevel == queueSize - 1 || queueIndex[queuePriorityLevel + 1] == -1) {  //the last sent item was already at the largest priority level value so send the queue item with smallest priority level value. The first statment(queuePriorityLevel == queueSize - 1) handles reaching the end of a full queue, the second statement(queueIndex[queuePriorityLevel + 1] == -1) handles reaching the empty portion of a partially filled queue
              queuePriorityLevel = 0;  //start from the least recently queued item
            }
            else {
              queuePriorityLevel++;  //go on to the next most recently queued item
            }
            queueSlotSend = queueIndex[queuePriorityLevel];
            queueSendTimestamp = millis();  //reset the timestamp to delay the next queue resend
          }
          else {  //send the oldest new item in the queue
            queueSlotSend = queueIndex[queueSize - queueNewCount];  //find the (queueNewCount)th largest priority level value
            queueNewCount--;
          }
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: queueSlotSend="));
          ETHEREVENTQUEUE_SERIAL.println(queueSlotSend);
          targetNode = getNode(IPqueue[queueSlotSend]);  //get the node of the target IP
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: targetNode="));
          ETHEREVENTQUEUE_SERIAL.println(targetNode);
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: nodeDevice="));
          ETHEREVENTQUEUE_SERIAL.println(nodeDevice);
          if (targetNode == nodeDevice) {  //ignore internal events, they are sent in availableEvent()
            ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queueHandler: nodeDevice=targetNode"));
            continue;  //move on to the next queue step
          }
          if (targetNode < 0) {  //-1 indicates no node match
            ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queueHandler: non-node targetIP"));
            break;  //non-nodes never timeout
          }

          if (millis() - nodeTimestamp[targetNode] < nodeTimeoutDuration || eventTypeQueue[queueSlotSend] == eventTypeOverrideTimeout) {  //non-timed out node or eventTypeOverrideTimeout
            break;  //continue with the message send
          }
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: targetNode timed out for queue#="));
          ETHEREVENTQUEUE_SERIAL.println(queueSlotSend);
          remove(queueSlotSend);  //dump messages for dead nodes from the queue
          if (queueSize == 0) {  //no events left to send
            return true;
          }
        }

        //set up the raw payload
        char payload[strlen(payloadQueue[queueSlotSend]) + eventIDlength + 1];
#ifdef __ARDUINO_X86__
        sprintf(payload, "%i", eventIDqueue[queueSlotSend]);
#else  //__ARDUINO_X86__
        itoa(eventIDqueue[queueSlotSend], payload, 10);  //put the message ID on the start of the payload
#endif  //__ARDUINO_X86__
        strcat(payload, payloadQueue[queueSlotSend]);  //add the true payload to the payload string

        ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: targetIP="));
        ETHEREVENTQUEUE_SERIAL.println(IPAddress(IPqueue[queueSlotSend]));
        ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: event="));
        ETHEREVENTQUEUE_SERIAL.println(eventQueue[queueSlotSend]);
        ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: payload="));
        ETHEREVENTQUEUE_SERIAL.println(payload);

        if (EtherEvent.send(ethernetClient, (const byte*)IPqueue[queueSlotSend], portQueue[queueSlotSend], (const char*)eventQueue[queueSlotSend], (const char*)payload) > 0) {
          ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queueHandler: send successful"));
          nodeTimestamp[nodeDevice] = millis();  //set the device timestamp(using the nodeDevice because that part of the array is never used otherwise)
          //update timestamp of the target node
          targetNode = getNode(IPqueue[queueSlotSend]);  //get the node of the senderIP
          if (targetNode >= 0) {  //the target IP is a node(-1 indicates no node match)
            nodeTimestamp[targetNode] = nodeTimestamp[nodeDevice];  //set the individual timestamp, any communication is considered to be a received keepalive - the nodeTimestamp for the device has just been set so I am using that variable so I don't have to call millis() twice for efficiency
            sendKeepaliveTimestamp[targetNode] = nodeTimestamp[nodeDevice] - sendKeepaliveResendDelay;  //Treat successful send of any event as a sent keepalive so delay the send of the next keepalive. -sendKeepaliveResendDelay is so that sendKeepalive() will be able to queue the eventKeepalive according to "millis() - nodeTimestamp[node] > nodeTimeoutDuration - sendKeepaliveMargin" without being blocked by the "millis() - sendKeepaliveTimestamp[node] > sendKeepaliveResendDelay", it will not cause immediate queue of eventKeepalive because nodeTimestamp[targetNode] has just been set. The nodeTimestamp for the device has just been set so I am using that variable so I don't have to call millis() again
            if (nodeState[targetNode] == nodeStateUnknown) {
              nodeState[targetNode] = nodeStateActive;  //set the node state to active
            }
          }

          if (eventTypeQueue[queueSlotSend] != eventTypeConfirm) {  //the flag indicates not to wait for an ack
            ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queueHandler: eventType != eventTypeConfirm, event removed from queue"));
            remove(queueSlotSend);  //remove the message from the queue immediately
          }
          return true;  //indicate send success
        }
        else {  //send failed
          ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queueHandler: send failed"));
          if (eventTypeQueue[queueSlotSend] == eventTypeOnce || eventTypeQueue[queueSlotSend] == eventTypeOverrideTimeout) {  //the flag indicates not to resend even after failure
            remove(queueSlotSend);  //remove keepalives even when send was not successful. This is because the keepalives are sent even to timed out nodes so they shouldn't be queued.
          }
          return false;  //indicate send failed
        }
      }
      return true;  //indicate no send required
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //flushQueue - removes all events from the queue
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void flushQueue() {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.flushQueue"));
      queueSize = 0;
      queueNewCount = 0;
      internalEventQueueCount = 0;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //checkTimeout - checks all the nodes until it finds a _NEWLY_ timed out node and returns it and then updates the nodeState value for that node. If no nodes are newly timed out then this function returns -1.  Note that this works differently than checkState()
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int8_t checkTimeout() {
      for (byte node = 0; node < nodeCount; node++) {
        if (!nodeIsSet(node)) {  //node has not been set
          continue;
        }
        if (nodeState[node] == nodeStateActive && millis() - nodeTimestamp[node] > nodeTimeoutDuration) {  //previous state not timed out, and is currently timed out
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.checkTimeout: timed out node="));
          ETHEREVENTQUEUE_SERIAL.println(node);
          nodeState[node] = nodeStateTimedOut;  //set the node state to inactive
          return node;
        }
      }
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.checkTimeout: no newly timed out nodes"));
      return -1;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //checkTimein - checks all the authorized IPs until it finds a _NEWLY_ timed in node and returns it and then updates the nodeState value for that node. If no nodes are newly timed in then this function returns -1.  Note that this works differently than checkState()
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int8_t checkTimein() {
      for (byte node = 0; node < nodeCount; node++) {
        if (!nodeIsSet(node)) {  //node has not been set
          continue;
        }
        if (nodeState[node] == nodeStateTimedOut && millis() - nodeTimestamp[node] < nodeTimeoutDuration) {  //node is newly timed in(since the last time the function was run)
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.checkTimein: timed in node="));
          ETHEREVENTQUEUE_SERIAL.println(node);
          nodeState[node] = nodeStateActive;  //set the node state to active
          return node;
        }
      }
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.checkTimein: no newly timed in nodes"));
      return -1;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //checkState - checks if the given node is timed out. Note that this doesn't update the nodeState like checkTimeout()/checkTimein().
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int8_t checkState(const byte node) {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.checkTimeoutNode: nodeState for node "));
      ETHEREVENTQUEUE_SERIAL.print(node);
      ETHEREVENTQUEUE_SERIAL.print(F("="));
      if (node > nodeCount - 1) {  //sanity check
        ETHEREVENTQUEUE_SERIAL.println(F("invalid node number"));
        return -1;
      }
      if (millis() - nodeTimestamp[node] > nodeTimeoutDuration) {  //node is not this device, not already timed out, and is timed out
        ETHEREVENTQUEUE_SERIAL.println(F("timed out"));
        return false;
      }
      ETHEREVENTQUEUE_SERIAL.println(F("not timed out"));
      return true;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //getNode - template that can accept IPAddress or byte array type parameters - this function must be defined in the .h instead of the .cpp because it is a template
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename IP_t>
    int8_t getNode(const IP_t &IPvalue) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.getNode"));
      for (byte node = 0; node < nodeCount; node++) {  //step through all the nodes
        byte octet;
        for (octet = 0; octet < 4; octet++) {
          if (nodeIP[node][octet] != IPvalue[octet]) {  //mismatch
            octet = 0;
            break;  //check the next node for a match
          }
        }
        if (octet == 4) {  //node matched
          //ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.getNode: node found="));
          //ETHEREVENTQUEUE_SERIAL.println(node);
          return node;
        }
      }
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.getNode: node not found"));
      return -1;  //no match
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //checkQueueOverflow
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    boolean checkQueueOverflow() {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.checkQueueOverflow: queueOverflowFlag="));
      ETHEREVENTQUEUE_SERIAL.println(queueOverflowFlag);
      const byte queueOverflowFlagValue = queueOverflowFlag;  //save the value before resetting it
      queueOverflowFlag = false;  //reset the flag
      return queueOverflowFlagValue;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //setResendDelay
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setResendDelay(const unsigned long resendDelayValue) {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.setResendDelay: resendDelay="));
      ETHEREVENTQUEUE_SERIAL.println(resendDelayValue);
      resendDelay = resendDelayValue;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //getResendDelay
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    unsigned long getResendDelay() {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.getResendDelay: resendDelay="));
      ETHEREVENTQUEUE_SERIAL.println(resendDelay);
      return resendDelay;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //setNodeTimeoutDuration
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setNodeTimeoutDuration(const unsigned long nodeTimeoutDurationValue) {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.setNodeTimeoutDuration: nodeTimeoutDuration="));
      ETHEREVENTQUEUE_SERIAL.println(nodeTimeoutDurationValue);
      nodeTimeoutDuration = nodeTimeoutDurationValue;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //getNodeTimeoutDuration
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    unsigned long getNodeTimeoutDuration() {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.getNodeTimeoutDuration: nodeTimeoutDuration="));
      ETHEREVENTQUEUE_SERIAL.println(nodeTimeoutDuration);
      return nodeTimeoutDuration;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //receiveNodesOnly
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef ethernetclientwithremoteIP_h
    void receiveNodesOnly(const boolean receiveNodesOnlyValue = true) {
      receiveNodesOnlyState = receiveNodesOnlyValue;
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.receiveNodesOnly: new state="));
      ETHEREVENTQUEUE_SERIAL.println(receiveNodesOnlyState);
    }
#endif  //ethernetclientwithremoteIP_h


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //sendNodesOnly
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void sendNodesOnly(const boolean sendNodesOnlyValue = true) {
      sendNodesOnlyState = sendNodesOnlyValue;
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.receiveNodesOnly: new state="));
      ETHEREVENTQUEUE_SERIAL.println(sendNodesOnlyState);
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //setNode
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename IP_t>
    boolean setNode(const byte nodeNumber, const IP_t &nodeIPaddress) {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.setNode: node="));
      ETHEREVENTQUEUE_SERIAL.println(nodeNumber);
      if (nodeNumber >= nodeCount) {  //sanity check
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.setNode: invalid node number"));
        return false;
      }
      //write the device IP address to nodeIP
      for (byte counter = 0; counter < 4; counter++) {
        nodeIP[nodeNumber][counter] = nodeIPaddress[counter];
      }
      nodeTimestamp[nodeNumber] = millis();
      sendKeepaliveTimestamp[nodeNumber] = millis() - sendKeepaliveResendDelay;
      nodeState[nodeNumber] = nodeStateUnknown;  //start in unknown state
      return true;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //removeNode
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void removeNode(const byte nodeNumber) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.removeNode"));
      if (nodeNumber >= nodeCount) {  //sanity check
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.removeNode: invalid node number"));
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
    IPAddress getIP(const byte nodeNumber) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.getIP"));
      if (nodeNumber >= nodeCount) {  //sanity check
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.getIP: invalid node number"));
        return IPAddress(0, 0, 0, 0);
      }
      else {
        return IPAddress(nodeIP[nodeNumber][0], nodeIP[nodeNumber][1], nodeIP[nodeNumber][2], nodeIP[nodeNumber][3]);
      }
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //setSendKeepaliveMargin
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setSendKeepaliveMargin(const unsigned long sendKeepaliveMarginInput) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.setSendKeepaliveMargin"));
      sendKeepaliveMargin = min(sendKeepaliveMarginInput, nodeTimeoutDuration);  //sendKeepaliveMargin can't be greater than nodeTimeoutDuration
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //getSendKeepaliveMargin
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    unsigned long getSendKeepaliveMargin() {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.getSendKeepaliveMargin"));
      return sendKeepaliveMargin;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //setSendKeepaliveResendDelay
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setSendKeepaliveResendDelay(const unsigned long sendKeepaliveResendDelayInput) {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.setSendKeepaliveResendDelay: sendKeepaliveResendDelayInput="));
      ETHEREVENTQUEUE_SERIAL.println(sendKeepaliveResendDelayInput);
      sendKeepaliveResendDelay = sendKeepaliveResendDelayInput;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //getSendKeepaliveResendDelay
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    unsigned long getSendKeepaliveResendDelay() {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.getSendKeepaliveResendDelay, sendKeepaliveResendDelay="));
      ETHEREVENTQUEUE_SERIAL.println(sendKeepaliveResendDelay);
      return sendKeepaliveResendDelay;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //setEventKeepalive
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    boolean setEventKeepalive(const char eventKeepaliveInput[]) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.setEventKeepalive"));
      const byte eventKeepaliveLength = strlen(eventKeepaliveInput);
      eventKeepalive = (char*)realloc(eventKeepalive, (eventKeepaliveLength + 1) * sizeof(*eventKeepalive));  //allocate memory
      if (eventKeepalive == NULL) {
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.setEventKeepalive: memory allocation failed"));
        return false;
      }
      strcpy(eventKeepalive, eventKeepaliveInput);  //store the event
      return true;
    }


    boolean setEventKeepalive(const int eventKeepaliveInput) {
      char eventKeepaliveInputChar[intLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf(eventKeepaliveInputChar, "%i", eventKeepaliveInput);
#else  //__ARDUINO_X86__
      itoa(eventKeepaliveInput, eventKeepaliveInputChar, 10);
#endif  //__ARDUINO_X86__
      return setEventKeepalive(eventKeepaliveInputChar);
    }


    boolean setEventKeepalive(const unsigned int eventKeepaliveInput) {
      char eventKeepaliveInputChar[unsignedIntLengthMax + 1];
      sprintf_P(eventKeepaliveInputChar, PSTR("%u"), eventKeepaliveInput);
      return setEventKeepalive(eventKeepaliveInputChar);
    }


    boolean setEventKeepalive(const long eventKeepaliveInput) {
      char eventKeepaliveInputChar[longLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf (eventKeepaliveInputChar, "%li", eventKeepaliveInput);
#else  //__ARDUINO_X86__
      ltoa(eventKeepaliveInput, eventKeepaliveInputChar, 10);
#endif  //__ARDUINO_X86__
      return setEventKeepalive(eventKeepaliveInputChar);
    }


    boolean setEventKeepalive(const unsigned long eventKeepaliveInput) {
      char eventKeepaliveInputChar[unsignedLongLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf (eventKeepaliveInputChar, "%lu", eventKeepaliveInput);
#else  //__ARDUINO_X86__
      ultoa(eventKeepaliveInput, eventKeepaliveInputChar, 10);
#endif  //__ARDUINO_X86__
      return setEventKeepalive(eventKeepaliveInputChar);
    }


    boolean setEventKeepalive(const double eventKeepalive) {
      char eventKeepaliveChar[doubleIntegerLengthMax + 1 + queueDoubleDecimalPlaces + 1];  //max integer length + decimal point + decimal places setting + null terminator
#ifdef __ARDUINO_X86__
      sprintf (eventKeepaliveChar, "%.*f", queueDoubleDecimalPlaces, eventKeepalive);
#else  //__ARDUINO_X86__
      dtostrf(eventKeepalive, queueDoubleDecimalPlaces + 2, queueDoubleDecimalPlaces, eventKeepaliveChar);
#endif  //__ARDUINO_X86__
      return setEventKeepalive(eventKeepaliveChar);
    }


    boolean setEventKeepalive(const char eventKeepaliveInput) {
      char eventKeepaliveInputChar[] = {eventKeepaliveInput, 0};
      return setEventKeepalive(eventKeepaliveInputChar);
    }


    boolean setEventKeepalive(const __FlashStringHelper* eventKeepaliveFSH) {
      byte stringLength = EtherEvent.FSHlength(eventKeepaliveFSH);
      char eventKeepaliveChar[stringLength + 1];
      memcpy_P(eventKeepaliveChar, eventKeepaliveFSH, stringLength + 1);  //+1 for the null terminator
      return setEventKeepalive(eventKeepaliveChar);
    }


    boolean setEventKeepalive(const String &eventKeepaliveInput) {
#ifdef __ARDUINO_X86__
      //x86 boards don't have c_str()
      const byte stringLength = eventKeepaliveInput.length();
      char eventKeepaliveInputChar[stringLength + 1];
      for (byte counter = 0; counter < stringLength; counter++) {
        eventKeepaliveInputChar[counter] = eventKeepaliveInput[counter];
      }
      eventKeepaliveInputChar[stringLength] = 0;
      return setEventKeepalive(eventKeepaliveInputChar);
#else  //__ARDUINO_X86__
      return setEventKeepalive(eventKeepaliveInput.c_str());
#endif  //__ARDUINO_X86__
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //sendKeepalive
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void sendKeepalive(const unsigned int port) {
      if (eventKeepalive == NULL) {
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.sendKeepalive: eventKeepalive not set"));
        return;
      }
      for (byte node = 0; node < nodeCount; node++) {
        if (node == nodeDevice || !nodeIsSet(node)) {  //device node or node has not been set
          continue;
        }
        if (millis() - nodeTimestamp[node] > nodeTimeoutDuration - sendKeepaliveMargin && millis() - sendKeepaliveTimestamp[node] > sendKeepaliveResendDelay) {  //node is newly timed out(since the last time the function was run)
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.sendKeepalive: sending to node="));
          ETHEREVENTQUEUE_SERIAL.println(node);
          queue(node, port, eventTypeOverrideTimeout, eventKeepalive);
          sendKeepaliveTimestamp[node] = millis();
        }
      }
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.sendKeepalive: no keepalive sent"));
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //setEventAck
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    boolean setEventAck(const char eventAckInput[]) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.setEventAck"));
      const byte eventAckLength = strlen(eventAckInput);
      eventAck = (char*)realloc(eventAck, (eventAckLength + 1) * sizeof(*eventAck));  //allocate memory
      if (eventAck == NULL) {
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.setEventAck: memory allocation failed"));
        return false;
      }
      strcpy(eventAck, eventAckInput);  //store the event
      return true;
    }


    boolean setEventAck(const int eventAckInput) {
      char eventAckInputChar[intLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf(eventAckInputChar, "%i", eventAckInput);
#else  //__ARDUINO_X86__
      itoa(eventAckInput, eventAckInputChar, 10);
#endif  //__ARDUINO_X86__
      return setEventAck(eventAckInputChar);
    }


    boolean setEventAck(const unsigned int eventAckInput) {
      char eventAckInputChar[unsignedIntLengthMax + 1];
      sprintf_P(eventAckInputChar, PSTR("%u"), eventAckInput);
      return setEventAck(eventAckInputChar);
    }


    boolean setEventAck(const long eventAckInput) {
      char eventAckInputChar[longLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf (eventAckInputChar, "%li", eventAckInput);
#else  //__ARDUINO_X86__
      ltoa(eventAckInput, eventAckInputChar, 10);
#endif  //__ARDUINO_X86__
      return setEventAck(eventAckInputChar);
    }


    boolean setEventAck(const unsigned long eventAckInput) {
      char eventAckInputChar[unsignedLongLengthMax + 1];
#ifdef __ARDUINO_X86__
      sprintf (eventAckInputChar, "%lu", eventAckInput);
#else  //__ARDUINO_X86__
      ultoa(eventAckInput, eventAckInputChar, 10);
#endif  //__ARDUINO_X86__
      return setEventAck(eventAckInputChar);
    }


    boolean setEventAck(const double eventAckInput) {
      char eventAckInputChar[doubleIntegerLengthMax + 1 + queueDoubleDecimalPlaces + 1];  //max integer length + decimal point + decimal places setting + null terminator
#ifdef __ARDUINO_X86__
      sprintf (eventAckInputChar, "%.*f", queueDoubleDecimalPlaces, eventAckInput);
#else  //__ARDUINO_X86__
      dtostrf(eventAckInput, queueDoubleDecimalPlaces + 2, queueDoubleDecimalPlaces, eventAckInputChar);
#endif  //__ARDUINO_X86__
      return setEventAck(eventAckInputChar);
    }


    boolean setEventAck(const char eventAckInput) {
      char eventAckInputChar[] = {eventAckInput, 0};
      return setEventAck(eventAckInputChar);
    }


    boolean setEventAck(const __FlashStringHelper* eventAckFSH) {
      byte stringLength = EtherEvent.FSHlength(eventAckFSH);
      char eventAckChar[stringLength + 1];
      memcpy_P(eventAckChar, eventAckFSH, stringLength + 1);  //+1 for the null terminator
      return setEventAck(eventAckChar);
    }


    boolean setEventAck(const String &eventAckInput) {
#ifdef __ARDUINO_X86__
      //x86 boards don't have c_str()
      const byte stringLength = eventAckInput.length();
      char eventAckInputChar[stringLength + 1];
      for (byte counter = 0; counter < stringLength; counter++) {
        eventAckInputChar[counter] = eventAckInput[counter];
      }
      eventAckInputChar[stringLength] = 0;
      return setEventAck(eventAckInputChar);
#else  //__ARDUINO_X86__
      return setEventAck(eventAckInput.c_str());
#endif  //__ARDUINO_X86__
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //setQueueDoubleDecimalPlaces - set the number of decimal places to queue of double/float event/payload
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void setQueueDoubleDecimalPlaces(byte decimalPlaces) {
      queueDoubleDecimalPlaces = decimalPlaces;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
  private:
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //private constants - these are constants that need to be accessed in this file so they can't be defined in EtherEventQueue.cpp
    static const byte uint16_tLengthMax = 5;  //5 digits
    static const byte int16_tLengthMax = 1 + uint16_tLengthMax;  //sign + 5 digits
    static const byte uint32_tLengthMax = 10;  //10 digits
    static const byte int32_tLengthMax = 1 + uint32_tLengthMax;  //sign + 10 digits
#if UINT_MAX <= 65535
    static const byte unsignedIntLengthMax = uint16_tLengthMax;
#else  //UINT_MAX <= 65535
    static const byte unsignedIntLengthMax = uint32_tLengthMax;
#endif  //UINT_MAX <= 65535
#if INT_MIN >= -32767
    static const byte intLengthMax = int16_tLengthMax;
#else  //INT_MIN >= -32767
    static const byte intLengthMax = int32_tLengthMax;
#endif  //INT_MIN >= -32767
    static const byte unsignedLongLengthMax = uint32_tLengthMax;
    static const byte longLengthMax = int32_tLengthMax;
    static const byte IPAddressLengthMax = 3 + 1 + 3 + 1 + 3 + 1 + 3;  //4 x octet + 3 x dot
    static const byte doubleIntegerLengthMax = 40;  //sign + 39 digits max (-1000000000000000000000000000000000000000 gives me "floating constant exceeds range of 'double'" warning)

    static const byte nodeStateTimedOut = 0;
    static const byte nodeStateActive = 1;
    static const byte nodeStateUnknown = 2;

    static const byte eventIDlength = 2;

    static const unsigned long nodeTimeoutDurationDefault = 270000;  //(ms)the node is timed out if it has been longer than this duration since the last event was received from it
    static const unsigned long sendKeepaliveMarginDefault = 30000;
    static const unsigned long sendKeepaliveResendDelayDefault = 60000;
    static const unsigned int resendDelayDefault = 45000;  //(ms)delay between resends of messages

    static const byte queueSizeMaxDefault = 5;
    static const byte eventLengthMaxDefault = 15;
    static const byte payloadLengthMaxDefault = 80;

    const byte queueDoubleDecimalPlacesDefault = 3;

    //private global variables
    byte nodeDevice;
    unsigned int defaultPort;
    byte receivedEventLengthMax;
    byte receivedEventIDvalue;
    char* receivedEvent;  //buffer to hold the received event
    unsigned int receivedPayloadLengthMax;
    char* receivedPayload;  //buffer to hold the received payload
    byte receivedEventLength;
    boolean receiveNodesOnlyState;  //restrict event receiving to nodes only

    byte queueSizeMax;
    int8_t* queueIndex;
    byte** IPqueue;  //queue buffers
    unsigned int* portQueue;
    byte sendEventLengthMax;
    char** eventQueue;
    byte* eventIDqueue;  //unique identifier for the message
    unsigned int sendPayloadLengthMax;
    char** payloadQueue;
    byte* eventTypeQueue;

    byte queueNewCount;  //number of new messages in the queue
    byte internalEventQueueCount;
    byte queueSize;  //how many messages are currently in the send queue
    byte queuePriorityLevel;  //the priority level of the last event sent
    unsigned long queueSendTimestamp;  //used for delayed resends of messages in the queue that failed the first time
    byte queueOverflowFlag;
    unsigned long resendDelay;
    boolean sendNodesOnlyState;  //restrict event sending to nodes only

    byte nodeCount;
    byte** nodeIP;
    byte* nodeState;  //1=not timed out 0=timed out - state at the last check
    unsigned long* nodeTimestamp;
    unsigned long* sendKeepaliveTimestamp;
    unsigned long sendKeepaliveResendDelay;
    unsigned long nodeTimeoutDuration;
    unsigned long sendKeepaliveMargin;

    char* eventKeepalive;
    char* eventAck;

    byte queueDoubleDecimalPlaces;


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //eventIDfind - find a free eventID
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    byte eventIDfind() {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.eventIDfind"));
      if (queueSize == 0) {  //the queue is empty
        ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.eventIDfind: eventID=10"));
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
            ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.eventIDfind: eventID="));
            ETHEREVENTQUEUE_SERIAL.println(eventID);
            return eventID;  //skip the rest of the for loop
          }
        }
      }
      return 0;  //this should never happen but it causes a compiler warning without
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //remove - remove the given item from the queue
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void remove(const byte removeQueueSlot) {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.remove: queueSlot="));
      ETHEREVENTQUEUE_SERIAL.println(removeQueueSlot);
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
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.remove: new queue size="));
      ETHEREVENTQUEUE_SERIAL.println(queueSize);
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //IPcopy - copies IPAddress or 4 byte array to 4 byte array - IPdestination will contain the converted IPsource after this function is called
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename IPdestination_t, typename IPsource_t>
    void IPcopy(IPdestination_t &IPdestination, const IPsource_t &IPsource) {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.IPcopy: IPsource="));
      ETHEREVENTQUEUE_SERIAL.println(IPAddress(IPsource));
      for (byte counter = 0; counter < 4; counter++) {
        IPdestination[counter] = IPsource[counter];
      }
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.IPcopy: IPdestination="));
      ETHEREVENTQUEUE_SERIAL.println(IPAddress(IPdestination));
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //nodeIsSet - check if the node has been set
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    boolean nodeIsSet(const byte nodeNumber) {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.nodeIsSet: result="));
      if (nodeIP[nodeNumber][0] == 0 && nodeIP[nodeNumber][1] == 0 && nodeIP[nodeNumber][2] == 0 && nodeIP[nodeNumber][3] == 0) {
        ETHEREVENTQUEUE_SERIAL.println(F("false"));
        return false;
      }
      ETHEREVENTQUEUE_SERIAL.println(F("true"));
      return true;
    }
};


static EtherEventQueueClass EtherEventQueue;  //declare the class so it doesn't have to be done in the sketch

#endif
