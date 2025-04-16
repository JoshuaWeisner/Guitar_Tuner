/*
 * ESP32 INMP441 I2S Microphone Test
 * 
 * This sketch tests the INMP441 I2S MEMS microphone by:
 * 1. Reading audio samples using I2S interface
 * 2. Calculating audio levels (RMS, Peak)
 * 3. Displaying a simple audio level meter in the serial monitor
 * 
 * Connections:
 * - INMP441 SD  -> ESP32 GPIO 40 (Serial Data)
 * - INMP441 WS  -> ESP32 GPIO 38 (Word Select/LR Clock)
 * - INMP441 SCK -> ESP32 GPIO 36 (Serial Clock)
 * - INMP441 VDD -> 3.3V
 * - INMP441 GND -> GND
 */

#include <driver/i2s.h>
#include <math.h>

// I2S pins definition
#define I2S_SD 40   // Serial Data (SD)
#define I2S_WS 38   // LR Clock (Word Select)
#define I2S_SCK 36  // Bit Clock (BCLK)
#define I2S_PORT I2S_NUM_0

// Sampling parameters
#define SAMPLE_RATE 16000
#define BUFFER_SIZE 512

// Buffers
int32_t i2s_samples[BUFFER_SIZE];
float audio_samples[BUFFER_SIZE];

// Calculate RMS of audio buffer
float calculateRMS(float* samples, int count) {
  float sum = 0;
  for (int i = 0; i < count; i++) {
    sum += samples[i] * samples[i];
  }
  return sqrt(sum / count);
}

// Find peak amplitude in audio buffer
float findPeak(float* samples, int count) {
  float peak = 0;
  for (int i = 0; i < count; i++) {
    float abs_val = fabs(samples[i]);
    if (abs_val > peak) {
      peak = abs_val;
    }
  }
  return peak;
}

// Display an ASCII level meter
void displayLevelMeter(float level, float max_level) {
  // Scale to 0-50 range for display
  int meter_level = (int)(level / max_level * 50);
  if (meter_level > 50) meter_level = 50;
  
  // Print the level meter
  Serial.print("[");
  for (int i = 0; i < 50; i++) {
    if (i < meter_level) {
      // Different characters for different levels
      if (i > 45) {
        Serial.print("!");
      } else if (i > 35) {
        Serial.print("*");
      } else {
        Serial.print("=");
      }
    } else {
      Serial.print(" ");
    }
  }
  Serial.print("] ");
  
  // Print the numerical value
  Serial.printf("%.4f\n", level);
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\nESP32 INMP441 I2S Microphone Test");
  Serial.println("================================");
  
  // Configure I2S
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  
  // Install and start I2S driver
  esp_err_t result = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    Serial.printf("Error installing I2S driver: %d\n", result);
    while (1); // Stop here if we can't install the driver
  }
  
  result = i2s_set_pin(I2S_PORT, &pin_config);
  if (result != ESP_OK) {
    Serial.printf("Error setting I2S pins: %d\n", result);
    while (1); // Stop here if we can't set the pins
  }
  
  i2s_zero_dma_buffer(I2S_PORT);
  
  Serial.println("I2S driver installed successfully");
  Serial.println("Microphone test starting...");
  Serial.println("Try making sounds near the microphone to see the level meter change");
  Serial.println("");
  
  // Header for the level meter display
  Serial.println("RMS Level:");
  delay(1000);
}

void loop() {
  // Read audio samples from I2S microphone
  size_t bytes_read = 0;
  esp_err_t result = i2s_read(I2S_PORT, i2s_samples, sizeof(i2s_samples), &bytes_read, portMAX_DELAY);
  
  if (result == ESP_OK) {
    // Convert samples to float [-1.0, 1.0]
    for (int i = 0; i < BUFFER_SIZE; i++) {
      // The raw value is 32-bit, but the actual data is 24-bit
      // So we divide by 2^23 to get the normalized float value
      audio_samples[i] = i2s_samples[i] / 8388607.0f;
    }
    
    // Calculate audio levels
    float rms = calculateRMS(audio_samples, BUFFER_SIZE);
    float peak = findPeak(audio_samples, BUFFER_SIZE);
    
    // Display level meter for RMS
    displayLevelMeter(rms, 0.1); // 0.1 is a reasonable max level for display scaling
    
    // Every second, also show peak and numerical values
    static unsigned long last_detailed_print = 0;
    if (millis() - last_detailed_print > 1000) {
      Serial.printf("\nDetailed Audio Metrics:\n");
      Serial.printf("  RMS Level: %.6f\n", rms);
      Serial.printf("  Peak Level: %.6f\n", peak);
      Serial.printf("  Crest Factor: %.2f\n", (rms > 0) ? (peak / rms) : 0);
      Serial.printf("  Bytes Read: %d\n", bytes_read);
      Serial.println("");
      last_detailed_print = millis();
    }
  } else {
    Serial.printf("Error reading from I2S: %d\n", result);
    delay(1000);
  }
  
  // Small delay to control update rate
  delay(50);
}