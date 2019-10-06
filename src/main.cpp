#include <Arduino.h>

const int     OPT_PIN       = D2;
const size_t  READ_INTERVAL = 10;

const size_t  CUT           = 0x00;
const size_t  THROUGH       = 0x01;

boolean isMachineWorking();
boolean isCut ();
boolean hasFlipped();

long    lastReadTime = 0;
size_t  lastStateRead;
size_t  steps = 0;
size_t  revolutions = 0;
int read;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while(!Serial){}
  Serial.println("Device started");
  pinMode(OPT_PIN, INPUT);
  lastStateRead = isCut() ? CUT : THROUGH;
}


void loop () {
  if (isMachineWorking()) {
    if (lastReadTime + READ_INTERVAL < millis()) {
      if (hasFlipped()) {
        ++steps;
        Serial.printf("Steps counted %d", steps / 2);
        Serial.println();
        if (steps == STEPS_IN_REV << 1) {
          steps = 0;
          ++revolutions;
          Serial.printf("Revolutions counted %d", revolutions);
          Serial.println();
        }
        lastStateRead = lastStateRead == CUT ? THROUGH : CUT;
      }
      lastReadTime = millis();
    }
  }
}

boolean isMachineWorking() {
  return true;
}

boolean hasFlipped () {
  if (isCut()) {
    return lastStateRead == THROUGH;
  } else {
    return lastStateRead == CUT;
  }
}

boolean isCut () {
  return (read = analogRead(OPT_PIN)) < CUT_THRESHOLD;
}

// ICACHE_RAM_ATTR void optRead () {
//   long read = analogRead(OPT_PIN);
//   if (read < lastAnalogRead - 100 || read > lastAnalogRead + 100) {
//     lastAnalogRead = read;
//     ++steps;
//   }
// }ZZZZZZZZ