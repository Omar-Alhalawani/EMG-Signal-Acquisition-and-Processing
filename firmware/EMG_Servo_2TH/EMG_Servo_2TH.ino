const int EMG_PIN = A0;

void setup() {
  Serial.begin(115200);
  pinMode(EMG_PIN, INPUT);
}

void loop() {
  int raw = analogRead(EMG_PIN);
  unsigned long ms = millis();

  Serial.print(ms);
  Serial.print(',');
  Serial.println(raw);

  delayMicroseconds(500); // ~2kHz (Python will downsample)
}
