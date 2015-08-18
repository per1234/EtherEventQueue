// EtherEventQueue - outgoing event queue for the EtherEvent authenticated network communication Arduino library: http://github.com/per1234/EtherEvent
#ifndef EtherEventQueue_h
#define EtherEventQueue_h
#include <Arduino.h>
#ifndef ARDUINO_ARCH_AVR
#include <avr/dtostrf.h>
#endif
#include "EtherEvent.h"


#define ETHEREVENTQUEUE_DEBUG false  //(false == serial debug output off,  true == serial debug output on)The serial debug output will increase memory usage and communication latency so only enable when needed.
#define ETHEREVENTQUEUE_SERIAL if(ETHEREVENTQUEUE_DEBUG)Serial  //I have to use a different name for Serial in this file otherwise the debug statement control also affects any other file that includes this file.


class EtherEventQueueClass {
  public:
    //public constants
    static const byte eventTypeOnce = 0;
    static const byte eventTypeRepeat = 1;
    static const byte eventTypeConfirm = 2;
    static const byte eventTypeOverrideTimeout = 3;

    static const byte queueSuccessOverflow = 2;

    EtherEventQueueClass();

    boolean begin();
    boolean begin(const byte queueSizeMaxInput, const byte sendEventLengthMaxInput, const unsigned int sendPayloadLengthMaxInput, const byte receivedEventLengthMaxInput, const unsigned int receivedPayloadLengthMaxInput);
    boolean begin(const byte nodeDeviceInput, const byte nodeCountInput);
    boolean begin(const byte nodeDeviceInput, byte nodeCountInput, byte queueSizeMaxInput, const byte sendEventLengthMaxInput, const unsigned int sendPayloadLengthMaxInput, const byte receivedEventLengthMaxInput, const unsigned int receivedPayloadLengthMaxInput);


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
        if (const byte availableBytesEvent = EtherEvent.availableEvent(ethernetServer, cookieInput)) {  //there is a new event
#else
        if (const byte availableBytesEvent = EtherEvent.availableEvent(ethernetServer)) {  //there is a new event
#endif
          ETHEREVENTQUEUE_SERIAL.println(F("---------------------------"));
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availableEvent: EtherEvent.availableEvent()="));
          ETHEREVENTQUEUE_SERIAL.println(availableBytesEvent);
#ifdef ethernetclientwithremoteIP_h  //this function is only available if the modified Ethernet library is installed
          ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.availableEvent: remoteIP="));
          ETHEREVENTQUEUE_SERIAL.println(EtherEvent.senderIP());
#endif

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
#endif

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


    unsigned int availablePayload();
    void readEvent(char eventBuffer[]);
    void readPayload(char payloadBuffer[]);

    byte receivedEventID();
    void flushReceiver();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //queue
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    byte queue(const byte targetIP[], const unsigned int port, const byte eventType, const char event[], const char payload[] = "");  //convert IPAddress to 4 byte array
    byte queue(const byte targetNode, const unsigned int port, const byte eventType, const char event[], const char payload[] = "");  //convert node number to 4 byte array
    byte queue(const IPAddress &targetIPAddress, const unsigned int port, const byte eventType,  const char event[], const char payload[] = "");  //main queue prototype

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
      return queue(target, port, eventType, (int)event, payload);  //Convert event to int. Needed to fix ambiguous overload warning.
    }
    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const int16_t event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(int event)"));
      char eventChar[int16_tLengthMax + 1];
      itoa(event, eventChar, 10);
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }
    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const uint16_t event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(unsigned int event)"));
      char eventChar[uint16_tLengthMax + 1];
      utoa(event, eventChar, 10);
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }
    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const int32_t event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(long event)"));
      char eventChar[int32_tLengthMax + 1];
      ltoa(event, eventChar, 10);
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }
    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const uint32_t event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(unsigned long event)"));
      char eventChar[uint32_tLengthMax + 1];
      ultoa(event, eventChar, 10);
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }
    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const __FlashStringHelper* event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(F() event)"));
      char eventChar[sendEventLengthMax + 1];
      FSHtoa(event, eventChar, sendEventLengthMax);
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }
    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const String &event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(String event)"));
      byte stringLength = event.length();
      char eventChar[stringLength + 1];
      for (byte counter = 0; counter < stringLength; counter++) {
        eventChar[counter] = event[counter];  //I could probably just use c_str() instead but then I have to deal with the pointer
      }
      eventChar[stringLength] = 0;
      return queue(target, port, eventType, (const char*)eventChar, payload);
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
      dtostrf(event, queueDoubleDecimalPlaces + 2, queueDoubleDecimalPlaces, eventChar);
      return queue(target, port, eventType, (const char*)eventChar, payload);
    }
    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const float event, const char payload[] = "") {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(float event)"));
      return queue(target, port, eventType, (double)event, payload);  //needed to fix ambiguous compiler warning
    }

    //convert payload
    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const char event[], char payload[]) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(char payload)"));
      return queue(target, port, eventType, event, (const char*)payload);
    }
    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const event_t event, const int16_t payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(int payload)"));
      char payloadChar[int16_tLengthMax + 1];
      itoa(payload, payloadChar, 10);
      return queue(target, port, eventType, event, payloadChar);
    }
    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const event_t event, const uint16_t payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(unsigned int payload)"));
      char payloadChar[uint16_tLengthMax + 1];
      utoa(payload, payloadChar, 10);
      return queue(target, port, eventType, event, payloadChar);
    }
    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const event_t event, const int32_t payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(long payload)"));
      char payloadChar[int32_tLengthMax + 1];
      ltoa(payload, payloadChar, 10);
      return queue(target, port, eventType, event, payloadChar);
    }
    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, const event_t event, const uint32_t payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(unsigned long payload)"));
      char payloadChar[uint32_tLengthMax + 1];
      ultoa(payload, payloadChar, 10);
      return queue(target, port, eventType, event, payloadChar);
    }
    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, event_t event, const __FlashStringHelper* payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(F() payload)"));
      char payloadChar[sendPayloadLengthMax + 1];
      FSHtoa(payload, payloadChar, sendPayloadLengthMax);
      return queue(target, port, eventType, event, payloadChar);
    }
    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, event_t event, const String &payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(String payload)"));
      byte stringLength = payload.length();
      char payloadChar[stringLength + 1];
      for (byte counter = 0; counter < stringLength; counter++) {
        payloadChar[counter] = payload[counter];  //I could probably just use c_str() instead but then I have to deal with the pointer
      }
      payloadChar[stringLength] = 0;
      return queue(target, port, eventType, event, payloadChar);
    }
    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, event_t event, const IPAddress &payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(String payload)"));
      char payloadChar[IPAddressLengthMax + 1];
      EtherEvent.IPtoa(payload, payloadChar);
      return queue(target, port, eventType, event, payloadChar);
    }
    template <typename target_t, typename event_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, event_t event, const double payload) {
      ETHEREVENTQUEUE_SERIAL.println(F("EtherEventQueue.queue(double payload)"));
      char payloadChar[doubleIntegerLengthMax + 1 + queueDoubleDecimalPlaces + 1];  //max integer length + decimal point + decimal places setting + null terminator
      dtostrf(payload, queueDoubleDecimalPlaces + 2, queueDoubleDecimalPlaces, payloadChar);
      return queue(target, port, eventType, event, payloadChar);
    }

    //convert event and payload
    template <typename target_t>
    byte queue(const target_t &target, const unsigned int port, const byte eventType, char event[], char payload[]) {
      return queue(target, port, eventType, (const char*)event, (const char*)payload);
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

          if (millis() - nodeTimestamp[targetNode] < nodeTimeoutDuration || eventTypeQueue[queueSlotSend] == eventTypeOverrideTimeout) { //non-timed out node or eventTypeOverrideTimeout
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
        itoa(eventIDqueue[queueSlotSend], payload, 10);  //put the message ID on the start of the payload
        strcat(payload, payloadQueue[queueSlotSend]);  //add the true payload to the payload string

        ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: targetIP="));
        ETHEREVENTQUEUE_SERIAL.println(IPAddress(IPqueue[queueSlotSend]));
        ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: event="));
        ETHEREVENTQUEUE_SERIAL.println(eventQueue[queueSlotSend]);
        ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.queueHandler: payload="));
        ETHEREVENTQUEUE_SERIAL.println(payload);

        if (EtherEvent.send(ethernetClient, (const byte*)IPqueue[queueSlotSend], portQueue[queueSlotSend], (const char*)eventQueue[queueSlotSend], payload) > 0) {
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


    void flushQueue();
    int8_t checkTimeout();
    int8_t checkTimein();
    int8_t checkState(const byte node);


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


    boolean checkQueueOverflow();
    void setResendDelay(const unsigned long resendDelayValue);
    unsigned long getResendDelay();
    void setNodeTimeoutDuration(const unsigned long nodeTimeoutDurationValue);
    unsigned long getNodeTimeoutDuration();
#ifdef ethernetclientwithremoteIP_h
    void receiveNodesOnly(const boolean receiveNodesOnlyValue = true);
#endif
    void sendNodesOnly(const boolean sendNodesOnlyValue = true);


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

    void removeNode(const byte nodeNumber);

    IPAddress getIP(const byte nodeNumber);
    void sendKeepalive(const unsigned int port);
    unsigned long getSendKeepaliveMargin();
    void setSendKeepaliveMargin(const unsigned long sendKeepaliveMarginInput);
    void setSendKeepaliveResendDelay(const unsigned long sendKeepaliveResendDelayInput);
    unsigned long getSendKeepaliveResendDelay();
    boolean setEventKeepalive(const char eventKeepaliveInput[]);
    boolean setEventKeepalive(const int16_t eventKeepaliveInput);
    boolean setEventKeepalive(const uint16_t eventKeepaliveInput);
    boolean setEventKeepalive(const int32_t eventKeepaliveInput);
    boolean setEventKeepalive(const uint32_t eventKeepaliveInput);
    boolean setEventKeepalive(const __FlashStringHelper* eventKeepaliveFSH);

    boolean setEventAck(const char eventAckInput[]);
    boolean setEventAck(const int16_t eventAckInput);
    boolean setEventAck(const uint16_t eventAckInput);
    boolean setEventAck(const int32_t eventAckInput);
    boolean setEventAck(const uint32_t eventAckInput);
    boolean setEventAck(const __FlashStringHelper* eventAckFSH);
    void setQueueDoubleDecimalPlaces(byte decimalPlaces);


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
  private:
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //private constants - these are constants that need to be accessed in this file so they can't be defined in EtherEventQueue.cpp
    static const byte uint16_tLengthMax = 5;  //5 digits
    static const byte int16_tLengthMax = 1 + uint16_tLengthMax;  //sign + 5 digits
    static const byte uint32_tLengthMax = 10;  //10 digits
    static const byte int32_tLengthMax = 1 + uint32_tLengthMax;  //sign + 10 digits
    static const byte IPAddressLengthMax = 3 + 1 + 3 + 1 + 3 + 1 + 3;  //4 x octet + 3 x dot
    static const byte doubleIntegerLengthMax = 40;  //sign + 39 digits max (-1000000000000000000000000000000000000000 gives me "floating constant exceeds range of 'double'" warning)

    static const byte nodeStateTimedOut = 0;
    static const byte nodeStateActive = 1;
    static const byte nodeStateUnknown = 2;

    static const byte eventIDlength = 2;

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


    byte eventIDfind();
    void remove(const byte queueStep);


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


    boolean nodeIsSet(const byte nodeNumber);
    void FSHtoa(const __FlashStringHelper* FlashString, char charBuffer[], byte maxLength);
};
extern EtherEventQueueClass EtherEventQueue;  //declare the class so it doesn't have to be done in the sketch
#endif

