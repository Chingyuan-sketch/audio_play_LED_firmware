const express = require("express");
const fs = require("fs");
const net = require("net");
const path = require("path");
const multer = require("multer");
const { execSync } = require("child_process");

const app = express();
const port = 3000;
// 修改為您的 Pico W 實際 IP 地址
const picoWHost = '192.168.51.138'; 
const picoWPort = 5000;
const modeFile = "/tmp/ws2812_mode.txt";

// 目錄結構
const uploadDir = path.join(__dirname, "audio/uploads");
const convertedDir = path.join(__dirname, "audio/converted");
fs.mkdirSync(uploadDir, { recursive: true });
fs.mkdirSync(convertedDir, { recursive: true });

// 靜態頁面
app.use(express.static(path.join(__dirname, "public")));

// 上傳處理器
const storage = multer.diskStorage({
  destination: uploadDir,
  filename: (req, file, cb) => cb(null, Date.now() + path.extname(file.originalname))
});
const upload = multer({ storage });

const CHUNK_SIZE = 1024;  // 增加每次傳送的數據量

function sendAudioInChunks(socket, audioBuffer) {
  let offset = 0;
  let isCompleted = false;  // 添加完成標記

  function sendNextChunk() {
    if (isCompleted) return;  // 如果已完成則不再傳送
    
    if (offset >= audioBuffer.length) {
      isCompleted = true;  // 標記為已完成
      console.log("所有數據傳送完成");
      setTimeout(() => {
        socket.end();
        socket.destroy();  // 確保連接完全關閉
      }, 2000);
      return;
    }

    const end = Math.min(offset + CHUNK_SIZE, audioBuffer.length);
    const chunk = audioBuffer.slice(offset, end);
    
    try {
      const canContinue = socket.write(chunk);
      offset = end;
      
      // 添加進度日誌
      console.log(`傳送進度: ${Math.floor((offset / audioBuffer.length) * 100)}%`);
      
      if (!canContinue) {
        socket.once('drain', () => {
          if (!isCompleted) {  // 確認未完成才繼續
            setTimeout(sendNextChunk, 100);
          }
        });
      } else {
        if (!isCompleted) {  // 確認未完成才繼續
          setTimeout(sendNextChunk, 100);
        }
      }
    } catch (err) {
      console.error('傳送錯誤:', err);
      socket.destroy();
    }
  }
  sendNextChunk();
}

app.post("/upload", upload.single("audioFile"), async (req, res) => {
  const filePath = req.file.path;
  const baseName = path.basename(filePath, path.extname(filePath));
  const wavPath = path.join(convertedDir, baseName + ".wav");

  try {
    try {
      execSync(`ffmpeg -y -i "${filePath}" -ar 24000 -ac 1 -f wav "${wavPath}"`, {
        stdio: 'pipe',
        encoding: 'utf-8'
      });
    } catch (ffmpegErr) {
      console.error("FFmpeg 錯誤:", ffmpegErr.stderr);
      throw new Error("音訊轉換失敗");
    }

    if (!fs.existsSync(wavPath)) {
      throw new Error("轉換後的檔案不存在");
    }

    const audioBytes = fs.readFileSync(wavPath).slice(44);

    let retryCount = 0;
    const maxRetries = 5;  // 增加重試次數
    const connectTimeout = 30000;  // 增加連接超時時間到 30 秒

    const socket = new Promise((resolve, reject) => {
      function tryConnect() {
        console.log(`嘗試連接到 Pico W (${retryCount + 1}/${maxRetries})...`);
        
        const connection = net.createConnection({ 
          host: picoWHost, 
          port: picoWPort,
          timeout: connectTimeout
        });

        // 設置 TCP keepalive
        connection.setKeepAlive(true, 1000);

        connection.on("connect", () => {
          console.log("成功連接到 Pico W");
          console.log("開始傳送音訊數據...");
          
          // 設置一次性監聽器
          connection.once("end", () => {
            console.log("傳輸結束，連接已關閉");
            connection.destroy();
          });
          
          sendAudioInChunks(connection, audioBytes);
          resolve("✅ 音訊傳送中（分段）");
        });

        connection.on("error", (err) => {
          console.error(`TCP 錯誤 (嘗試 ${retryCount + 1}/${maxRetries}):`, err);
          connection.destroy();
          
          if (retryCount < maxRetries - 1) {
            retryCount++;
            console.log(`等待 3 秒後重試...`);
            setTimeout(tryConnect, 3000);  // 增加重試間隔到 3 秒
          } else {
            reject(new Error(`連接失敗，已重試 ${maxRetries} 次`));
          }
        });

        connection.on("timeout", () => {
          console.log("連接超時，準備重試");
          connection.destroy();
        });

        connection.on("close", () => {
          console.log("連接已關閉");
        });
      }

      tryConnect();
    });

    const result = await socket;
    res.send(result);

  } catch (err) {
    console.error("處理錯誤:", err.message);
    res.status(500).send(`❌ 音訊處理錯誤: ${err.message}`);
  } finally {
    try {
      if (fs.existsSync(filePath)) fs.unlinkSync(filePath);
      if (fs.existsSync(wavPath)) fs.unlinkSync(wavPath);
    } catch (cleanupErr) {
      console.error("清理檔案錯誤:", cleanupErr);
    }
  }
});


// 🔘 LED 控制
app.post("/led/:mode", (req, res) => {
  const mode = req.params.mode;
  if (!["0", "1", "2", "3"].includes(mode)) {
    return res.status(400).send("❌ 無效的 LED 模式");
  }

  fs.writeFile(modeFile, mode, err => {
    if (err) {
      console.error("LED 模式寫入錯誤:", err);
      res.status(500).send("❌ 寫入失敗");
    } else {
      res.send(`✅ LED 模式已設為 ${mode}`);
    }
  });
});

// 啟動
app.listen(port, () => {
  console.log(`🌐 Web 控制器已啟動: http://localhost:${port}`);
});
