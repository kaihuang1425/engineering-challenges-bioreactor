// Pin Definitions (TTGO T-Display ESP32)
const int hallPin = 27;
const int motorPin = 26;

// --- 核心滤波与标定参数 ---
const unsigned long debounceUs = 200; // 过滤 <200us 的电刷/PWM毛刺（实测噪声在113us附近）
float pulsesPerRevolution = 72.0;      // 尚未标定真实PPR前保持 1.0，直接以 Freq(Hz) 为准

volatile unsigned long pulseCount = 0;
volatile unsigned long lastPulseUs = 0;

unsigned long lastTime = 0;
float smoothHz = 0.0;

void IRAM_ATTR countPulse() {
  unsigned long now = micros();
  if (now - lastPulseUs >= debounceUs) {
    pulseCount++;
    lastPulseUs = now;
  }
}

int currentPwm = 0;

void setMotorSpeed(int targetSpeed) {
  targetSpeed = constrain(targetSpeed, 0, 255);

  // 从静止启动到低速区间（1~80）时，先给 120ms 高脉冲打破静摩擦
  if (currentPwm == 0 && targetSpeed > 0 && targetSpeed < 80) {
    analogWrite(motorPin, 140);
    delay(120);
  }

  analogWrite(motorPin, targetSpeed);
  currentPwm = targetSpeed;
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50); // 兼容任意串口行结束符，不卡顿
  pinMode(hallPin, INPUT_PULLUP);
  pinMode(motorPin, OUTPUT);
  analogWrite(motorPin, 0);

  attachInterrupt(digitalPinToInterrupt(hallPin), countPulse, RISING);
  Serial.println("TTGO Filtered Stirrer Controller Ready. Enter PWM (0-255):");
}

void loop() {
  // 1. 稳定读取串口 PWM 指令（兼容有无换行符，避免误清零）
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0 && isDigit(input.charAt(0))) {
      int targetSpeed = constrain(input.toInt(), 0, 255);
      setMotorSpeed(targetSpeed);
      Serial.print("Target PWM set to: ");
      Serial.println(targetSpeed);
    }
  }

  // 2. 每 500 ms 计算一次滤波后的频率与转速
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= 500) {
    unsigned long elapsedMs = currentTime - lastTime;

    noInterrupts();
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    interrupts();

    float rawHz = (pulses * 1000.0) / elapsedMs;

    if (pulses == 0) {
      smoothHz = 0.0;
    } else if (smoothHz == 0.0) {
      smoothHz = rawHz;
    } else {
      smoothHz = 0.7 * smoothHz + 0.3 * rawHz;
    }

    float rpm = (smoothHz * 60.0) / pulsesPerRevolution;

    Serial.print("Freq: ");
    Serial.print(smoothHz, 1);
    Serial.print(" Hz | RPM: ");
    Serial.println(rpm, 1);

    lastTime = currentTime;
  }
}