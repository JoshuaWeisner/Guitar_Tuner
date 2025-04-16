/*
 * ESP32 TB6612FNG Motor Driver Test
 * 
 * This sketch tests the N30 5V DC motor by alternating between
 * clockwise and counter-clockwise rotation at different speeds.
 * 
 * Connections:
 * - TB6612FNG PWMA -> ESP32 GPIO 25
 * - TB6612FNG AIN1 -> ESP32 GPIO 26
 * - TB6612FNG AIN2 -> ESP32 GPIO 27
 * - TB6612FNG STBY -> 3.3V (connected externally)
 * - TB6612FNG VM   -> 5V (from boost converter)
 * - TB6612FNG VCC  -> 3.3V
 * - TB6612FNG GND  -> GND
 * - TB6612FNG A01  -> Motor Red Wire
 * - TB6612FNG A02  -> Motor Black Wire
 */

// Motor Driver Pins (TB6612FNG)
#define MOTOR_PWMA 25  // PWM control for motor A
#define MOTOR_AIN1 26  // Direction control 1 for motor A
#define MOTOR_AIN2 27  // Direction control 2 for motor A

// PWM configuration
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8     // 8-bit resolution (0-255)
#define PWM_FREQUENCY 1000   // 1 kHz

// Test parameters
#define TEST_DURATION 3000   // Duration of each test state in milliseconds
#define DELAY_BETWEEN 1000   // Delay between test states in milliseconds

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  
  // Configure PWM for motor control
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_PWMA, PWM_CHANNEL);
  
  // Configure direction control pins
  pinMode(MOTOR_AIN1, OUTPUT);
  pinMode(MOTOR_AIN2, OUTPUT);
  
  // Initialize with motor stopped
  stopMotor();
  
  Serial.println("\nESP32 TB6612FNG Motor Test");
  Serial.println("===========================");
  Serial.println("This test will run the motor in different directions and speeds.");
  Serial.println("Make sure all connections are secure.");
  Serial.println("Starting test sequence in 3 seconds...");
  delay(3000);
}

void loop() {
  // Test 1: Clockwise at half speed
  Serial.println("\nTest 1: Clockwise rotation at 50% speed");
  motorClockwise(128);
  delay(TEST_DURATION);
  stopMotor();
  delay(DELAY_BETWEEN);
  
  // Test 2: Counter-clockwise at half speed
  Serial.println("\nTest 2: Counter-clockwise rotation at 50% speed");
  motorCounterClockwise(128);
  delay(TEST_DURATION);
  stopMotor();
  delay(DELAY_BETWEEN);
  
  // Test 3: Clockwise at full speed
  Serial.println("\nTest 3: Clockwise rotation at 100% speed");
  motorClockwise(255);
  delay(TEST_DURATION);
  stopMotor();
  delay(DELAY_BETWEEN);
  
  // Test 4: Counter-clockwise at full speed
  Serial.println("\nTest 4: Counter-clockwise rotation at 100% speed");
  motorCounterClockwise(255);
  delay(TEST_DURATION);
  stopMotor();
  delay(DELAY_BETWEEN);
  
  // Test 5: Gradual speed increase (clockwise)
  Serial.println("\nTest 5: Clockwise with gradual speed increase");
  digitalWrite(MOTOR_AIN1, HIGH);
  digitalWrite(MOTOR_AIN2, LOW);
  
  for (int speed = 50; speed <= 255; speed += 5) {
    ledcWrite(PWM_CHANNEL, speed);
    Serial.printf("Speed: %d/255\n", speed);
    delay(100);
  }
  
  stopMotor();
  delay(DELAY_BETWEEN);
  
  // Test 6: Gradual speed increase (counter-clockwise)
  Serial.println("\nTest 6: Counter-clockwise with gradual speed increase");
  digitalWrite(MOTOR_AIN1, LOW);
  digitalWrite(MOTOR_AIN2, HIGH);
  
  for (int speed = 50; speed <= 255; speed += 5) {
    ledcWrite(PWM_CHANNEL, speed);
    Serial.printf("Speed: %d/255\n", speed);
    delay(100);
  }
  
  stopMotor();
  delay(DELAY_BETWEEN);
  
  // End of test cycle
  Serial.println("\nTest cycle complete. Restarting in 5 seconds...");
  delay(5000);
}

// Stop the motor
void stopMotor() {
  digitalWrite(MOTOR_AIN1, LOW);
  digitalWrite(MOTOR_AIN2, LOW);
  ledcWrite(PWM_CHANNEL, 0);
  Serial.println("Motor stopped");
  delay(500); // Short delay after stopping
}

// Run motor clockwise
void motorClockwise(int speed) {
  digitalWrite(MOTOR_AIN1, HIGH);
  digitalWrite(MOTOR_AIN2, LOW);
  ledcWrite(PWM_CHANNEL, speed);
}

// Run motor counter-clockwise
void motorCounterClockwise(int speed) {
  digitalWrite(MOTOR_AIN1, LOW);
  digitalWrite(MOTOR_AIN2, HIGH);
  ledcWrite(PWM_CHANNEL, speed);
}
