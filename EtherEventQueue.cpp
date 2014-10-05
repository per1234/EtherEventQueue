// EtherEventQueue outgoing event queue for the EtherEvent authenticated network communication arduino library: http://github.com/per1234/EtherEvent
#include "Arduino.h"
#include "EtherEventQueue.h"  //http://github.com/per1234/EtherEventQueue
#include <SPI.h>  //for the ethernet library
#include <Ethernet.h> 
#include <MD5.h>  //for etherEvent authentication
#include <Entropy.h>  //true random numbers for the EtherEvent authentication process
#include "EtherEvent.h"  //http://github.com/per1234/EtherEvent


//this will disable the compiler warning for F()
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

//configuration:
#define DEBUG 0 // (0==serial debug output off, 1==serial debug output on)The serial debug output will greatly slow down communication time and so different timeout values are enabled.
#define Serial if(DEBUG)Serial

//#define NODE_ONLY_RECEIVE  //uncomment to restrict event receiving to nodes only
//#define NODE_ONLY_SEND  //uncomment to restrict event sending to nodes only

const IPAddress node[]={  //IP addresses on the network, this can be used to filter IPs or to monitor the IPs for timeout/timein and optimize the queue based on this information
  IPAddress(192,168,69,100),
  IPAddress(192,168,69,101),
  IPAddress(192,168,69,102),
  IPAddress(192,168,69,103),
  IPAddress(192,168,69,104),
  IPAddress(192,168,69,105),
  IPAddress(192,168,69,106),
  IPAddress(192,168,69,107),
  IPAddress(192,168,69,108),
  IPAddress(192,168,69,109),
  IPAddress(192,168,69,110)
};

const unsigned long nodeTimeout=270000;  //(ms)the node is timed out if it has been longer than this duration since the last event was received from it
const unsigned long nodeTimeoutSelf=200000;  //(ms)the device is timed out if it has been longer than this duration since any event was received

const char eventPing[]="100";  //the library handles these special events differently
const char eventAck[]="101";
const unsigned int resendDelay=30000;  //(ms)delay between resends of messages


void EtherEventQueueClass::begin(byte deviceIDvalue, unsigned int portValue){
  deviceID=deviceIDvalue;
  port=portValue;
  nodeTimestamp[0]=0;
  //const byte nodeCount=sizeof(node)/sizeof(IPAddress);  //size the node related buffers according to this equation here
}


byte EtherEventQueueClass::availableEvent(EthernetServer &ethernetServer){
  if(byte length=strlen(receivedEvent)){    //there is already a previously received event buffered
    return length+1;   //number of bytes including the null terminator remaining to be read of the event
  }
  if(availablePayload()>0){  //don't get another event until the last is fully read or flushed
    return 0;
  }

  if(byte availableBytesEvent = EtherEvent.availableEvent(ethernetServer)){  //there is a new event
    Serial.println(F("---------------------------"));
    Serial.print(F("EtherEventQueue.availableEvent: EtherEvent.availableEvent()="));
    Serial.println(availableBytesEvent);
    IPAddress receivedIP=EtherEvent.senderIP();
    Serial.print(F("ethernetRead: remoteIP="));
    Serial.println(receivedIP);

    nodeTimestamp[deviceID]=millis();  //set the general ping timestamp(using the deviceID because that part of the array is never used otherwise)
    //update timed out status of the event sender
    byte receivedID=getNodeID(receivedIP);  //get the ID of the receivedIP
    if(receivedID>=0){  //-1 indicates no node match
      nodeTimestamp[receivedID]=millis();  //set the individual timestamp, any communication is considered to be a ping
    }
    #ifdef RECEIVE_ONLY_FROM_NODE
      else{
        Serial.println(F("EtherEventQueue.availableEvent: unauthorized IP"));
        EtherEvent.flushReceiver();
        return 0;
      }
    #endif

    EtherEvent.readEvent(receivedEvent);  //put the event in the buffer
    Serial.print(F("EtherEventQueue.availableEvent: ethernetReadEvent="));
    Serial.println(receivedEvent);
    
    if(strcmp(receivedEvent, eventPing)==0){  //ping received
      EtherEvent.flushReceiver();
      return 0;  //receive ping silently
    }
    
    byte payloadLength=EtherEvent.availablePayload();
    Serial.print(F("EtherEventQueue.availableEvent: EtherEvent.availablePayload()="));
    Serial.println(payloadLength);
    char receivedPayloadRaw[payloadLength];  //(TODO: get rid of this, just use the regular payload buffer)initialize raw payload buffer
    EtherEvent.readPayload(receivedPayloadRaw);  //read the payload to the buffer
    Serial.print(F("EtherEventQueue.availableEvent: rawPayload="));
    Serial.println(receivedPayloadRaw);
    
    //break the payload down into parts and convert the messageID, code, and target address to byte, the true payload stays as a char array 
    char receivedMessageID[messageIDlength+1];
    for(byte count=0;count<messageIDlength;count++){
      receivedMessageID[count]=receivedPayloadRaw[count];
    }
    receivedMessageID[messageIDlength]=0;  //add the null terminator because there is not one after the id in the string
    Serial.print(F("EtherEventQueue.availableEvent: ethernetReadMessageID="));
    Serial.println(receivedMessageID);
         
    if(payloadLength>messageIDlength+1){  //there is a true payload
      for(byte count=0; count < payloadLength - messageIDlength; count++){
        receivedPayload[count]=receivedPayloadRaw[count+messageIDlength];  //(TODO: just use receivedPayload for the buffer instead of having the raw buffer)    
      }
      Serial.print(F("EtherEventQueue.availableEvent: receivedPayload="));
      Serial.println(receivedPayload);
    }

    if(strcmp(receivedEvent, eventAck)==0){  //ack handler
      Serial.println(F("EtherEventQueue.availableEvent: ack received"));
      byte receivedPayloadInt=atoi(receivedPayload);  //convert to a byte        
      for(byte count=0;count<queueSize;count++){  //step through the currently occupied section of the messageIDqueue[]
        if(receivedPayloadInt==messageIDqueue[count]){  //this is the message the ack is for
          Serial.println(F("EtherEventQueue.availableEvent: ack messageID match"));
          remove(count);  //remove the message from the queue
        }
      }
      EtherEvent.flushReceiver();
      return 0;  //receive ack silently
    }

    Serial.println(F("EtherEventQueue.availableEvent: send ack"));
    queue(receivedIP, port, eventAck, receivedMessageID, 0);  //send the ack - the messageID of the received message is the payload

    return availableBytesEvent;  //there is a new event, return the number of bytes
  }
  return 0;
}


