//Returns the number of characters required to display a number in base 10. This is useful for converting numbers to text. It works for signed or unsigned byte, int and long. http://github.com/per1234/Numlen
#include "Arduino.h"
#include "Numlen.h"

byte NumlenClass::numlen(int8_t numlenInput) {
  byte numlenNegative = 0;
  if (numlenInput < 0) {
    numlenNegative = 1;
  }
  numlenInput = abs(numlenInput);
  byte numlenOutput = 1;
  if (numlenInput > 9) {
    numlenOutput = 2;
  }
  if (numlenInput > 99) {
    numlenOutput = 3;
  }
  return numlenOutput + numlenNegative;
}

byte NumlenClass::numlen(byte numlenInput) {
  byte numlenOutput = 1;
  if (numlenInput > 9) {
    numlenOutput = 2;
  }
  if (numlenInput > 99) {
    numlenOutput = 3;
  }
  return numlenOutput;
}

byte NumlenClass::numlen(int numlenInput) {
  byte numlenNegative = 0;
  if (numlenInput < 0) {
    numlenNegative = 1;
  }
  numlenInput = abs(numlenInput);
  byte numlenOutput = 1;
  if (numlenInput > 9) {
    numlenOutput = 2;
  }
  if (numlenInput > 99) {
    numlenOutput = 3;
  }
  if (numlenInput > 999) {
    numlenOutput = 4;
  }
  if (numlenInput > 9999) {
    numlenOutput = 5;
  }
  return numlenOutput + numlenNegative;
}

byte NumlenClass::numlen(unsigned int numlenInput) {
  byte numlenOutput = 1;
  if (numlenInput > 9) {
    numlenOutput = 2;
  }
  if (numlenInput > 99) {
    numlenOutput = 3;
  }
  if (numlenInput > 999) {
    numlenOutput = 4;
  }
  if (numlenInput > 9999) {
    numlenOutput = 5;
  }
  return numlenOutput;
}

byte NumlenClass::numlen(long numlenInput) {
  byte numlenNegative = 0;
  if (numlenInput < 0) {
    numlenNegative = 1;
  }
  numlenInput = abs(numlenInput);
  byte numlenOutput = 1;
  if (numlenInput > 9) {
    numlenOutput = 2;
  }
  if (numlenInput > 99) {
    numlenOutput = 3;
  }
  if (numlenInput > 999) {
    numlenOutput = 4;
  }
  if (numlenInput > 9999) {
    numlenOutput = 5;
  }
  if (numlenInput > 99999) {
    numlenOutput = 6;
  }
  if (numlenInput > 999999) {
    numlenOutput = 7;
  }
  if (numlenInput > 9999999) {
    numlenOutput = 8;
  }
  if (numlenInput > 99999999) {
    numlenOutput = 9;
  }
  if (numlenInput > 999999999) {
    numlenOutput = 10;
  }
  return numlenOutput + numlenNegative;
}

byte NumlenClass::numlen(unsigned long numlenInput) {
  byte numlenOutput = 1;
  if (numlenInput > 9) {
    numlenOutput = 2;
  }
  if (numlenInput > 99) {
    numlenOutput = 3;
  }
  if (numlenInput > 999) {
    numlenOutput = 4;
  }
  if (numlenInput > 9999) {
    numlenOutput = 5;
  }
  if (numlenInput > 99999) {
    numlenOutput = 6;
  }
  if (numlenInput > 999999) {
    numlenOutput = 7;
  }
  if (numlenInput > 9999999) {
    numlenOutput = 8;
  }
  if (numlenInput > 99999999) {
    numlenOutput = 9;
  }
  if (numlenInput > 999999999) {
    numlenOutput = 10;
  }
  return numlenOutput;
}

NumlenClass Numlen;

