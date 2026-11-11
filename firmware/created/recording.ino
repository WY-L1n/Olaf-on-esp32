
#include <Arduino.h>
#include <driver/i2s.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>

// INMP441 pins
#define I2S_WS   6
#define I2S_SD   4
#define I2S_SCK  7

// TF card SPI pins
#define SD_CS    10
#define SD_MOSI  11
#define SD_MISO  13
#define SD_SCK   12

#define SAMPLE_RATE 16000
#define RECORD_SECONDS 10
#define WAV_FILE "/mic_test.wav"

File wavFile;

void writeWavHeader(File &file, uint32_t dataSize) {
  uint32_t fileSize = dataSize + 36;
  uint16_t audioFormat = 1;
  uint16_t channels = 1;
  uint16_t bitsPerSample = 16;
  uint32_t byteRate = SAMPLE_RATE * channels * bitsPerSample / 8;
  uint16_t blockAlign = channels * bitsPerSample / 8;
  uint32_t sampleRate = SAMPLE_RATE;

  file.write((const uint8_t*)"RIFF", 4);
  file.write((uint8_t*)&fileSize, 4);
  file.write((const uint8_t*)"WAVE", 4);
  file.write((const uint8_t*)"fmt ", 4);

  uint32_t subChunk1Size = 16;
  file.write((uint8_t*)&subChunk1Size, 4);
  file.write((uint8_t*)&audioFormat, 2);
  file.write((uint8_t*)&channels, 2);
  file.write((uint8_t*)&sampleRate, 4);
  file.write((uint8_t*)&byteRate, 4);
  file.write((uint8_t*)&blockAlign, 2);
  file.write((uint8_t*)&bitsPerSample, 2);

  file.write((const uint8_t*)"data", 4);
  file.write((uint8_t*)&dataSize, 4);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\nINMP441 recording test");

  // Initialize TF card
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI, 20000000)) {
    Serial.println("ERROR: TF card initialization failed");
    return;
  }

  Serial.println("TF card OK");

  // Configure I2S
  i2s_config_t i2s_config = {};
  i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  i2s_config.sample_rate = SAMPLE_RATE;
  i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
  i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  i2s_config.dma_buf_count = 8;
  i2s_config.dma_buf_len = 512;
  i2s_config.use_apll = false;
  i2s_config.tx_desc_auto_clear = false;
  i2s_config.fixed_mclk = 0;

  if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
    Serial.println("ERROR: I2S driver install failed");
    return;
  }

  i2s_pin_config_t pin_config = {};
  pin_config.bck_io_num = I2S_SCK;
  pin_config.ws_io_num = I2S_WS;
  pin_config.data_out_num = I2S_PIN_NO_CHANGE;
  pin_config.data_in_num = I2S_SD;

  if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) {
    Serial.println("ERROR: I2S pin setup failed");
    return;
  }

  i2s_zero_dma_buffer(I2S_NUM_0);
  delay(500);

  // Open WAV file
  SD.remove(WAV_FILE);
  wavFile = SD.open(WAV_FILE, FILE_WRITE);

  if (!wavFile) {
    Serial.println("ERROR: cannot create WAV file");
    return;
  }

  // Reserve WAV header
  uint8_t emptyHeader[44] = {};
  wavFile.write(emptyHeader, sizeof(emptyHeader));

  Serial.println("Recording for 10 seconds...");
  Serial.println("Please keep the microphone in a quiet environment.");

  const size_t samplesPerRead = 512;
  int32_t rawBuffer[samplesPerRead];
  int16_t pcmBuffer[samplesPerRead];

  const uint32_t totalSamples = SAMPLE_RATE * RECORD_SECONDS;
  uint32_t recordedSamples = 0;
  int64_t sumSamples = 0;
  double sumSquares = 0;
  int32_t minSample = INT32_MAX;
  int32_t maxSample = INT32_MIN;
  int32_t testBuffer[16];
  size_t testBytes = 0;

  i2s_read(I2S_NUM_0, testBuffer, sizeof(testBuffer),
         &testBytes, portMAX_DELAY);

  Serial.println("Raw I2S samples:");
  for (int i = 0; i < testBytes / 4; i++) {
    Serial.printf("%08lX  %ld\n",
                (unsigned long)testBuffer[i],
                (long)testBuffer[i]);
}
  while (recordedSamples < totalSamples) {
    size_t bytesRead = 0;

    esp_err_t result = i2s_read(
      I2S_NUM_0,
      rawBuffer,
      sizeof(rawBuffer),
      &bytesRead,
      portMAX_DELAY
    );

    if (result != ESP_OK || bytesRead == 0) {
      Serial.println("I2S read error");
      break;
    }

    size_t samplesRead = bytesRead / sizeof(int32_t);

    for (size_t i = 0; i < samplesRead; i++) {
      // INMP441 24-bit data -> 16-bit PCM
      int32_t sample = rawBuffer[i] >> 16;
      sumSamples += sample;
      sumSquares += (double)sample * sample;

      if (sample < minSample) minSample = sample;
      if (sample > maxSample) maxSample = sample;

      if (sample > 32767) sample = 32767;
      if (sample < -32768) sample = -32768;

      pcmBuffer[i] = (int16_t)sample;
    }

    uint32_t remaining = totalSamples - recordedSamples;
    if (samplesRead > remaining) {
      samplesRead = remaining;
    }

    wavFile.write(
      (const uint8_t*)pcmBuffer,
      samplesRead * sizeof(int16_t)
    );

    recordedSamples += samplesRead;
  }
  if (recordedSamples > 0) {
  Serial.println("Audio statistics:");

  Serial.printf("Average: %lld\n",
                (long long)(sumSamples / recordedSamples));

  Serial.printf("Minimum: %ld\n", (long)minSample);
  Serial.printf("Maximum: %ld\n", (long)maxSample);
  double rms = sqrt(sumSquares / recordedSamples);

  Serial.printf("RMS: %.2f\n", rms);
  }
  // Rewrite WAV header with actual data length
  uint32_t dataSize = recordedSamples * sizeof(int16_t);
  wavFile.seek(0);
  writeWavHeader(wavFile, dataSize);
  wavFile.close();

  i2s_driver_uninstall(I2S_NUM_0);

  Serial.println("Recording finished!");
  Serial.println("Saved as: mic_test.wav");
  Serial.printf("Recorded samples: %lu\n", (unsigned long)recordedSamples);
  Serial.println("You can now remove the TF card.");
}

void loop() {
  delay(1000);
}
