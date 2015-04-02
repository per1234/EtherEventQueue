//EtherEventQueue outgoing event queue for the EtherEvent authenticated network communication arduino library: http://github.com/per1234/EtherEvent
#ifndef EtherEventQueue_h
#define EtherEventQueue_h
#include <SPI.h>  //for the ethernet library
#include "Ethernet.h"
#include "Flash.h"  //https://github.com/rkhamilton/Flash - uncomment this line if you have the Flash library installed
#include "EtherEventQueueNodes.h"

class EtherEventQueueClass {
  public:
    void begin(char password[], byte nodeDeviceInput, unsigned int portInput, byte queueSizeMaxInput, byte sendEventLengthMaxInput, byte sendPayloadLengthMaxInput, byte receiveEventLengthMaxInput, byte receivePayloadLengthMaxInput);
    byte availableEvent(EthernetServer &ethernetServer);
    byte availablePayload();
    void readEvent(char eventBuffer[]);
    void readPayload(char payloadBuffer[]);
    IPAddress senderIP();
    void flushReceiver();
    byte queue(byte targetNode, unsigned int port, const char event[], const char payload[], byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const char event[], const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const char event[], int payloadInt, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const char event[], unsigned int payloadUint, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const char event[], long payloadLong, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const char event[], unsigned long payloadUlong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], int payloadInt, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], unsigned int payloadUint, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], long payloadLong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], unsigned long payloadUlong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], const char payload[], byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, const char payload[], byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, int payloadInt, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, unsigned int payloadUint, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, long payloadLong, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, int event, unsigned long payloadUlong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, int payloadInt, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, unsigned int payloadUint, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, long payloadLong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, unsigned long payloadUlong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, int event, const char payload[], byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, const char payload[], byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, int payloadInt, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, unsigned int payloadUint, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, long payloadLong, byte resendFlag);
    byte queue(byte targetNode, unsigned int port, const __FlashStringHelper* event, byte eventLength, unsigned long payloadUlong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, const __FlashStringHelper* payload, byte payloadLength, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, int payloadInt, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, unsigned int payloadUint, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, long payloadLong, byte resendFlag);
    byte queue(const IPAddress targetIP, unsigned int port, const __FlashStringHelper* event, byte eventLength, unsigned long payloadUlong, byte resendFlag);
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
      for (byte node = 0; node < sizeof(EtherEventQueueNodes::nodeIP) / sizeof(EtherEventQueueNodes::nodeIP[0]); node++) {  //step through all the nodes
        byte octet;
        for (octet = 0; octet < 4; octet++) {
          if (EtherEventQueueNodes::nodeIP[node][octet] != IPvalue[octet]) {  //mismatch
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
      Serial.print(F("EtherEventQueue.getNode: node not found"));
      return -1;  //no match
    }


    boolean checkQueueOverflow();
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

    byte nodeState[sizeof(EtherEventQueueNodes::nodeIP)/sizeof(EtherEventQueueNodes::nodeIP[0])];  //1=not timed out 0=timed out - state at the last check
    unsigned long nodeTimestamp[sizeof(EtherEventQueueNodes::nodeIP)/sizeof(EtherEventQueueNodes::nodeIP[0])];  //last received event time
    byte queueOverflowFlag;
    byte localEventQueueCount;
    IPAddress receivedIP;
    byte receivedEventLength;
};
extern EtherEventQueueClass EtherEventQueue;  //declare the class so it doesn't have to be done in the sketch
#endif
