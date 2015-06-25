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
    boolean begin(const byte queueSizeMaxInput, const byte sendEventLengthMaxInput, const byte sendPayloadLengthMaxInput, const byte receivedEventLengthMaxInput, const byte receivedPayloadLengthMaxInput);
    boolean begin(const byte nodeDeviceInput, const byte nodeCountInput);
    boolean begin(const byte nodeDeviceInput, byte nodeCountInput, byte queueSizeMaxInput, const byte sendEventLengthMaxInput, const byte sendPayloadLengthMaxInput, const byte receivedEventLengthMaxInput, const byte receivedPayloadLengthMaxInput);

    byte availableEvent(EthernetServer &ethernetServer, long cookieInput = false);
    byte availablePayload();
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


    boolean queueHandler(EthernetClient &ethernetClient);
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

    //private global variables
    byte nodeDevice;
    unsigned int defaultPort;
    byte receivedEventLengthMax;
    byte receivedEventIDvalue;
    char* receivedEvent;  //buffer to hold the received event
    byte receivedPayloadLengthMax;
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
    byte sendPayloadLengthMax;
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