byte EtherEventQueueClass::availablePayload(){  //returns the number of chars in the payload including the null terminator if there is one
  if(byte length=strlen(receivedPayload)){  //strlen(receivedPayload)>0
    return length+1;  //length of the payload + null terminator
  }
  return 0;
}


void EtherEventQueueClass::readEvent(char eventBuffer[]){
  Serial.println(F("EtherEventQueue.readEvent: start"));
  strcpy(eventBuffer,receivedEvent);
  eventBuffer[strlen(receivedEvent)]=0;  //null terminator - is this needed?
  receivedEvent[0]=0;  //reset the event buffer
}


void EtherEventQueueClass::readPayload(char payloadBuffer[]){
  Serial.println(F("EtherEventQueue.readPayload: start"));
  strcpy(payloadBuffer,receivedPayload);
  payloadBuffer[strlen(receivedPayload)]=0;  //null terminator - is this needed?
  receivedPayload[0]=0;  //reset the payload buffer
}


void EtherEventQueueClass::flushReceiver(){  //dump the last message received so another one can be received
  Serial.println(F("EtherEventQueue.flushReceiver: start"));
  receivedEvent[0]=0;  //reset the event buffer
  receivedPayload[0]=0;  //reset the payload buffer
}


byte EtherEventQueueClass::queue(const IPAddress targetIP, unsigned int targetPort, const char event[], const char payload[], boolean resendFlag){  //add the relayed outgoing message to the send queue. Returns: 0==fail, 1==success, 2==success w/ queue overflow
  Serial.println(F("---------------------------"));
  Serial.println(F("EtherEventQueue.queue: start"));
  int targetID=getNodeID(targetIP);
  
  #ifdef SEND_TO_NODE_ONLY
    if(targetID<0){  //not an authorized IP
      Serial.println(F("EtherEventQueue.queue: not an authorized IP"));
      return 0;
    }
  #endif
  byte success=0;
  if(targetID==deviceID){
    Serial.println(F("EtherEventQueue.queue: can't self send"));
  }
  else if(targetID>=0 && millis() - nodeTimestamp[targetID] > nodeTimeout){  //check for timedout node
    Serial.println(F("EtherEventQueue.queue: timed out authorized IP"));
  }
  else{
    success=1;  //indicate event successfully queued in return
    byte messageID=messageIDfind();  //get a message ID
    queueSize++;
    if(queueSize>queueSizeMax){  //queue overflowed
      queueSize=queueSizeMax;  //had to bump the first item in the queue because there's no room for it
      Serial.println(F("EtherEventQueue.queue: Queue Overflowed"));  //I don't really want to send another message into the queue because that will recursive loop
      success=2;  //indicate overflow in the return
      for(byte count=0;count<queueSize-1;count++){  //shift all messages up the queue and add new item to queue. This is kind of similar to the ack received part where I removed the message from the queue so maybe it could be a function
        IPqueue[count]=IPqueue[count+1];
        portQueue[count]=portQueue[count+1];
        strcpy(eventQueue[count],eventQueue[count+1]);    
        messageIDqueue[count]=messageIDqueue[count+1];
        strcpy(payloadQueue[count],payloadQueue[count+1]);
        resendFlagQueue[count]=resendFlagQueue[count+1];        
      }
    }
    Serial.print(F("EtherEventQueue.queue: new size="));
    Serial.println(queueSize);
    
    //add the new message to the queue
    IPqueue[queueSize-1]=targetIP;  //put the new code in the array after the most recent entry
    portQueue[queueSize-1]=targetPort;
    strcpy(eventQueue[queueSize-1], event);  //set the messageID for the message in the queue
    messageIDqueue[queueSize-1]=messageID;
    strcpy(payloadQueue[queueSize-1],payload);  //put the new payload in the queue
    queueNewCount++;  //this is a new one so send immediately
    Serial.print(F("EtherEventQueue.queue: done, queueNewCount="));
    Serial.println(queueNewCount);
  }
  return success;  //don't put it in the queue
}


