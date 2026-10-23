// ===== BioCARE EMG + L293D Motor Control =====
// Protocol with Python:
//  - Arduino SENDS:  "<millis>,<emg_raw>\n"
//  - Arduino RECEIVES: "MOTOR:<0-100>\n"  → sets DC motor PWM via L293D

// -------- Pins (UPDATED) --------
const int EMG_PIN   = A0;   // MyoWare output
const int ENA_PIN   = 11;   // L293D EN1 (must be PWM-capable)
const int IN1_PIN   = 9;    // L293D IN1
const int IN2_PIN   = 10;   // L293D IN2

// -------- Motor limits --------
const int MOTOR_PWM_MAX = 200;  // out of 255 (≈78%)

// Sampling rate
const unsigned long SAMPLE_PERIOD_US = 1000; // 1 kHz

// Serial command buffer
const byte CMD_BUF_LEN = 32;
char cmdBuf[CMD_BUF_LEN];
byte cmdPos = 0;

// Motor state
int currentMotorPercent = 0;
unsigned long lastSampleMicros = 0;


void setup() {
  pinMode(EMG_PIN, INPUT);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // Default motor direction + initially off
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, 0);

  Serial.begin(115200);
}


// Map 0–100% → PWM and apply it
void setMotorPercent(int pct) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;

  currentMotorPercent = pct;
  int pwm = map(pct, 0, 100, 0, MOTOR_PWM_MAX);

  if (pwm == 0) {
    analogWrite(ENA_PIN, 0);
  } else {
    // One-direction operation (safe default)
    digitalWrite(IN1_PIN, HIGH);
    digitalWrite(IN2_PIN, LOW);
    analogWrite(ENA_PIN, pwm);
  }
}


// Reads "MOTOR:<value>" from Serial
void handleSerial() {
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      cmdBuf[cmdPos] = '\0';
      if (cmdPos > 0) {
        parseCommand(cmdBuf);
      }
      cmdPos = 0;
    } else {
      if (cmdPos < CMD_BUF_LEN - 1) {
        cmdBuf[cmdPos++] = c;
      } else {
        cmdPos = 0; // overflow
      }
    }
  }
}

void parseCommand(const char *cmd) {
  if (strncmp(cmd, "MOTOR:", 6) == 0) {
    int value = atoi(cmd + 6);
    setMotorPercent(value);
  }
}


// Main loop: EMG sampling + serial commands
void loop() {

  // --- 1 kHz EMG sampling ---
  unsigned long now_us = micros();
  if (now_us - lastSampleMicros >= SAMPLE_PERIOD_US) {
    lastSampleMicros += SAMPLE_PERIOD_US;

    int raw = analogRead(EMG_PIN);
    unsigned long ms = millis();

    // Python expects EXACT format: "<ms>,<raw>"
    Serial.print(ms);
    Serial.print(",");
    Serial.println(raw);
  }
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, 100);
  // Handle motor commands from Python
  handleSerial();
}
