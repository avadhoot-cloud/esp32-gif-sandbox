/**
 * ESP32 + ST7789 240x240 sandbox
 * TFT baseline: init() + setRotation(1) (matches user's working sketch)
 * Serial upload at SANDBOX_SERIAL_BAUD (default 115200)
 */
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <AnimatedGIF.h>
#include <TFT_eSPI.h>

#ifndef SANDBOX_SERIAL_BAUD
#define SANDBOX_SERIAL_BAUD 115200
#endif

static TFT_eSPI tft = TFT_eSPI();
static AnimatedGIF gif;
static File gifFile;
static File uploadFile;

static bool gifPlaying = false;
static int gifXOff = 0, gifYOff = 0;
static uint8_t tftRotation = 1;

static bool binaryMode = false;
static bool uploadActive = false;
static size_t chunkExpected = 0;
static size_t chunkReceived = 0;
static size_t uploadTotal = 0;
static size_t uploadReceived = 0;
static String jsonLine;
static String uploadPath;

static const size_t SERIAL_RX_BUF = 16384;

static void protocolReply(const String& json) {
    Serial.println(json);
    Serial.flush();
}

static void tftInit(uint8_t rotation) {
    tftRotation = rotation;
    tft.init();
    tft.setRotation(rotation);
    tft.fillScreen(TFT_BLACK);
    tft.setSwapBytes(false);
}

static void runTftBaseline() {
    tftInit(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(40, 40);
    tft.println("ESP32");
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(20, 90);
    tft.println("ST7789 TFT");
    tft.drawRect(10, 10, 220, 220, TFT_RED);
    tft.fillCircle(120, 180, 30, TFT_BLUE);
}

static void GIFDraw(GIFDRAW* pDraw);
static void* gifOpen(const char* fname, int32_t* pSize);
static void gifClose(void* pHandle);
static int32_t gifRead(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen);
static int32_t gifSeek(GIFFILE* pFile, int32_t iPosition);

static bool openGif(const char* path) {
    if (gifPlaying) {
        gif.close();
        gifPlaying = false;
        if (gifFile) gifFile.close();
        tft.endWrite();
    }
    gif.begin(BIG_ENDIAN_PIXELS);
    if (!gif.open(path, gifOpen, gifClose, gifRead, gifSeek, GIFDraw)) {
        return false;
    }
    gifXOff = (tft.width() - gif.getCanvasWidth()) / 2;
    gifYOff = (tft.height() - gif.getCanvasHeight()) / 2;
    tft.startWrite();
    gifPlaying = true;
    return true;
}

static void updateGif() {
    if (!gifPlaying) return;
    static unsigned long lastFrame = 0;
    static int delayMs = 50;
    if (millis() - lastFrame < (unsigned long)delayMs) return;

    int frameDelay = 0;
    int r = gif.playFrame(false, &frameDelay);
    if (r < 0) return;
    if (gif.getLastError() == GIF_SUCCESS) {
        lastFrame = millis();
        if (frameDelay > 0) delayMs = frameDelay;
    }
    if (r == 0) {
        gif.reset();
        lastFrame = 0;
    }
}

static bool formatFs() {
    if (gifPlaying) {
        gif.close();
        gifPlaying = false;
        if (gifFile) gifFile.close();
        tft.endWrite();
    }
    LittleFS.end();
    if (!LittleFS.format()) return false;
    return LittleFS.begin(false);
}

static String listGifs() {
    String j = "[";
    File root = LittleFS.open("/");
    bool first = true;
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
        String name = f.name();
        if (!name.endsWith(".gif") && !name.endsWith(".GIF")) continue;
        if (!first) j += ",";
        j += "\"" + name + "\"";
        first = false;
    }
    j += "]";
    return j;
}

