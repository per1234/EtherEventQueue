#ifndef EtherEventQueueNodes_h
#define EtherEventQueueNodes_h

namespace etherEventQueue {
const IPAddress nodeIP[] = {  //IP addresses on the network, this can be used to filter IPs or to monitor the IPs for timeout/timein and optimize the queue based on this information
  IPAddress(192, 168, 69, 100),
  IPAddress(192, 168, 69, 101),
  IPAddress(192, 168, 69, 102),
  IPAddress(192, 168, 69, 103),
  IPAddress(192, 168, 69, 104),
  IPAddress(192, 168, 69, 105),
  IPAddress(192, 168, 69, 106),
  IPAddress(192, 168, 69, 107),
  IPAddress(192, 168, 69, 108),
  IPAddress(192, 168, 69, 109),
  IPAddress(192, 168, 69, 110)
};
}

#endif
