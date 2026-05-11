// DESIGNMASK - direct PWM control for five AGFRC linear actuators.
//
// Board: Adafruit ESP32 Feather V2 / HUZZAH32 ESP32 Feather V2
//
// Wiring:
//   Motor 1 signal -> GPIO33
//   Motor 2 signal -> GPIO32
//   Motor 3 signal -> GPIO27
//   Motor 4 signal -> GPIO12
//   Motor 5 signal -> GPIO13
//   Actuator V+    -> servo power +
//   Actuator GND   -> servo power GND and Feather GND
//
// This sketch does not move on boot. It only outputs PWM after a command.

const uint8_t MOTOR_COUNT = 5;
const uint8_t MOTOR_PINS[MOTOR_COUNT] = {33, 32, 27, 12, 13};

const uint32_t PWM_HZ = 50;           // Standard servo rate
const uint8_t PWM_BITS = 16;
const uint32_t PWM_TOP = (1UL << PWM_BITS) - 1UL;
const uint32_t PWM_PERIOD_US = 20000; // 50 Hz = 20 ms

const int MIN_US = 1000;
const int MAX_US = 2000;
const int CENTER_US = 1500;

bool pwmActive[MOTOR_COUNT] = {false, false, false, false, false};
int currentPulseUs[MOTOR_COUNT] = {CENTER_US, CENTER_US, CENTER_US, CENTER_US, CENTER_US};

char inputLine[64];
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

bool isValidMotor(uint8_t motorIndex) {
  return motorIndex < MOTOR_COUNT;
}

bool startPwmIfNeeded(uint8_t motorIndex) {
  if (!isValidMotor(motorIndex)) return false;
  if (pwmActive[motorIndex]) return true;
  if (!ledcAttach(MOTOR_PINS[motorIndex], PWM_HZ, PWM_BITS)) {
    Serial.print("ERR ledcAttach failed motor=");
    Serial.println(motorIndex + 1);
    return false;
  }
  pwmActive[motorIndex] = true;
  return true;
}

void writePulse(uint8_t motorIndex, int pulseUs) {
  if (!isValidMotor(motorIndex)) return;
  currentPulseUs[motorIndex] = constrain(pulseUs, MIN_US, MAX_US);
  if (!startPwmIfNeeded(motorIndex)) return;
  ledcWrite(MOTOR_PINS[motorIndex], pulseToDuty(currentPulseUs[motorIndex]));
}

void stopPwm(uint8_t motorIndex) {
  if (!isValidMotor(motorIndex) || !pwmActive[motorIndex]) return;
  ledcWrite(MOTOR_PINS[motorIndex], 0);
  ledcDetach(MOTOR_PINS[motorIndex]);
  pinMode(MOTOR_PINS[motorIndex], INPUT);
  pwmActive[motorIndex] = false;
}

void stopAllPwm() {
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    stopPwm(i);
  }
}

void printHelp() {
  Serial.println();
  Serial.println("DESIGNmask five-motor PWM actuator control");
  Serial.println("Commands:");
  Serial.println("  HELP          show commands");
  Serial.println("  STATUS        show all motor states");
  Serial.println("  S p           set motor 1 percent 0-100");
  Serial.println("  S m p         set motor m percent 0-100");
  Serial.println("  US pulse      set motor 1 pulse 1000-2000 microseconds");
  Serial.println("  US m pulse    set motor m pulse 1000-2000 microseconds");
  Serial.println("  C             center all motors at 1500us");
  Serial.println("  C m           center motor m at 1500us");
  Serial.println("  D             stop PWM on all motors");
  Serial.println("  D m           stop PWM on motor m");
  Serial.println();
}

void printMotorStatus(uint8_t motorIndex) {
  Serial.print("ACTUATOR ");
  Serial.print(motorIndex + 1);
  Serial.print(" pin=GPIO");
  Serial.print(MOTOR_PINS[motorIndex]);
  Serial.print(" pos=");
  Serial.print(pulseToPercent(currentPulseUs[motorIndex]));
  Serial.print("% pulse=");
  Serial.print(currentPulseUs[motorIndex]);
  Serial.print("us active=");
  Serial.println(pwmActive[motorIndex] ? "yes" : "no");
}

void printStatus() {
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    printMotorStatus(i);
  }
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

bool parseMotorToken(const char* token, uint8_t* motorIndex) {
  if (!token) return false;
  int motor = atoi(token);
  if (motor < 1 || motor > MOTOR_COUNT) return false;
  *motorIndex = motor - 1;
  return true;
}

bool parseIndexedValue(char* first, char* second, uint8_t* motorIndex, int* value) {
  if (!first) return false;
  if (second) {
    if (!parseMotorToken(first, motorIndex)) return false;
    *value = atoi(second);
    return true;
  }
  *motorIndex = 0;
  *value = atoi(first);
  return true;
}

void centerMotor(uint8_t motorIndex) {
  writePulse(motorIndex, CENTER_US);
}

void centerAllMotors() {
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    centerMotor(i);
  }
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
    uint8_t motorIndex = 0;
    int percent = 0;
    if (!parseIndexedValue(first, second, &motorIndex, &percent)) {
      Serial.println("ERR usage: S [motor] percent");
      return;
    }
    writePulse(motorIndex, percentToPulse(percent));
    Serial.print("OK S ");
    Serial.print(motorIndex + 1);
    Serial.print(" ");
    Serial.print(pulseToPercent(currentPulseUs[motorIndex]));
    Serial.print("% ");
    Serial.print(currentPulseUs[motorIndex]);
    Serial.println("us");
    return;
  }

  if (!strcmp(command, "US")) {
    char* first = strtok(nullptr, " \t");
    char* second = strtok(nullptr, " \t");
    uint8_t motorIndex = 0;
    int pulseUs = 0;
    if (!parseIndexedValue(first, second, &motorIndex, &pulseUs)) {
      Serial.println("ERR usage: US [motor] pulse");
      return;
    }
    writePulse(motorIndex, pulseUs);
    Serial.print("OK US ");
    Serial.print(motorIndex + 1);
    Serial.print(" ");
    Serial.print(currentPulseUs[motorIndex]);
    Serial.println("us");
    return;
  }

  if (!strcmp(command, "C")) {
    char* first = strtok(nullptr, " \t");
    if (first) {
      uint8_t motorIndex = 0;
      if (!parseMotorToken(first, &motorIndex)) {
        Serial.println("ERR usage: C [motor]");
        return;
      }
      centerMotor(motorIndex);
      Serial.print("OK centered ");
      Serial.println(motorIndex + 1);
      return;
    }
    centerAllMotors();
    Serial.println("OK centered all");
    return;
  }

  if (!strcmp(command, "D")) {
    char* first = strtok(nullptr, " \t");
    if (first) {
      uint8_t motorIndex = 0;
      if (!parseMotorToken(first, &motorIndex)) {
        Serial.println("ERR usage: D [motor]");
        return;
      }
      stopPwm(motorIndex);
      Serial.print("OK detached ");
      Serial.println(motorIndex + 1);
      return;
    }
    stopAllPwm();
    Serial.println("OK detached all");
    return;
  }

  Serial.println("ERR unknown command, type HELP");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    pinMode(MOTOR_PINS[i], INPUT);
  }
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