static void handleCommand(const String& line) {
    JsonDocument doc;
    if (deserializeJson(doc, line)) {
        protocolReply("{\"status\":\"error\",\"message\":\"bad json\"}");
        return;
    }
    const char* cmd = doc["cmd"];
    if (!cmd) {
        protocolReply("{\"status\":\"error\",\"message\":\"missing cmd\"}");
        return;
    }
    String c = cmd;
    if (c == "PING") {
        protocolReply("{\"status\":\"success\",\"message\":\"pong\"}");
    } else if (c == "TFT_BASELINE") {
        runTftBaseline();
        protocolReply("{\"status\":\"success\",\"message\":\"tft baseline\"}");
    } else if (c == "TFT_ROT") {
        tftInit((uint8_t)(doc["value"] | 1));
        protocolReply("{\"status\":\"success\",\"message\":\"rotation set\"}");
    } else if (c == "FORMAT_FS") {
        protocolReply(formatFs() ? "{\"status\":\"success\",\"message\":\"formatted\"}"
                                 : "{\"status\":\"error\",\"message\":\"format failed\"}");
    } else if (c == "LIST_GIFS") {
        {
            String reply = "{\"status\":\"success\",\"gifs\":" + listGifs() + "}";
            protocolReply(reply);
        }
    } else if (c == "PLAY_GIF") {
        const char* file = doc["file"];
        String p = file ? String(file) : "";
        if (!p.startsWith("/")) p = "/" + p;
        if (openGif(p.c_str())) {
            Serial.printf("[sandbox] playing %s rot=%u baud=%d\n", p.c_str(), tftRotation, SANDBOX_SERIAL_BAUD);
            protocolReply("{\"status\":\"success\",\"message\":\"playing\"}");
        } else {
            protocolReply("{\"status\":\"error\",\"message\":\"play failed\"}");
        }
    } else if (c == "START_UPLOAD") {
        if (gifPlaying) {
            gif.close();
            gifPlaying = false;
            if (gifFile) gifFile.close();
            tft.endWrite();
        }
        const char* file = doc["file"];
        uploadTotal = doc["size"] | 0;
        if (!file || !uploadTotal) {
            protocolReply("{\"status\":\"error\",\"message\":\"bad upload args\"}");
            return;
        }
        uploadPath = file;
        if (!uploadPath.startsWith("/")) uploadPath = "/" + uploadPath;
        uploadFile = LittleFS.open(uploadPath.c_str(), "w");
        if (!uploadFile) {
            protocolReply("{\"status\":\"error\",\"message\":\"open failed\"}");
            return;
        }
        uploadReceived = 0;
        uploadActive = true;
        protocolReply("{\"status\":\"success\",\"message\":\"ready for chunks\"}");
    } else if (c == "CHUNK") {
        if (!uploadActive) {
            protocolReply("{\"status\":\"error\",\"message\":\"not uploading\"}");
            return;
        }
        chunkExpected = doc["size"] | 0;
        chunkReceived = 0;
        binaryMode = true;
        protocolReply("{\"status\":\"success\",\"message\":\"send chunk\"}");
    } else if (c == "END_UPLOAD") {
        uploadFile.close();
        uploadActive = false;
        binaryMode = false;
        if (uploadReceived != uploadTotal) {
            LittleFS.remove(uploadPath.c_str());
            protocolReply("{\"status\":\"error\",\"message\":\"size mismatch\"}");
        } else {
            protocolReply("{\"status\":\"success\",\"message\":\"upload complete\"}");
        }
    } else {
        protocolReply("{\"status\":\"error\",\"message\":\"unknown cmd\"}");
    }
}

void setup() {
    Serial.setRxBufferSize(SERIAL_RX_BUF);
    Serial.begin(SANDBOX_SERIAL_BAUD);
    delay(200);

    runTftBaseline();

    Serial.printf("\n[sandbox] baud=%d rot=%u\n", SANDBOX_SERIAL_BAUD, tftRotation);
    Serial.println("[sandbox] TFT MOSI=25 SCK=14 DC=27 RST=12 BL=13 CS=none");

    if (!LittleFS.begin(true)) {
        Serial.println("[sandbox] LittleFS failed");
    } else {
        Serial.println("[sandbox] LittleFS ok");
    }
    Serial.println("[sandbox] ready");
}

