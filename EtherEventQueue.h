//EtherEventQueue outgoing event queue for the EtherEvent authenticated network communication arduino library: http://github.com/per1234/EtherEvent
#ifndef EtherEventQueue_h
#define EtherEventQueue_h
#include <SPI.h>  //for the ethernet library
#include "Ethernet.h"
//#include "Flash.h"  //https://github.com/rkhamilton/Flash - uncomment this line if you have the Flash library installed
#include "EtherEventQueueNodes.h"

class EtherEventQueueClass {
  public:
    EtherEventQueueClass();
    boolean begin(const char password[], byte nodeDeviceInput, unsigned int portInput, byte queueSizeMaxInput = 5, byte sendEventLengthMaxInput = 15, byte sendPayloadLengthMaxInput = 80, byte receiveEventLengthMaxInput = 15, byte receivePayloadLengthMaxInput = 80);
    byte availableEvent(EthernetServer &ethernetServer);
    byte availablePayload();
    void readEvent(char eventBuffer[]);
    void readPayload(char payloadBuffer[]);
#ifdef ethernetclientwithremoteIP_h
    IPAddress senderIP();
#endif
    void flushReceiver();
    byte queue(byte targetNode, unsigned int port, const char event[], const char payload[], byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const char event[], const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const char event[], int16_t payloadInt, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const char event[], uint16_t payloadUint, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const char event[], int32_t payloadLong, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const char event[], uint32_t payloadUlong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], int16_t payloadInt, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], uint16_t payloadUint, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], int32_t payloadLong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], uint32_t payloadUlong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], const char payload[], byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, const char payload[], byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, int16_t payloadInt, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, uint16_t payloadUint, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, int32_t payloadLong, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, uint32_t payloadUlong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, int16_t payloadInt, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, uint16_t payloadUint, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, int32_t payloadLong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, uint32_t payloadUlong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int16_t event, const char payload[], byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, const char payload[], byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, int16_t payloadInt, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, uint16_t payloadUint, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, int32_t payloadLong, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, uint32_t payloadUlong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, int16_t payloadInt, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, uint16_t payloadUint, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, int32_t payloadLong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, uint32_t payloadUlong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, const char payload[], byte resendFlag);
#ifdef __FLASH_H__
    byte queue(byte targetNode, unsigned int port, const char event[], _FLASH_STRING payloadFlashString, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], _FLASH_STRING payloadFlashString, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, _FLASH_STRING payloadFlashString, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, _FLASH_STRING payloadFlashString, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, _FLASH_STRING payloadFlashString, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, _FLASH_STRING payloadFlashString, byte resendFlag);
#endif
    void queueHandler(EthernetClient &ethernetClient);
    void flushQueue();
    int8_t checkTimeout();
    int8_t checkTimein();
    boolean checkState(byte node);


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //getNode - template that can accept IPAddress or byte array type parameters - this function must be defined in the .h instead of the .cpp because it is a template
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename IPtype>
    int8_t getNode(const IPtype IPvalue) {
      //Serial.println(F("EtherEventQueue.getNode: start"));
      for (byte node = 0; node < sizeof(etherEventQueue::nodeIP) / sizeof(etherEventQueue::nodeIP[0]); node++) {  //step through all the nodes
        byte octet;
        for (octet = 0; octet < 4; octet++) {
          if (etherEventQueue::nodeIP[node][octet] != IPvalue[octet]) {  //mismatch
            octet = 0;
            break;  //check the next node for a match
          }
        }
        if (octet == 4) {  //node matched
          //Serial.print(F("EtherEventQueue.getNode: node found="));
          //Serial.println(node);
          return node;
        }
      }
      Serial.println(F("EtherEventQueue.getNode: node not found"));
      return -1;  //no match
    }


    boolean checkQueueOverflow();
    void setResendDelay(unsigned int resendDelayValue);
    unsigned int getResendDelay();
    void setNodeTimeoutDuration(unsigned int nodeTimeoutDurationValue);
    unsigned int getNodeTimeoutDuration();
    void receiveNodesOnly(boolean receiveNodesOnlyValue = true);
    void sendNodesOnly(boolean sendNodesOnlyValue = true);

    //public constants
    const byte queueTypeOnce = 0;
    const byte queueTypeRepeat = 1;
    const byte queueTypeConfirm = 2;

    //the library handles these special events differently
    const char eventKeepalive[4] = {'1', '0', '0', 0};
    const char eventAck[4] = {'1','0', '1', 0};

  private:
    byte eventIDfind();
    void remove(byte queueStep);
    void IPcopy(byte IPdestination[], const IPAddress IPsource);

    byte nodeDevice;
    unsigned int port;
    byte receivedEventLengthMax;
    char* receivedEvent;  //buffer to hold the received event
    byte receivedPayloadLengthMax;
    char* receivedPayload;  //buffer to hold the received payload

    byte queueSizeMax;
    byte** IPqueue;  //queue buffers
    unsigned int* portQueue;
    byte sendEventLengthMax;
    char** eventQueue;
    byte* eventIDqueue;  //unique identifier for the message
    byte sendPayloadLengthMax;
    char** payloadQueue;
    byte* resendFlagQueue;

    byte queueNewCount;  //number of new messages in the queue
    byte queueSize;  //how many messages are currently in the send queue
    byte queueStep;  //which message in the queue is it on
    unsigned long queueSendTimestamp;  //used for delayed resends of messages in the queue that failed the first time

    byte nodeState[sizeof(etherEventQueue::nodeIP) / sizeof(etherEventQueue::nodeIP[0])]; //1=not timed out 0=timed out - state at the last check
    unsigned long nodeTimestamp[sizeof(etherEventQueue::nodeIP) / sizeof(etherEventQueue::nodeIP[0])]; //last received event time
    byte queueOverflowFlag;
    byte localEventQueueCount;
#ifdef ethernetclientwithremoteIP_h
    IPAddress receivedIP;
#endif
    byte receivedEventLength;
    unsigned long nodeTimeoutDuration;
    unsigned int resendDelay;
    boolean receiveNodesOnlyState;  //restrict event receiving to nodes only
    boolean sendNodesOnlyState;  //restrict event sending to nodes only
};
extern EtherEventQueueClass EtherEventQueue;  //declare the class so it doesn't have to be done in the sketch
#endif
