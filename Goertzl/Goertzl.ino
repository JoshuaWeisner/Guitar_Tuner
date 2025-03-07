#include <driver/i2s.h>
#include <math.h>

// I2S pins definition
#define I2S_SD 36   // Serial Data (SD)
#define I2S_WS 38   // LR Clock (Word Select)
#define I2S_SCK 40  // Bit Clock (BCLK)
#define I2S_PORT I2S_NUM_0

// Goertzel and sampling parameters
#define SAMPLE_RATE 16000         // 16kHz sampling rate
#define BUFFER_SIZE 512           // Buffer size
#define FIRST_PASS_MIN_FREQ 50.0  // Lowest frequency to scan
#define FIRST_PASS_MAX_FREQ 350.0 // Highest frequency to scan
#define FIRST_PASS_STEP 5        // First pass step size (10 Hz) //adjusted to 5 Hz
#define SECOND_PASS_RANGE 10       // Range for second pass. //adjusted to 10 bins
#define SECOND_PASS_STEP 1        // Second pass step size (1 Hz)

// Guitar strings standard tuning frequencies (E2, A2, D3, G3, B3, E4)
const float GUITAR_NOTES[6] = { 82.41, 110.00, 146.83, 196.00, 246.94, 329.63 };
const char* NOTE_NAMES[6] = { "E2", "A2", "D3", "G3", "B3", "E4" };

// Buffers
int32_t i2s_samples[BUFFER_SIZE];
float audio_samples[BUFFER_SIZE];

// Debouncing array to store detected string indices
#define DEBOUNCE_SIZE 4
int string_history[DEBOUNCE_SIZE] = {-1, -1, -1, -1};
int history_pos = 0;

// Function for Goertzel algorithm to detect magnitude at a frequency
float goertzel(float* samples, int sample_count, float target_freq) {
  float omega = 2.0 * M_PI * target_freq / SAMPLE_RATE;
  float coeff = 2.0 * cos(omega);
  float q0 = 0, q1 = 0, q2 = 0;

  // Process all samples through the Goertzel filter
  for (int i = 0; i < sample_count; i++) {
    q0 = coeff * q1 - q2 + samples[i];
    q2 = q1;
    q1 = q0;
  }

  // Calculate magnitude
  float real = q1 - q2 * cos(omega);
  float imag = q2 * sin(omega);
  float magnitude = sqrt(real*real + imag*imag);
  
  return magnitude;
}

// Pre-emphasis filter to reduce lower frequency bias
void applyPreEmphasis(float* samples, int count) {
  float prev_sample = 0;
  float alpha = 0.85; // Pre-emphasis factor.   changed from 0.85 to 0.95
  
  for (int i = 0; i < count; i++) {
    float current = samples[i];
    samples[i] = current - alpha * prev_sample;
    prev_sample = current;
  }
}

// Calculate RMS of audio buffer
float calculateRMS(float* samples, int count) {
  float sum = 0;
  for (int i = 0; i < count; i++) {
    sum += samples[i] * samples[i];
  }
  return sqrt(sum / count);
}

// First pass to find approximate frequency using 10 Hz steps
float firstPassScan() {
  float best_freq = 0;
  float max_magnitude = 0;
  
  // Create normalized magnitude array for peak detection
  int freq_count = (FIRST_PASS_MAX_FREQ - FIRST_PASS_MIN_FREQ) / FIRST_PASS_STEP + 1;
  float magnitudes[freq_count];
  float freqs[freq_count];
  int idx = 0;
  
  // First compute all magnitudes
  for (float freq = FIRST_PASS_MIN_FREQ; freq <= FIRST_PASS_MAX_FREQ; freq += FIRST_PASS_STEP) {
    float magnitude = goertzel(audio_samples, BUFFER_SIZE, freq);
    magnitudes[idx] = magnitude;
    freqs[idx] = freq;
    idx++;
  }
  
  // Find the global maximum magnitude
  float global_max = 0;
  for (int i = 0; i < freq_count; i++) {
    if (magnitudes[i] > global_max) {
      global_max = magnitudes[i];
    }
  }
  
  // Normalize magnitudes and find peaks
  if (global_max > 0) {
    for (int i = 0; i < freq_count; i++) {
      magnitudes[i] /= global_max; // Normalize to 0-1 range
    }
    
    // Find peak (must be higher than neighbors and above threshold)
    for (int i = 1; i < freq_count-1; i++) {
      if (magnitudes[i] > 0.5 && // Must be at least 50% of max magnitude
          magnitudes[i] > magnitudes[i-1] && 
          magnitudes[i] > magnitudes[i+1]) {
        // Found a peak - if it's the largest so far, remember it
        if (magnitudes[i] > max_magnitude) {
          max_magnitude = magnitudes[i];
          best_freq = freqs[i];
        }
      }
    }
  }
  
  return best_freq;
}