void loop() {
    if (!uploadActive) {
        updateGif();
    }

    while (Serial.available()) {
        if (binaryMode && uploadActive) {
            uint8_t buf[256];
            size_t avail = Serial.available();
            size_t want = chunkExpected - chunkReceived;
            size_t n = Serial.readBytes(buf, min(min(avail, sizeof(buf)), want));
            if (n) {
                uploadFile.write(buf, n);
                uploadReceived += n;
                chunkReceived += n;
            }
            if (chunkReceived >= chunkExpected) {
                binaryMode = false;
                protocolReply("{\"status\":\"success\",\"message\":\"chunk received\"}");
            }
            continue;
        }

        char ch = Serial.read();
        if (ch == '\n' || ch == '\r') {
            if (jsonLine.length() > 0) {
                handleCommand(jsonLine);
                jsonLine = "";
            }
        } else if (jsonLine.length() < 512) {
            jsonLine += ch;
        } else {
            jsonLine = "";
            protocolReply("{\"status\":\"error\",\"message\":\"line too long\"}");
        }
    }
}

static void GIFDraw(GIFDRAW* pDraw) {
    static uint16_t lineBuf[240];
    int y = pDraw->iY + pDraw->y + gifYOff;
    int x = pDraw->iX + gifXOff;
    int w = pDraw->iWidth;
    if (y < 0 || y >= tft.height() || x >= tft.width() || w < 1) return;
    if (x + w > tft.width()) w = tft.width() - x;

    uint8_t* s = pDraw->pPixels;
    uint16_t* pal = pDraw->pPalette;

    if (pDraw->ucHasTransparency) {
        uint8_t* end = s + w;
        int runX = 0;
        while (s < end) {
            uint8_t c = pDraw->ucTransparent - 1;
            uint16_t* d = lineBuf;
            int n = 0;
            while (c != pDraw->ucTransparent && s < end && n < 240) {
                c = *s++;
                if (c == pDraw->ucTransparent) s--;
                else { *d++ = pal[c]; n++; }
            }
            if (n) {
                tft.setAddrWindow(x + runX, y, n, 1);
                tft.pushPixels(lineBuf, n);
                runX += n;
            }
            c = pDraw->ucTransparent;
            while (c == pDraw->ucTransparent && s < end) {
                c = *s++;
                if (c == pDraw->ucTransparent) runX++;
                else s--;
            }
        }
    } else {
        for (int i = 0; i < w; i++) lineBuf[i] = pal[s[i]];
        tft.setAddrWindow(x, y, w, 1);
        tft.pushPixels(lineBuf, w);
    }
}

static void* gifOpen(const char* fname, int32_t* pSize) {
    gifFile = LittleFS.open(fname, "r");
    if (!gifFile) return nullptr;
    *pSize = gifFile.size();
    return &gifFile;
}

static void gifClose(void* pHandle) {
    File* f = static_cast<File*>(pHandle);
    if (f && *f) f->close();
}

static int32_t gifRead(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen) {
    File* f = static_cast<File*>(pFile->fHandle);
    if (!f || !*f) return 0;
    int32_t n = iLen;
    if ((pFile->iSize - pFile->iPos) < iLen) n = pFile->iSize - pFile->iPos - 1;
    if (n <= 0) return 0;
    int32_t r = f->read(pBuf, n);
    pFile->iPos = f->position();
    return r;
}

static int32_t gifSeek(GIFFILE* pFile, int32_t iPosition) {
    File* f = static_cast<File*>(pFile->fHandle);
    if (!f || !*f) return 0;
    f->seek(iPosition);
    pFile->iPos = f->position();
    return pFile->iPos;
}
