#include "MotorDriver.h"

// ESP32 PWM channels & frequencies for TB6612
// In Arduino ESP32 core 3.x, ledcAttach() connects pin directly
static const uint32_t PWM_FREQ = 20000; // 20 kHz ultrasonic PWM (no motor whine)
static const uint8_t PWM_RES   = 8;     // 8-bit resolution (0 - 255)

void initMotors() {
  pinMode(PIN_MOTOR_AIN1, OUTPUT);
  pinMode(PIN_MOTOR_AIN2, OUTPUT);
  pinMode(PIN_MOTOR_BIN1, OUTPUT);
  pinMode(PIN_MOTOR_BIN2, OUTPUT);

  // Setup PWM on ESP32-S3
  ledcAttach(PIN_MOTOR_PWMA, PWM_FREQ, PWM_RES);
  ledcAttach(PIN_MOTOR_PWMB, PWM_FREQ, PWM_RES);

  stopMotors();
}

void setMotorSpeeds(int leftSpeed, int rightSpeed) {
  // Constrain inputs to valid -255 to 255 range
  leftSpeed  = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // --- Motor A (Left Motor) ---
  if (leftSpeed > 0) {
    digitalWrite(PIN_MOTOR_AIN1, HIGH);
    digitalWrite(PIN_MOTOR_AIN2, LOW);
    ledcWrite(PIN_MOTOR_PWMA, leftSpeed);
  } else if (leftSpeed < 0) {
    digitalWrite(PIN_MOTOR_AIN1, LOW);
    digitalWrite(PIN_MOTOR_AIN2, HIGH);
    ledcWrite(PIN_MOTOR_PWMA, -leftSpeed);
  } else {
    digitalWrite(PIN_MOTOR_AIN1, LOW);
    digitalWrite(PIN_MOTOR_AIN2, LOW);
    ledcWrite(PIN_MOTOR_PWMA, 0);
  }

  // --- Motor B (Right Motor) ---
  if (rightSpeed > 0) {
    digitalWrite(PIN_MOTOR_BIN1, HIGH);
    digitalWrite(PIN_MOTOR_BIN2, LOW);
    ledcWrite(PIN_MOTOR_PWMB, rightSpeed);
  } else if (rightSpeed < 0) {
    digitalWrite(PIN_MOTOR_BIN1, LOW);
    digitalWrite(PIN_MOTOR_BIN2, HIGH);
    ledcWrite(PIN_MOTOR_PWMB, -rightSpeed);
  } else {
    digitalWrite(PIN_MOTOR_BIN1, LOW);
    digitalWrite(PIN_MOTOR_BIN2, LOW);
    ledcWrite(PIN_MOTOR_PWMB, 0);
  }
}

void brakeMotors() {
  // TB6612 Active Brake: Both inputs HIGH, PWM = 255
  digitalWrite(PIN_MOTOR_AIN1, HIGH);
  digitalWrite(PIN_MOTOR_AIN2, HIGH);
  ledcWrite(PIN_MOTOR_PWMA, 255);

  digitalWrite(PIN_MOTOR_BIN1, HIGH);
  digitalWrite(PIN_MOTOR_BIN2, HIGH);
  ledcWrite(PIN_MOTOR_PWMB, 255);
}

void stopMotors() {
  // Coast stop: Both inputs LOW, PWM = 0
  digitalWrite(PIN_MOTOR_AIN1, LOW);
  digitalWrite(PIN_MOTOR_AIN2, LOW);
  ledcWrite(PIN_MOTOR_PWMA, 0);

  digitalWrite(PIN_MOTOR_BIN1, LOW);
  digitalWrite(PIN_MOTOR_BIN2, LOW);
  ledcWrite(PIN_MOTOR_PWMB, 0);
}

