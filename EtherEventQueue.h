//EtherEventQueue outgoing event queue for the EtherEvent authenticated network communication arduino library: http://github.com/per1234/EtherEvent
#ifndef EtherEventQueue_h
#define EtherEventQueue_h
#include <SPI.h>  //for the ethernet library
#include "Ethernet.h"
//#include "Flash.h"  //uncomment this line if you have the Flash library installed

#define DEBUG false  //(false == serial debug output off,  true == serial debug output on)The serial debug output will increase memory usage and communication latency so only enable when in use.
#define ETHEREVENTQUEUE_SERIAL if(DEBUG)Serial  //I have to use a different name for Serial in this file otherwise the debug statement control also affects any other file that includes this file.


class EtherEventQueueClass {
  public:
    //public constants
    const byte queueTypeOnce = 0;
    const byte queueTypeRepeat = 1;
    const byte queueTypeConfirm = 2;

    const byte queueSuccessOverflow = 2;

    //the library handles these special events differently
    const char eventKeepalive[4] = {'1', '0', '0', 0};
    const char eventAck[4] = {'1', '0', '1', 0};


    EtherEventQueueClass();

    boolean begin();
    boolean begin(byte queueSizeMaxInput, byte sendEventLengthMaxInput, byte sendPayloadLengthMaxInput, byte receivedEventLengthMaxInput, byte receivedPayloadLengthMaxInput);
    boolean begin(byte nodeDeviceInput, byte nodeCountInput);
    boolean begin(byte nodeDeviceInput, byte nodeCountInput, byte queueSizeMaxInput, byte sendEventLengthMaxInput, byte sendPayloadLengthMaxInput, byte receivedEventLengthMaxInput, byte receivedPayloadLengthMaxInput);

    byte availableEvent(EthernetServer &ethernetServer);
    byte availablePayload();
    void readEvent(char eventBuffer[]);
    void readPayload(char payloadBuffer[]);
#ifdef ethernetclientwithremoteIP_h
    IPAddress senderIP();
#endif
    byte receivedEventID();
    void flushReceiver();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //queue
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    byte queue(const byte targetIP[], unsigned int port, const char event[], const char payload[], byte resendFlag);  //main queue prototype
    byte queue(byte targetNode, unsigned int targetPort, const char event[], const char payload[], byte resendFlag);
    byte queue(const IPAddress &targetIPAddress, unsigned int port,  const char event[], const char payload[], byte resendFlag);

    //convert event
    template <typename targetType, typename payloadType>
    byte queue(const targetType &target, unsigned int port, int16_t event, const payloadType payload, byte resendFlag) {
      char eventChar[int16_tLengthMax + 1];
      itoa(event, eventChar, 10);
      return queue(target, port, eventChar, payload, resendFlag);
    }
    template <typename targetType, typename payloadType>
    byte queue(const targetType &target, unsigned int port, uint16_t event, const payloadType payload, byte resendFlag) {
      char eventChar[uint16_tLengthMax + 1];
      sprintf_P(eventChar, PSTR("%u"), event);
      return queue(target, port, eventChar, payload, resendFlag);
    }
    template <typename targetType, typename payloadType>
    byte queue(const targetType &target, unsigned int port, int32_t event, const payloadType payload, byte resendFlag) {
      char eventChar[int32_tLengthMax + 1];
      ltoa(event, eventChar, 10);
      return queue(target, port, eventChar, payload, resendFlag);
    }
    template <typename targetType, typename payloadType>
    byte queue(const targetType &target, unsigned int port, uint32_t event, const payloadType payload, byte resendFlag) {
      char eventChar[uint32_tLengthMax + 1];
      ultoa(event, eventChar, 10);
      return queue(target, port, eventChar, payload, resendFlag);
    }
    template <typename targetType, typename payloadType>
    byte queue(const targetType &target, unsigned int port, const __FlashStringHelper* event, byte eventLength, const payloadType payload, byte resendFlag) {
      char eventChar[eventLength + 1];
      memcpy_P(eventChar, event, eventLength + 1);  //+1 for the null terminator
      return queue(target, port, eventChar, payload, resendFlag);
    }

    //convert payload
    template <typename targetType>
    byte queue(const targetType &target, unsigned int port, const char event[], int16_t payload, byte resendFlag) {
      char payloadChar[int16_tLengthMax + 1];
      itoa(payload, payloadChar, 10);
      return queue(target, port, event, payloadChar, resendFlag);
    }
    template <typename targetType>
    byte queue(const targetType &target, unsigned int port, const char event[], uint16_t payload, byte resendFlag) {
      char payloadChar[uint16_tLengthMax + 1];
      sprintf_P(payloadChar, PSTR("%u"), payload);
      return queue(target, port, event, payloadChar, resendFlag);
    }
    template <typename targetType>
    byte queue(const targetType &target, unsigned int port, const char event[], int32_t payload, byte resendFlag) {
      char payloadChar[int32_tLengthMax + 1];
      ltoa(payload, payloadChar, 10);
      return queue(target, port, event, payloadChar, resendFlag);
    }
    template <typename targetType>
    byte queue(const targetType &target, unsigned int port, const char event[], uint32_t payload, byte resendFlag) {
      char payloadChar[uint32_tLengthMax + 1];
      uloa(payload, payloadChar, 10);
      return queue(target, port, event, payloadChar, resendFlag);
    }
    template <typename targetType, typename eventType>
    byte queue(const targetType &target, unsigned int port, eventType event, const __FlashStringHelper* payload, byte payloadLength, byte resendFlag) {
      char payloadChar[payloadLength + 1];
      memcpy_P(payloadChar, payload, payloadLength + 1);  //+1 for the null terminator
      return queue(target, port, event, payloadChar, resendFlag);
    }


