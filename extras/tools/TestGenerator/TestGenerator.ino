// This sketch is used to automatically generate the queue() overload tests and print them to the Serial Monitor

const byte maximumStringLength = 25;
const char targetTypes[][maximumStringLength] = {
  {"IPAddress"},
  {"byte_array"},
  {"const_byte_array"},
  {"byte"}
};

const char eventPayloadTypes[][maximumStringLength] = {
  {"char_array"},
  {"const_char_array"},
  {"byte"},
  {"int8_t"},
  {"int"},
  {"const_int"},
  {"unsigned_int"},
  {"long"},
  {"unsigned_long"},
  {"float"},
  {"double"},
  {"IPAddress"},
  {"String"},
  {"string_literal"},
  {"__FlashStringHelper"}
};

const byte eventPayloadTypesCount = sizeof(eventPayloadTypes) / sizeof(eventPayloadTypes[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("void runTests(){");
  Serial.println("// queue() overload tests:");

  for (byte targetTypeCounter = 0; targetTypeCounter < sizeof(targetTypes) / sizeof(targetTypes[0]); ++targetTypeCounter) {
    for (byte eventTypeCounter = 0; eventTypeCounter < eventPayloadTypesCount; ++eventTypeCounter) {
      // queue(target, port, eventType, event)
      printThroughEvent(targetTypeCounter, eventTypeCounter);
      Serial.println(");");

      for (byte payloadTypeCounter = 0; payloadTypeCounter < eventPayloadTypesCount; ++payloadTypeCounter) {
        // queue(ethernetClient, target, port, eventType, event, payload)
        printThroughPayload(targetTypeCounter, eventTypeCounter, payloadTypeCounter);
        Serial.println(");");
      }
    }
  }
  Serial.println("}");
}


void printThroughEvent(const byte targetTypeCounter, const byte eventTypeCounter) {
  Serial.print("EtherEventQueue.queue(target_");
  Serial.print(targetTypes[targetTypeCounter]);
  Serial.print(", sendPort, eventType, event_");
  Serial.print(eventPayloadTypes[eventTypeCounter]);
}


void printThroughPayload(const byte targetTypeCounter, const byte eventTypeCounter, const byte payloadTypeCounter) {
  printThroughEvent(targetTypeCounter, eventTypeCounter);
  Serial.print(", payload_");
  Serial.print(eventPayloadTypes[payloadTypeCounter]);
}


void loop() {}
