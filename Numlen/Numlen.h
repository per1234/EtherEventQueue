#ifndef Numlen_h
#define Numlen_h
#include "Arduino.h"

class NumlenClass {
  public:
    byte numlen(int8_t numlenInput);
    byte numlen(byte numlenInput);
    byte numlen(int numlenInput);
    byte numlen(unsigned int numlenInput);
    byte numlen(long numlenInput);
    byte numlen(unsigned long numlenInput);
  private:
};
extern NumlenClass Numlen;
#endif
