# Smart Speaker System (audio_play_LED)
    簡易的商場音訊廣播與 LED 控制系統。使用者可透過網頁上傳音訊檔，伺服器將檔案轉換後傳送給 Raspberry Pi 5 音訊模組播放，同時可透過按鈕切換 LED 顯示模式。

## 📁 專案結構

```bash
audio_play_LED/
├── README.md               # 本說明文件
├── server.js               # Node.js Express TCP 傳送器 + LED 控制伺服器
├── wav.py                  # Python 工具：將中文轉為 mp3 並轉換為 wav
├── audio_sample.h          # Raspberry Pi 5 韌體中的音訊資料（轉成 byte 陣列）
├── I2S.ino                 # Raspberry Pi 5 主程式，接收音訊並透過 I2S 播放
│
├── audio/                  # 音訊相關目錄
│   ├── uploads/            # 上傳的原始音檔 (自動清除)
│   └── converted/          # 經 ffmpeg 轉為 24000Hz wav 的音訊 (自動清除)
│
├── public/                 # 前端頁面
│   └── index.html          # 網頁操作介面（上傳音訊、切換 LED 模式）
│
├── kernel_driver/          # Linux 核心模組（驅動 WS2812）
│   ├── meme-ws2812.c       # ✅ SPI 介面的 WS2812 驅動原始碼（由使用者補完版本）
│   ├── meme-ws2812.dts     # Device tree source，定義 compatible = "meme,ws2812"
│   ├── meme-ws2812.mod.c   # 編譯自動產生模組描述檔（非原始碼）
│   ├── Makefile            # 編譯驅動模組的腳本
│
├── play.c                  # 使用者空間程式：讀取 ws2812_mode.txt 控制 LED 模式
├── ws2812_mode.txt         # 暫存 LED 模式（0~3）
```

## 🔧 使用方式

### Node.js 後端

1. 安裝依賴：
```bash
npm install express multer net
```

2. 需安裝 ffmpeg：
```bash
sudo apt install ffmpeg
```

3. 啟動伺服器：
```bash
node server.js
```

### 前端頁面

透過瀏覽器開啟 `http://localhost:3000`，即可上傳音訊或切換 LED 模式。

### Raspberry Pi 5 韌體

請使用 Arduino IDE 上傳 `I2S.ino` 至 Raspberry Pi 5，並接好 MAX98357A I2S 模組與旋鈕：
- 音訊腳位：DOUT=19, BCLK=20, WS=21, MCLK=22
- 音量旋鈕：GPIO26 (ADC0)

### LED 核心模組

1. 進入 kernel_driver 資料夾
```bash
cd audio_play_LED/kernel_driver
make
sudo insmod meme-ws2812.ko
```

2. 執行 LED 模式控制程式：
```bash
gcc ../play.c -o play
sudo ./play
```

### 文字轉音訊工具

將中文文字轉為語音 wav 檔：
```bash
python3 wav.py
```
會產生 `ad2.wav` 可供上傳。

## 🎯 特色

- 透過 TCP socket 傳送音訊資料至 Raspberry Pi 5，並立即播放
- Node.js 伺服器自動轉檔（mp3 → wav → byte array）
- Raspberry Pi 5 音訊播放支援音量旋鈕控制
- 自製 SPI WS2812 Linux driver，搭配使用者空間控制程式 `play.c`

## 📌 注意事項

- 傳送至 Raspberry Pi 5 的音訊需為 24000Hz, mono, 16-bit PCM 格式
- 每次播放僅支援一筆資料，若需連續播放可延伸 buffer queue
- WS2812 驅動採 SPI 傳輸方式，請確認硬體接腳符合 device tree 設定
