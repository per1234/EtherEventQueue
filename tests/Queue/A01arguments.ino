const IPAddress target_IPAddress = IPAddress(192, 168, 69, 100);  //The IP address to send the test events to.
byte target_byte_array[] = {192, 168, 69, 100};  //The IP address to send the test events to.
byte target_const_byte_array[] = {192, 168, 69, 100};  //The IP address to send the test events to.
const byte target_byte = 1;  //The node to send the test events to.

char event_char_array[] = "asdf";
const char event_const_char_array[] = "asdf";
char event_char = 'a';
byte event_byte = 3;
int8_t event_int8_t = -5;
int event_int = 1000;
const int event_const_int = 1000;
unsigned int event_unsigned_int = -400;
long event_long = 80000;
unsigned long event_unsigned_long = -90999;
float event_float = 1.23;
double event_double = 1.23;
IPAddress event_IPAddress = IPAddress(192, 168, 69, 104);
String event_String = "asdf";
#define event_string_literal "asdf"
#define event___FlashStringHelper F("asdf")

char payload_char_array[] = "asdf";
const char payload_const_char_array[] = "asdf";
char payload_char = 'a';
byte payload_byte = 3;
int8_t payload_int8_t = -5;
int payload_int = 1000;
const int payload_const_int = 1000;
unsigned int payload_unsigned_int = -400;
long payload_long = 80000;
unsigned long payload_unsigned_long = -90999;
float payload_float = 1.23;
double payload_double = 1.23;
IPAddress payload_IPAddress = IPAddress(192, 168, 69, 104);
String payload_String = "asdf";
#define payload_string_literal "asdf"
#define payload___FlashStringHelper F("asdf")

const unsigned int sendPort = 1024;  //The port to send the test events to.
const byte eventType = EtherEventQueue.eventTypeOnce;
