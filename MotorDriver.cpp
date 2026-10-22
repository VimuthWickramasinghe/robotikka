#include "MotorDriver.h"

// TB6612FNG pins from Config.h (EasyEDA Rev 2.0 schematic):
//   PWMA=38, AIN1=34, AIN2=35  |  PWMB=9, BIN1=36, BIN2=37
// STBY is hardwired to +3.3 V (always enabled).

static const uint32_t MOTOR_PWM_FREQ_HZ = 20000;  // silent operation

void initMotors() {
    pinMode(PIN_MOTOR_AIN1, OUTPUT);
    pinMode(PIN_MOTOR_AIN2, OUTPUT);
    pinMode(PIN_MOTOR_BIN1, OUTPUT);
    pinMode(PIN_MOTOR_BIN2, OUTPUT);

    // Set 20 kHz PWM frequency on the speed pins before first analogWrite
    analogWriteFrequency(PIN_MOTOR_PWMA, MOTOR_PWM_FREQ_HZ);
    analogWriteFrequency(PIN_MOTOR_PWMB, MOTOR_PWM_FREQ_HZ);

    // Attach LEDC PWM channels (8-bit default: 0..255 duty)
    analogWrite(PIN_MOTOR_PWMA, 0);
    analogWrite(PIN_MOTOR_PWMB, 0);

    stopMotors();
}

void setMotorASpeed(int pwm) {
    if (pwm >= 0) {
        digitalWrite(PIN_MOTOR_AIN1, HIGH);
        digitalWrite(PIN_MOTOR_AIN2, LOW);
    } else {
        digitalWrite(PIN_MOTOR_AIN1, LOW);
        digitalWrite(PIN_MOTOR_AIN2, HIGH);
        pwm = -pwm;
    }
    analogWrite(PIN_MOTOR_PWMA, constrain(pwm, 0, 255));
}

void setMotorBSpeed(int pwm) {
    if (pwm >= 0) {
        digitalWrite(PIN_MOTOR_BIN1, HIGH);
        digitalWrite(PIN_MOTOR_BIN2, LOW);
    } else {
        digitalWrite(PIN_MOTOR_BIN1, LOW);
        digitalWrite(PIN_MOTOR_BIN2, HIGH);
        pwm = -pwm;
    }
    analogWrite(PIN_MOTOR_PWMB, constrain(pwm, 0, 255));
}

void setMotorsSpeed(int pwm) {
    setMotorASpeed(pwm);
    setMotorBSpeed(pwm);
}

void stopMotors() {
    digitalWrite(PIN_MOTOR_AIN1, LOW);
    digitalWrite(PIN_MOTOR_AIN2, LOW);
    digitalWrite(PIN_MOTOR_BIN1, LOW);
    digitalWrite(PIN_MOTOR_BIN2, LOW);
    analogWrite(PIN_MOTOR_PWMA, 0);
    analogWrite(PIN_MOTOR_PWMB, 0);
}

void brakeMotors() {
    digitalWrite(PIN_MOTOR_AIN1, HIGH);
    digitalWrite(PIN_MOTOR_AIN2, HIGH);
    digitalWrite(PIN_MOTOR_BIN1, HIGH);
    digitalWrite(PIN_MOTOR_BIN2, HIGH);
    analogWrite(PIN_MOTOR_PWMA, 0);
    analogWrite(PIN_MOTOR_PWMB, 0);
}