    //convert F() event and payload
    template <typename targetType>
    byte queue(const targetType &target, unsigned int port, const __FlashStringHelper* event, byte eventLength, const __FlashStringHelper* payload, byte payloadLength, byte resendFlag) {
      char eventChar[eventLength + 1];
      memcpy_P(eventChar, event, eventLength + 1);  //+1 for the null terminator

      char payloadChar[payloadLength + 1];
      memcpy_P(payloadChar, payload, payloadLength + 1);  //+1 for the null terminator

      return queue(target, port, eventChar, payloadChar, resendFlag);
    }

    //Flash templates
#ifdef __FLASH_H__
    template <typename targetType, typename payloadType>
    byte queue(const targetType &target, unsigned int port, const _FLASH_STRING event, const payloadType payload, byte resendFlag) {
      byte stringLength = event.length();
      char eventChar[stringLength + 1];
      event.copy(eventChar, stringLength + 1, 0);  //+1 for null terminator
      return queue(target, port, eventChar, payload, resendFlag);
    }
    template <typename targetType>
    byte queue(const targetType &target, unsigned int port, const char event[], const _FLASH_STRING payload, byte resendFlag) {
      byte stringLength = payload.length();
      char payloadChar[stringLength + 1];
      payload.copy(payloadChar, stringLength + 1, 0);  //+1 for null terminator
      return queue(target, port, event, payloadChar, resendFlag);
    }
#endif



    void queueHandler(EthernetClient &ethernetClient);
    void flushQueue();
    int8_t checkTimeout();
    int8_t checkTimein();
    int8_t checkState(byte node);


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //getNode - template that can accept IPAddress or byte array type parameters - this function must be defined in the .h instead of the .cpp because it is a template
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename IPtype>
    int8_t getNode(const IPtype &IPvalue) {
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
    void setResendDelay(unsigned long resendDelayValue);
    unsigned long getResendDelay();
    void setNodeTimeoutDuration(unsigned long nodeTimeoutDurationValue);
    unsigned long getNodeTimeoutDuration();
#ifdef ethernetclientwithremoteIP_h
    void receiveNodesOnly(boolean receiveNodesOnlyValue = true);
#endif
    void sendNodesOnly(boolean sendNodesOnlyValue = true);


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //setNode
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename IPtype>
    boolean setNode(byte nodeNumber, const IPtype &nodeIPaddress) {
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

    void removeNode(byte nodeNumber);

    IPAddress getIP(byte nodeNumber);
    void sendKeepalive(unsigned int port);
    unsigned long getSendKeepaliveMargin();
    void setSendKeepaliveMargin(unsigned long sendKeepaliveMarginInput);
    void setSendKeepaliveResendDelay(unsigned long sendKeepaliveResendDelayInput);
    unsigned long getSendKeepaliveResendDelay();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
  private:
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////


    //private constants - these are constants that need to be accessed in this file so they can't be defined in EtherEventQueue.cpp
    const byte uint16_tLengthMax = 5;  //5 digits
    const byte int16_tLengthMax = 1 + uint16_tLengthMax;  //sign + 5 digits
    const byte uint32_tLengthMax = 10;  //10 digits
    const byte int32_tLengthMax = 1 + uint32_tLengthMax;  //sign + 10 digits
    const byte floatLengthMax = 1 + 7 + 1;  //sign + 7 digits + decimal
    const byte nodeStateTimedOut = 0;
    const byte nodeStateActive = 1;
    const byte nodeStateUnknown = 2;

    //private global variables
    byte nodeDevice;
    unsigned int defaultPort;
    byte receivedEventLengthMax;
    byte receivedEventIDvalue;
    char* receivedEvent;  //buffer to hold the received event
    byte receivedPayloadLengthMax;
    char* receivedPayload;  //buffer to hold the received payload
#ifdef ethernetclientwithremoteIP_h
    IPAddress receivedIP;
#endif
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
    byte* resendFlagQueue;

    byte queueNewCount;  //number of new messages in the queue
    byte localEventQueueCount;
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


    byte eventIDfind();
    void remove(byte queueStep);


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //IPcopy - copies IPAddress or 4 byte array to 4 byte array - IPdestination will contain the converted IPsource after this function is called
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename IPdestinationType, typename IPsourceType>
    void IPcopy(IPdestinationType &IPdestination, const IPsourceType &IPsource) {
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.IPcopy: IPsource="));
      ETHEREVENTQUEUE_SERIAL.println(IPAddress(IPsource));
      for (byte counter = 0; counter < 4; counter++) {
        IPdestination[counter] = IPsource[counter];
      }
      ETHEREVENTQUEUE_SERIAL.print(F("EtherEventQueue.IPcopy: IPdestination="));
      ETHEREVENTQUEUE_SERIAL.println(IPAddress(IPdestination));
    }


    boolean nodeIsSet(byte nodeNumber);
};
extern EtherEventQueueClass EtherEventQueue;  //declare the class so it doesn't have to be done in the sketch
#endif
