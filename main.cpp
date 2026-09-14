#include <driver/i2s.h>
#include <Arduino.h>

//==== I2S0 分时复用：INMP441 麦克风 RX / MAX98357A 功放 TX ====
#define I2S_PORT I2S_NUM_0

// --- MIC INMP441 ---
#define MIC_BCLK 5
#define MIC_WS   1
#define MIC_DATA 0
// --- SPK MAX98357A ---
#define SPK_BCLK 8
#define SPK_WS   7
#define SPK_DATA 10

#define REC_BTN 9     // ESP32-C3 SuperMini 板载BOOT按键
#define SAMPLE_RATE 16000
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#define CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT

const int MAX_SAMPLES = 16000 * 3; //最长3秒录音
int16_t audioBuffer[MAX_SAMPLES];
size_t recordedSamples = 0;
bool isRecording = false;

// I2S初始化为麦克风接收模式
void i2s_mic_init(){
  i2s_driver_uninstall(I2S_PORT);
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
    .channel_format = CHANNEL,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 512,
    .use_apll = false
  };
  i2s_pin_config_t pins = {
    .bck_io_num = MIC_BCLK,
    .ws_io_num = MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = MIC_DATA
  };
  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
}

// I2S初始化为喇叭输出模式
void i2s_spk_init(){
  i2s_driver_uninstall(I2S_PORT);
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = BITS_PER_SAMPLE,
    .channel_format = CHANNEL,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 512,
    .use_apll = false
  };
  i2s_pin_config_t pins = {
    .bck_io_num = SPK_BCLK,
    .ws_io_num = SPK_WS,
    .data_out_num = SPK_DATA,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_PORT, &cfg,0,NULL);
  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
}

void recordAudio(){
  i2s_mic_init();
  Serial.println("=== 录音开始 ===");
  recordedSamples = 0;
  size_t bytesRead;
  int16_t tempBuf[512];
  while(digitalRead(REC_BTN)==LOW && recordedSamples < MAX_SAMPLES){
    i2s_read(I2S_PORT, tempBuf, sizeof(tempBuf), &bytesRead, portMAX_DELAY);
    int samples = bytesRead / sizeof(int16_t);
    for(int i=0;i<samples;i++){
      audioBuffer[recordedSamples++] = tempBuf[i];
      if(recordedSamples >= MAX_SAMPLES) break;
    }
  }
  Serial.printf("录音结束，采样点：%d\n", recordedSamples);
}

void playAudio(){
  if(recordedSamples == 0) return;
  i2s_spk_init();
  Serial.println("=== 播放录音 ===");
  size_t bytesWritten;
  i2s_write(I2S_PORT, audioBuffer, recordedSamples*sizeof(int16_t), &bytesWritten, portMAX_DELAY);
  Serial.println("播放完成");
}

void setup() {
  Serial.begin(115200);
  pinMode(REC_BTN, INPUT_PULLUP);
  Serial.println("就绪，按住BOOT按键录音，松开自动播放");
}

void loop() {
  if(digitalRead(REC_BTN) == LOW && !isRecording){
    isRecording = true;
    recordAudio();
    playAudio();
    isRecording = false;
  }
  delay(50);
}
