// DESIGNMASK — 5ch servo controller for Arduino Pro Micro
//
// Wiring (Pro Micro):
//   Servo 1 signal -> D3
//   Servo 2 signal -> D5
//   Servo 3 signal -> D6
//   Servo 4 signal -> D9
//   Servo 5 signal -> D10
//   All servo V+    -> Pro Micro VCC (5V)  *see note*
//   All servo GND   -> Pro Micro GND       (must share ground)
//
// Power note: 1.5g linear servos draw ~80–150mA each under light load.
// Five of them off USB is borderline; if servos jitter or the Pro Micro
// resets, run servo V+ from an external 5V supply and keep GND shared.
//
// Serial protocol (115200 baud):
//   "S<idx>:<val>\n"   idx = 0..4, val = 0..180
//   "C\n"              center all (val = 90)
//   "?\n"              prints current positions

#include <Servo.h>

const uint8_t PINS[5] = {3, 5, 6, 9, 10};
Servo servos[5];
uint8_t pos[5] = {90, 90, 90, 90, 90};

char buf[24];
uint8_t blen = 0;

void applyAll() {
  for (uint8_t i = 0; i < 5; i++) servos[i].write(pos[i]);
}

void reportPositions() {
  Serial.print("P:");
  for (uint8_t i = 0; i < 5; i++) {
    Serial.print(pos[i]);
    if (i < 4) Serial.print(',');
  }
  Serial.println();
}

void handleLine() {
  buf[blen] = '\0';
  if (blen == 0) return;

  if (buf[0] == 'S') {
    char* colon = strchr(buf, ':');
    if (!colon) return;
    *colon = '\0';
    int idx = atoi(buf + 1);
    int val = atoi(colon + 1);
    if (idx < 0 || idx > 4) return;
    if (val < 0) val = 0;
    if (val > 180) val = 180;
    pos[idx] = (uint8_t)val;
    servos[idx].write(pos[idx]);
    Serial.print("OK ");
    Serial.print(idx);
    Serial.print(' ');
    Serial.println(pos[idx]);
  } else if (buf[0] == 'C') {
    for (uint8_t i = 0; i < 5; i++) pos[i] = 90;
    applyAll();
    Serial.println("OK C");
  } else if (buf[0] == '?') {
    reportPositions();
  }
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < 5; i++) {
    servos[i].attach(PINS[i]);
    servos[i].write(pos[i]);
  }
  // Pro Micro has native USB; wait briefly for host
  delay(200);
  Serial.println("READY");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (blen > 0) {
        handleLine();
        blen = 0;
      }
    } else if (blen < sizeof(buf) - 1) {
      buf[blen++] = c;
    } else {
      blen = 0; // overflow guard
    }
  }
}
