#include <WiFi.h>
#include <WiFiClient.h>
#include <I2S.h>
//#include "audio_sample.h"

// I2S 設定
#define pDOUT 19
#define pBCLK 20
#define pWS   21
#define pMCLK 22  // 可選的 MCLK 腳位
float volume;
const int potPin = 26; // GPIO26 = ADC0
const unsigned char audioData[]={} ;
const int audioDataLen = sizeof(audioData);


I2S i2s(OUTPUT);

// WiFi 設定
const char* ssid = "iSpan-R201";
const char* password = "66316588";
WiFiServer server(5000);  // 建立伺服器監聽 5000 端口
WiFiClient client;


// 新增音訊緩衝區相關變數
const size_t MAX_AUDIO_SIZE = 156200;  // 約 1 秒的 16-bit 44100Hz 單聲道
uint8_t audioBuffer[MAX_AUDIO_SIZE];
size_t receivedBytes = 0;

// 音訊資料變數
const uint8_t* currentAudioData = audioData;
uint32_t currentAudioLen = audioDataLen;
uint32_t audioIndex = 0;
bool hasNewAudio = false;


void receiveNewAudio() {
  receivedBytes = 0;
  Serial.println("📥 接收音訊中...");
  
  unsigned long startTime = millis();
  const unsigned long timeout = 30000;  // 30 秒超時
  
  while (client.connected() && receivedBytes < MAX_AUDIO_SIZE) {
    if (millis() - startTime > timeout) {
      Serial.println("接收超時");
      break;
    }
    
    if (client.available()) {
      int available = client.available();
      if (available > 0) {
        // 每次讀取較小的數據塊
        int toRead = min(available, min(512, MAX_AUDIO_SIZE - receivedBytes));
        int len = client.read(audioBuffer + receivedBytes, toRead);
        if (len > 0) {
          receivedBytes += len;
          // 每接收 10KB 打印一次進度
          if (receivedBytes % 10240 == 0) {
            Serial.printf("已接收: %d bytes\n", receivedBytes);
          }
        }
        // 給系統一些處理時間
        delay(5);
      }
    }
  }

  if (receivedBytes > 0) {
    Serial.printf("✅ 接收完成，共 %d bytes\n", receivedBytes);
    currentAudioData = audioBuffer;
    currentAudioLen = receivedBytes;
    audioIndex = 0;
    hasNewAudio = true;
  } else {
    Serial.println("❌ 未接收到數據");
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("連接 WiFi...");
  //Wifi setting

  Serial.println("連接 WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n已連上 WiFi");
  Serial.println("IP: " + WiFi.localIP().toString());
  
  server.begin();  // 啟動伺服器
  Serial.println("伺服器已啟動");


  // 設定 I2S 腳位與參數
  i2s.setBCLK(pBCLK);
  i2s.setDATA(pDOUT);
  i2s.setMCLK(pMCLK);
  i2s.setMCLKmult(256); // MCLK = Fs * 256
  i2s.setBitsPerSample(16);
  
  // 設定系統時鐘以匹配音訊取樣率 24000 Hz
  i2s.setSysClk(24000);  // 修改為 24000 Hz

  // 開始 I2S，設置取樣率為 24000
  if (!i2s.begin(24000)) {
    Serial.println("I2S 初始化失敗!");
    while (true);
  }
}

void loop() {
  int potValue = analogRead(potPin);         // 讀取 ADC
  volume = potValue / 4095.0f;         // 轉成 0.0 ~ 1.0 音量係數
  // 檢查是否有來自伺服器的新音訊
 //Serial.printf("客戶端狀態: %d, 可讀取數據: %d\n", client.connected(), client.available());
  
if (!client || !client.connected()) {
    client = server.accept();
    if (client) {
      Serial.println("Client connected");
    }
  }


  if(client.connected() ){
    Serial.printf("客戶端狀態: %d, 可讀取數據: %d\n", client.connected(), client.available());
  if ( client.available()) {
    Serial.println("收到新音訊，開始更新...");
    receiveNewAudio();
  }
}
  // 播放音訊樣本
  playAudio();
}

void playAudio() {
  if (currentAudioLen == 0) return;

  if (hasNewAudio) {
    // 播放新接收的音訊
    int16_t sample = audioBuffer[audioIndex] | (audioBuffer[audioIndex + 1] << 8);
    sample = sample * volume;  //可調整音量
    i2s.write(sample); // 左聲道
    i2s.write(sample); // 右聲道

    audioIndex += 2;
    if (audioIndex >= receivedBytes) {
      audioIndex = 0; // 循環播放
    }
  } else {
    // 播放預設音訊
    int16_t sample = currentAudioData[audioIndex] | (currentAudioData[audioIndex + 1] << 8);
    sample = sample * volume;  //可調整音量
    i2s.write(sample); // 左聲道
    i2s.write(sample); // 右聲道

    audioIndex += 2;
    if (audioIndex >= currentAudioLen) {
      audioIndex = 0; // 循環播放
    }
  }
}
