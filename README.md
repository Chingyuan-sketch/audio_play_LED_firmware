# Smart Speaker System (audio_play_LED)
簡易的商場音訊廣播與LED控制系統。使用者可透過網頁上傳音訊檔，伺服器將檔案轉換後傳送給Pico W音訊模組播放，同時可透過按鈕切換LED顯示模式。

## 📁 專案結構

```bash
audio_play_LED/
├── README.md               # 本說明文件
├── server.js               # Node.js Express TCP傳送器 + LED控制伺服器
├── wav.py                  # Python工具：將中文轉為mp3並轉換為wav
├── audio_sample.h          # Raspberry Pi Pico W韌體中的音訊資料（轉成byte陣列）
├── I2S.ino                 # Raspberry Pi Pico W主程式，接收音訊並透過I2S播放
│
├── audio/                  # 音訊相關目錄
│   ├── uploads/            # 上傳的原始音檔 (自動清除)
│   └── converted/          # 經ffmpeg轉為24000Hz wav的音訊 (自動清除)
│
├── public/                 # 前端頁面
│   └── index.html          # 網頁操作介面（上傳音訊、切換LED模式）
│
├── kernel_driver/          # Linux核心模組（驅動WS2812）
│   ├── meme-ws2812.c       # SPI介面的WS2812驅動原始碼
│   ├── meme-ws2812.dts     # Device tree source，定義compatible = "meme,ws2812"
│   ├── meme-ws2812.mod.c   # 編譯自動產生模組描述檔（非原始碼）
│   ├── Makefile            # 編譯驅動模組的腳本
│
├── play.c                  # 使用者空間程式：讀取ws2812_mode.txt控制LED模式
├── ws2812_mode.txt         # 暫存LED模式（0~3）
```

## 🔧 使用方式

### Node.js後端

1. 安裝依賴：
```bash
npm install express multer net
```

2. 需安裝ffmpeg：
```bash
sudo apt install ffmpeg
```

3. 啟動伺服器：
```bash
node server.js
```

### 前端頁面

透過瀏覽器開啟`http://localhost:3000`，即可上傳音訊或切換LED模式。

### Pico W韌體

請使用Arduino IDE上傳`I2S.ino`至Pico W，並接好MAX98357A I2S模組與旋鈕：
- 音訊腳位：DOUT=19, BCLK=20, WS=21, MCLK=22
- 音量旋鈕：GPIO26 (ADC0)

### LED核心模組

1. 進入kernel_driver資料夾
```bash
cd audio_play_LED/kernel_driver
make
sudo insmod meme-ws2812.ko
```

2. 執行LED模式控制程式：
```bash
gcc ../play.c -o play
sudo ./play
```

### 文字轉音訊工具

將中文文字轉為語音wav檔：
```bash
python3 wav.py
```
會產生`ad2.wav`可供上傳。

## 🎯 特色

- 透過TCP socket傳送音訊資料至Raspberry Pi Pico W，並立即播放
- Node.js伺服器自動轉檔（mp3 → wav → byte array）
- Raspberry Pi Pico W音訊播放支援音量旋鈕控制
- 自製SPI WS2812 Linux driver，搭配使用者空間控制程式`play.c`

## 📌 注意事項

- 傳送至Raspberry Pi Pico W的音訊需為24000Hz, mono, 16-bit PCM格式
- 每次播放僅支援一筆資料，若需連續播放可延伸buffer queue
- WS2812驅動採SPI傳輸方式，請確認硬體接腳符合device tree設定