void EtherEventQueueClass::queueHandler(EthernetClient &ethernetClient){  //ethernetQueueHandler - sends out the messages in the queue-------------
  if(queueSize>0){  //there are messages in the queue
    if(queueNewCount>0 || millis() - queueSendTimestamp > resendDelay){  //it is time
      Serial.print(F("EtherEventQueue.queueHandler: queueSize="));
      Serial.println(queueSize);
      Serial.print(F("EtherEventQueue.queueHandler: queueNewCount="));
      Serial.println(queueNewCount);
      byte queueStepSend;  //this is used to store the step to send which may not be the current step because of sending the new messages first
      for(;;){  
        if(queueNewCount==0){  //time to send the next one in the queue
          queueSendTimestamp=millis();  //reset the timestamp to delay the next queue resend
          queueStep++;
          if(queueStep>=queueSize){
            queueStep=0;
          }
          queueStepSend=queueStep;
        }
        else if(queueNewCount>0){  //send the new items in the queue immediately
           queueStepSend=queueSize-queueNewCount;  //send the oldest new one first
           queueNewCount--;
        }
        Serial.print(F("EtherEventQueue.queueHandler: queueStepSend="));
        Serial.println(queueStepSend);
        int targetID=getNodeID(IPqueue[queueStepSend]);  //get the node ID of the target IP
        Serial.print(F("EtherEventQueue.queueHandler: targetID="));
        Serial.println(targetID);
        if(targetID<0){  //-1 indicates no node match
          Serial.println(F("EtherEventQueue.queueHandler: non-node targetIP"));
          break;  //non-nodes never timeout
        }

        if(millis() - nodeTimestamp[targetID] < nodeTimeout || strcmp(eventQueue[queueStepSend], eventPing)==0){  //non-timed out ID or ping
          break;  //continue with the message send
        }
        Serial.print(F("EtherEventQueue.queueHandler: ethernetQueueTargetID timed out for queue#="));
        Serial.println(queueStepSend);
        remove(queueStepSend);  //dump messages for dead IDs from the queue
        if(queueSize==0){  //no events left to send
          return;
        }
      }

      //set up the raw payload
      char messageID[3];
      itoa(messageIDqueue[queueStepSend],messageID,10);  //put the message ID on the start of the payload
      char payload[strlen(payloadQueue[queueStepSend])+messageIDlength+1];
      strcpy(payload,messageID);
      strcat(payload,payloadQueue[queueStepSend]);  //add the true payload to the payload string

      Serial.print(F("EtherEventQueue.queueHandler: targetIP="));
      Serial.println(IPqueue[queueStepSend]);
      Serial.print(F("EtherEventQueue.queueHandler: event="));
      Serial.println(eventQueue[queueStepSend]);
      Serial.print(F("EtherEventQueue.queueHandler: payload="));
      Serial.println(payload);
      
      if(EtherEvent.send(ethernetClient, IPqueue[queueStepSend], portQueue[queueStepSend], eventQueue[queueStepSend], payload)>0){
        Serial.println(F("EtherEventQueue.queueHandler: send successful"));
        if(resendFlagQueue[queueStepSend]==0){
          Serial.println(F("EtherEventQueue.queueHandler: resendFlag==0, event removed from queue"));
          remove(queueStepSend);  //remove the message from the queue immediately
        }
        return;
      }
      Serial.println(F("EtherEventQueue.queueHandler: send failed"));
    }
  }
}


