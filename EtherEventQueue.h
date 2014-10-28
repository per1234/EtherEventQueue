#ifndef EtherEventQueue_h
#define EtherEventQueue_h
#include <Arduino.h>
#include <SPI.h>  //for the ethernet library
#include <Ethernet.h>  //
#include "MD5.h"  //for etherEvent authentication
#include "EtherEvent.h"  //include the EtherEvent library so its functions can be accessed

const byte EtherEventQueue_queueSizeMax = 30; //max number of messages to queue up before discarding the oldest one
const byte EtherEventQueue_eventLengthMax = 15; //max number of characters of the event
const byte EtherEventQueue_payloadLengthMax = 60; //max number of characters of the payload
const byte EtherEventQueue_eventIDlength = 2; //number of characters of the message ID that is appended to the start of the raw payload, the message ID must be exactly this length
const byte EtherEventQueue_nodeCount = 11; //total number of nodes

class EtherEventQueueClass {
  public:
    void begin(byte nodeDeviceValue, unsigned int portValue);
    byte availableEvent(EthernetServer &ethernetServer);
    byte availablePayload();
    void readEvent(char eventBuffer[]);
    void readPayload(char payloadBuffer[]);
    IPAddress senderIP();
    void flushReceiver();
    byte queue(const IPAddress targetIP, unsigned int port, const char event[], const char payload[], boolean resendFlag);
    byte queue(byte targetNode, unsigned int port, const char event[], const char payload[], boolean resendFlag);
    void queueHandler(EthernetClient &ethernetClient);
    void flushQueue();
    int checkTimeout();
    int checkTimein();
    boolean checkState(byte node);
    int getNode(const IPAddress IPvalue);
  private:
    byte eventIDfind();
    void remove(byte queueStep);

    byte EtherEventQueue_nodeDevice;
    unsigned int port;
    char receivedEvent[EtherEventQueue_eventLengthMax + 1]; //buffers to hold the available event
    char receivedPayload[EtherEventQueue_payloadLengthMax + 1];

    unsigned int portQueue[EtherEventQueue_queueSizeMax];
    char eventQueue[EtherEventQueue_queueSizeMax][EtherEventQueue_eventLengthMax + 1];
    byte eventIDqueue[EtherEventQueue_queueSizeMax];  //unique identifier for the message
    char payloadQueue[EtherEventQueue_queueSizeMax][EtherEventQueue_payloadLengthMax + 1];
    boolean resendFlagQueue[EtherEventQueue_queueSizeMax];

    byte queueNewCount;  //number of new messages in the queue
    byte queueSize;  //how many messages are currently in the send queue
    byte queueStep;  //which message in the queue is it on
    unsigned long queueSendTimestamp;  //used for delayed resends of messages in the queue that failed the first time

    byte nodeState[EtherEventQueue_nodeCount];  //1=not timed out 0=timed out - state at the last check
    unsigned long nodeTimestamp[EtherEventQueue_nodeCount];  //last received event time
};
extern EtherEventQueueClass EtherEventQueue;  //declare the class so it doesn't have to be done in the sketch
#endif