// Second pass for more precise frequency detection
float secondPassScan(float center_freq) {
  float best_freq = center_freq;
  float max_magnitude = 0;
  
  for (float freq = center_freq - SECOND_PASS_RANGE; 
       freq <= center_freq + SECOND_PASS_RANGE; 
       freq += SECOND_PASS_STEP) {
    float magnitude = goertzel(audio_samples, BUFFER_SIZE, freq);
    if (magnitude > max_magnitude) {
      max_magnitude = magnitude;
      best_freq = freq;
    }
  }
  
  return best_freq;
}

// Find closest guitar string
int findClosestString(float freq) {
  int best_idx = 0;
  float min_cents_diff = 1000.0; // Large initial value
  
  for (int i = 0; i < 6; i++) {
    float cents = 1200 * log2(freq / GUITAR_NOTES[i]);
    float abs_cents = fabs(cents);
    
    if (abs_cents < min_cents_diff) {
      min_cents_diff = abs_cents;
      best_idx = i;
    }
  }
  
  // Only return a valid string if we're within a reasonable range (e.g., 100 cents = 1 semitone)
  return (min_cents_diff <= 100) ? best_idx : -1;
}

// Check if a string index is consistently detected in history
bool isConsistentString(int string_idx) {
  if (string_idx < 0) return false;
  
  int count = 0;
  for (int i = 0; i < DEBOUNCE_SIZE; i++) {
    if (string_history[i] == string_idx) {
      count++;
    }
  }
  
  return (count >= 3); // At least 3 of 4 samples match
}

// Add string index to history
void addToHistory(int string_idx) {
  string_history[history_pos] = string_idx;
  history_pos = (history_pos + 1) % DEBOUNCE_SIZE;
}

void setup() {
  // Initialize Serial
  Serial.begin(115200);
  delay(1000);
  
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
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);
  
  Serial.println("Guitar Tuner Ready");
  Serial.println("Play a string to begin tuning...");
}

void loop() {
  // Read audio samples from I2S microphone
  size_t bytes_read = 0;
  esp_err_t result = i2s_read(I2S_PORT, i2s_samples, sizeof(i2s_samples), &bytes_read, portMAX_DELAY);
  
  if (result == ESP_OK) {
    // Convert samples to float and apply windowing
    for (int i = 0; i < BUFFER_SIZE; i++) {
      // Convert to float [-1.0, 1.0]
      audio_samples[i] = i2s_samples[i] / 8388607.0f;
      
      // Apply Hann window to reduce spectral leakage
      float window = 0.5 * (1 - cos(2 * M_PI * i / (BUFFER_SIZE - 1)));
      audio_samples[i] *= window;
    }
    
    // Apply pre-emphasis to boost higher frequencies
    applyPreEmphasis(audio_samples, BUFFER_SIZE);
    
    // Check if audio level is sufficient
    float rms = calculateRMS(audio_samples, BUFFER_SIZE);
    if (rms < 0.002) { // Minimum threshold for valid signal
      delay(20);
      return;
    }
    
    // Step 1: First pass scan to find approximate frequency
    float coarse_freq = firstPassScan();
    
    if (coarse_freq > 0) {
      // Step 2: Second pass for more precise frequency
      float fine_freq = secondPassScan(coarse_freq);
      
      // Step 3: Find closest string
      int string_idx = findClosestString(fine_freq);
      
      // Step 4: Add to history for debouncing
      addToHistory(string_idx);
      
      // Step 5: Only output if we have consistent detection
      if (isConsistentString(string_idx)) {
        // Calculate cents difference from target note
        float target = GUITAR_NOTES[string_idx];
        float cents = 1200 * log2(fine_freq / target);
        
        // Print tuning information
        Serial.printf("Frequency: %.1f Hz, String: %s, Cents: %+.1f\n", 
                     fine_freq, NOTE_NAMES[string_idx], cents);
        
        // Display tuning guidance
        if (cents < -10.0) {
          Serial.println("▼ Too low - tighten string");
        } else if (cents > 10.0) {
          Serial.println("▲ Too high - loosen string");
        } else {
          Serial.println("✓ In tune (within ±10 cents)");
        }
        
        Serial.println();
      }
    }
  }
  
  delay(30); // Small delay between measurements
}