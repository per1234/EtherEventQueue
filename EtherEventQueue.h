#ifndef EtherEventQueue_h
  #define EtherEventQueue_h
  #include "Arduino.h"
  #include <SPI.h>  //for the ethernet library
  #include <Ethernet.h>  //
  #include <MD5.h>  //for etherEvent authentication
  #include <Entropy.h>  //true random numbers for the EtherEvent authentication process
  #include "EtherEvent.h"  //include the EtherEvent library so its functions can be accessed

  const byte queueSizeMax=30;  //max number of messages to queue up before discarding the oldest one
  const byte eventLengthMax=3;  //max number of characters of the event
  const byte payloadLengthMax=50;  //max number of characters of the payload
  const byte messageIDlength=2;  //number of characters of the message ID that is appended to the start of the raw payload, the message ID must be exactly this length
  const byte nodeCount=11;  //
  
  class EtherEventQueueClass{
    public:
      void begin(byte deviceIDvalue, unsigned int portValue);
      byte availableEvent(EthernetServer &ethernetServer);
      byte availablePayload();
      void readEvent(char eventBuffer[]);
      void readPayload(char payloadBuffer[]);
      void flushReceiver();
      byte queue(const IPAddress targetIP, unsigned int port, const char event[], const char payload[], boolean resendFlag);
      void queueHandler(EthernetClient &ethernetClient);
      IPAddress checkTimeout();
      IPAddress checkTimein();
      boolean checkTimeoutSelf();
    private:
      byte messageIDfind();
      void remove(byte queueStep);
      int getNodeID(const IPAddress IPvalue);
      
      byte deviceID;
      unsigned int port;
      char receivedEvent[eventLengthMax+1];  //buffers to hold the available event
      char receivedPayload[payloadLengthMax+1];
      
      IPAddress IPqueue[queueSizeMax];  //queue buffers
      unsigned int portQueue[queueSizeMax];
      char eventQueue[queueSizeMax][eventLengthMax+1];
      byte messageIDqueue[queueSizeMax];  //unique identifier for the message
      char payloadQueue[queueSizeMax][payloadLengthMax+1];
      boolean resendFlagQueue[queueSizeMax];
      
      byte queueNewCount;  //number of new messages in the queue
      byte queueSize;  //how many messages are currently in the send queue
      byte queueStep;  //which message in the queue is it on
      unsigned long queueSendTimestamp;  //used for delayed resends of messages in the queue that failed the first time
      
      boolean nodeState[nodeCount];  //0=not timed out 1=timed out - state at the last check
      unsigned long nodeTimestamp[nodeCount];  //last received event time
  };
  extern EtherEventQueueClass EtherEventQueue;  //declare the class so it doesn't have to be done in the sketch
#endif
