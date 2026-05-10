// DESIGNMASK - direct PWM test for one AGFRC linear actuator.
//
// Board: Adafruit ESP32 Feather V2 / HUZZAH32 ESP32 Feather V2
//
// Wiring for this test:
//   Actuator signal -> Feather D33 / GPIO33
//   Actuator V+     -> servo power +
//   Actuator GND    -> servo power GND and Feather GND
//
// This sketch does not move on boot. It only outputs PWM after a command.

const uint8_t ACTUATOR_PIN = 33;      // D33 on the Feather
const uint32_t PWM_HZ = 50;           // Standard servo rate
const uint8_t PWM_BITS = 16;
const uint32_t PWM_TOP = (1UL << PWM_BITS) - 1UL;
const uint32_t PWM_PERIOD_US = 20000; // 50 Hz = 20 ms

const int MIN_US = 1000;
const int MAX_US = 2000;
const int CENTER_US = 1500;

bool pwmActive = false;
int currentPulseUs = CENTER_US;

char inputLine[48];
uint8_t inputLength = 0;

uint32_t pulseToDuty(int pulseUs) {
  pulseUs = constrain(pulseUs, MIN_US, MAX_US);
  return ((uint32_t)pulseUs * PWM_TOP) / PWM_PERIOD_US;
}

int percentToPulse(int percent) {
  percent = constrain(percent, 0, 100);
  return map(percent, 0, 100, MIN_US, MAX_US);
}

int pulseToPercent(int pulseUs) {
  pulseUs = constrain(pulseUs, MIN_US, MAX_US);
  return map(pulseUs, MIN_US, MAX_US, 0, 100);
}

void startPwmIfNeeded() {
  if (pwmActive) return;
  if (!ledcAttach(ACTUATOR_PIN, PWM_HZ, PWM_BITS)) {
    Serial.println("ERR ledcAttach failed");
    return;
  }
  pwmActive = true;
}

void writePulse(int pulseUs) {
  currentPulseUs = constrain(pulseUs, MIN_US, MAX_US);
  startPwmIfNeeded();
  if (!pwmActive) return;
  ledcWrite(ACTUATOR_PIN, pulseToDuty(currentPulseUs));
}

void stopPwm() {
  if (!pwmActive) return;
  ledcWrite(ACTUATOR_PIN, 0);
  ledcDetach(ACTUATOR_PIN);
  pinMode(ACTUATOR_PIN, INPUT);
  pwmActive = false;
}

void printHelp() {
  Serial.println();
  Serial.println("DESIGNmask direct PWM actuator test");
  Serial.println("Wire signal to D33 / GPIO33 only.");
  Serial.println("Commands:");
  Serial.println("  HELP       show commands");
  Serial.println("  STATUS     show state");
  Serial.println("  S p        set percent 0-100");
  Serial.println("  US pulse   set pulse 1000-2000 microseconds");
  Serial.println("  C          center at 1500us");
  Serial.println("  TEST       slow jog: 1300us, 1700us, 1500us");
  Serial.println("  D          stop PWM");
  Serial.println();
}

void printStatus() {
  Serial.print("ACTUATOR pin=D33/GPIO33 pos=");
  Serial.print(pulseToPercent(currentPulseUs));
  Serial.print("% pulse=");
  Serial.print(currentPulseUs);
  Serial.print("us active=");
  Serial.println(pwmActive ? "yes" : "no");
}

void normalizeLine(char* line) {
  for (uint8_t i = 0; line[i] != '\0'; i++) {
    if (line[i] == ':' || line[i] == ',' || line[i] == '=') {
      line[i] = ' ';
    } else {
      line[i] = toupper(line[i]);
    }
  }
}

void runTestJog() {
  Serial.println("TEST 1300us");
  writePulse(1300);
  delay(1000);
  Serial.println("TEST 1700us");
  writePulse(1700);
  delay(1000);
  Serial.println("TEST 1500us");
  writePulse(1500);
  Serial.println("OK TEST");
}

void handleLine() {
  inputLine[inputLength] = '\0';
  normalizeLine(inputLine);

  char* command = strtok(inputLine, " \t");
  if (!command) return;

  if (!strcmp(command, "HELP") || !strcmp(command, "?")) {
    printHelp();
    return;
  }

  if (!strcmp(command, "STATUS")) {
    printStatus();
    return;
  }

  if (!strcmp(command, "S")) {
    char* first = strtok(nullptr, " \t");
    char* second = strtok(nullptr, " \t");
    if (!first) {
      Serial.println("ERR usage: S p");
      return;
    }
    int percent = second ? atoi(second) : atoi(first);
    writePulse(percentToPulse(percent));
    Serial.print("OK S ");
    Serial.print(pulseToPercent(currentPulseUs));
    Serial.print("% ");
    Serial.print(currentPulseUs);
    Serial.println("us");
    return;
  }

  if (!strcmp(command, "US")) {
    char* first = strtok(nullptr, " \t");
    char* second = strtok(nullptr, " \t");
    if (!first) {
      Serial.println("ERR usage: US pulse");
      return;
    }
    int pulseUs = second ? atoi(second) : atoi(first);
    writePulse(pulseUs);
    Serial.print("OK US ");
    Serial.print(currentPulseUs);
    Serial.println("us");
    return;
  }

  if (!strcmp(command, "C")) {
    writePulse(CENTER_US);
    Serial.println("OK centered");
    return;
  }

  if (!strcmp(command, "TEST")) {
    runTestJog();
    return;
  }

  if (!strcmp(command, "D")) {
    stopPwm();
    Serial.println("OK detached");
    return;
  }

  Serial.println("ERR unknown command, type HELP");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(ACTUATOR_PIN, INPUT);
  printHelp();
  printStatus();
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputLength > 0) {
        handleLine();
        inputLength = 0;
      }
    } else if (inputLength < sizeof(inputLine) - 1) {
      inputLine[inputLength++] = c;
    } else {
      inputLength = 0;
      Serial.println("ERR line too long");
    }
  }
}