IPAddress EtherEventQueueClass::checkTimeout(){  //checks all the authorized IPs until it finds a newly timed out IP and returns it or none and returns 0.0.0.0
  for(byte ID=0;ID<sizeof(node)/sizeof(IPAddress);ID++){
    if(ID!=deviceID && nodeState[ID]==0 && millis() - nodeTimestamp[ID] < nodeTimeout){  //ID is not this device, not already timed out, and is timed out
      Serial.print(F("EtherEventQueue.checkTimeout: timed out ID="));
      Serial.println(ID);
      nodeState[ID]=1;  //1 indicates the ID is timed out
      return node[ID];
    }
  }
  return IPAddress(0,0,0,0);
}


IPAddress EtherEventQueueClass::checkTimein(){  //checks all the authorized IPs until it finds a newly timed out IP and returns it or none and returns 0.0.0.0
  for(byte ID=0;ID<sizeof(node)/sizeof(IPAddress);ID++){
    if(ID!=deviceID && nodeState[ID]==1 && millis() - nodeTimestamp[ID] < nodeTimeout){  //ID is not this device, not already timed out, and is timed out
      Serial.print(F("EtherEventQueue.checkTimein: timed out ID="));
      Serial.println(ID);
      nodeState[ID]=0;  //1 indicates the ID is timed out
      return node[ID];
    }
  }
  return IPAddress(0,0,0,0);
}


boolean EtherEventQueueClass::checkTimeoutSelf(){  //checks all the authorized IPs until it finds a newly timed out IP and returns it or none and returns 0.0.0.0
  if(nodeState[deviceID]==0 && millis() - nodeTimestamp[deviceID] < nodeTimeoutSelf){  //ID is not this device, not already timed out, and is timed out
    Serial.print(F("EtherEventQueue.checkTimeoutSelf: timed out"));
    nodeState[deviceID]=1;  //1 indicates the ID is timed out
    return 1;
  }
  Serial.println(F("EtherEventQueue.checkTimeoutSelf: not timed out"));
  return 0;
}


//----------------private functions------------------------------
byte EtherEventQueueClass::messageIDfind(){  //----find a free messageID---- this is similar to the messageID check in the ack received code so maybe make a function  
  Serial.println(F("EtherEventQueue.messageIDfind: start"));
  if(queueSize==0){  //the queue is empty
    Serial.println(F("EtherEventQueue.messageIDfind: messageID=10"));
    return 10;  //default value if there are no other messages
  }
  if(queueSize>0){
    for(byte messageID=10;messageID<queueSizeMax+10;messageID++){  //step through all possible messageIDs. They start at 10 so they will always be 2 digit
      byte messageIDduplicate=0;
      for(byte count=0;count<queueSize;count++){  //step through the currently occupied section of the messageIDqueue[]
        if(messageID==messageIDqueue[count]){  //the messageID is already being used
          messageIDduplicate=1;
        }
      }
      if(messageIDduplicate==0){  //the messageID was unique
        Serial.print(F("EtherEventQueue.messageIDfind: messageID="));
        Serial.println(messageID);
        return messageID;  //skip the rest of the for loop
      }
    }
  }
  return 0;  //this should never happen but it causes a compiler warning without
} 


void EtherEventQueueClass::remove(byte queueStep){
  if(queueSize>0){
    queueSize--;
  }
  Serial.print(F("EtherEventQueue.remove: new queue size="));
  Serial.println(queueSize);
  if(queueSize>0){  //if this is the only message in the queue being removed then it doesn't need to adjust the queue
    for(byte count=queueStep;count<queueSize;count++){  //move all the messages above the one to remove up in the queue
      IPqueue[count]=IPqueue[count+1];  //set the target for the message in the queue
      portQueue[count]=portQueue[count+1];      
      strcpy(eventQueue[count],eventQueue[count+1]);    
      messageIDqueue[count]=messageIDqueue[count+1];
      strcpy(payloadQueue[count],payloadQueue[count+1]);
      resendFlagQueue[count]=resendFlagQueue[count+1];
    }
  }
}


int EtherEventQueueClass::getNodeID(const IPAddress IPvalue){
  Serial.println(F("EtherEventQueue.getNodeID: start"));
  for(byte ID=0; ID<sizeof(node)/sizeof(IPAddress); ID++){  //step through all the nodes
    byte octet;
    for(octet=0; octet < 4; octet++){
      if(node[ID][octet]!=IPvalue[octet]){  //mismatch
        octet=0;
        break;
      }      
    }
    if(octet==4){  //match
      Serial.println(F("EtherEventQueue.getNodeID: ID found"));
      return ID;
    }
  }
  Serial.println(F("EtherEventQueue.getNodeID: ID not found"));
  return -1;  //no match
}


EtherEventQueueClass EtherEventQueue; //This sets up a single global instance of the library so the class doesn't need to be declared in the user sketch and multiple instances are not necessary in this case.
