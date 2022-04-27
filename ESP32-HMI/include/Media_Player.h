//这是一个媒体播放应用
//支持的格式有：.mjpeg / .jpg / .mp3 / .pcm / .txt

String User = "Null";
String Userdir;
String filedir;
String filedir2;
String bookdir;
String password = "9961";
String MusicName;
String MusicState = "List Loop";
String VideoName;
String BookName;
int MS_num;
int Musicnum = 1;
int Music_num = 1;
int MusicPage = 1;
int MusicBline;
int MusicEline;
int EnableMC = 0; //MusicCheck
int pos;
int a = 1;
int VideoPage = 1;
int VideoBline;
int VideoEline;
int Videonum = 1;
int EnableVC = 0;
int IfVision = 1;
int BookPage = 1;
int EnableBC = 0;
int BookBline;
int BookEline;

bool notice;
String n_otice;

//Mjpeg player:
#include "MjpegClass.h"
#include <Arduino_GFX_Library.h>
#include <SD_MMC.h>
#include <driver/i2s.h>

Arduino_DataBus *bus = new Arduino_ESP32SPI(27, 5, 18, 23, -1);
Arduino_ILI9488_18bit *gfx = new Arduino_ILI9488_18bit(bus, 33, 3);

#define TFT_BRIGHTNESS 255
#define MJPEG_BUFFER_SIZE (480 * 320 * 2 / 14)
#define FPS 30
#define READ_BUFFER_SIZE 2048
#define TFT_BL 19

static MjpegClass mjpeg;
int next_frame = 0;
int skipped_frames = 0;
unsigned long total_read_audio = 0;
unsigned long total_play_audio = 0;
unsigned long total_read_video = 0;
unsigned long total_play_video = 0;
unsigned long total_remain = 0;
unsigned long start_ms, curr_ms, next_frame_ms;
static unsigned long total_show_video = 0;
bool Media_play = true;

static int drawMCU(JPEGDRAW *pDraw) {
  unsigned long s = millis();
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  total_show_video += millis() - s;
  return 1;
}
void Mjpeg_start(const char *MJPEG_FILENAME, const char *AUDIO_FILENAME) {
  gfx->fillScreen(TFT_BLACK);
  gfx->setCursor(0,0);
  gfx->setTextColor(TFT_BLACK);
  // Init Audio
  i2s_config_t i2s_config_dac = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_PCM | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // lowest interrupt priority
    // .dma_buf_count = 5,
    .dma_buf_count = 8,
    .dma_buf_len = 490,
    .use_apll = false,
  };
  Serial.printf("%p\n", &i2s_config_dac);
  if (i2s_driver_install(I2S_NUM_0, &i2s_config_dac, 0, NULL) != ESP_OK)
  {
    Serial.println(F("ERROR: Unable to install I2S drives!"));
    gfx->println(F("ERROR: Unable to install I2S drives!"));
  }
  else {
    i2s_set_pin((i2s_port_t)0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
    i2s_zero_dma_buffer((i2s_port_t)0);

    File aFile = SD_MMC.open(AUDIO_FILENAME);
    if (!aFile || aFile.isDirectory()) {
      Serial.println(F("ERROR: Failed to open file for reading!"));
      gfx->println(F("ERROR: Failed to open file for reading!"));
    }
    else {
      File vFile = SD_MMC.open(MJPEG_FILENAME);
      if (!vFile || vFile.isDirectory()) {
        Serial.println(F("ERROR: Failed to open file for reading"));
        Serial.println(AUDIO_FILENAME);
        Serial.println(MJPEG_FILENAME);
        gfx->println(F("ERROR: Failed to open file for reading"));
      }
      else {
        uint8_t *aBuf = (uint8_t *)malloc(2940);
        // uint8_t *aBuf = (uint8_t *)malloc(5880);
        if (!aBuf) {
          Serial.println(F("aBuf malloc failed!"));
        }
        else {
          uint8_t *mjpeg_buf = (uint8_t *)malloc(MJPEG_BUFFER_SIZE);
          if (!mjpeg_buf) {
            Serial.println(F("mjpeg_buf malloc failed!"));
          }
          else {
            Serial.println(F("PCM audio MJPEG video start"));
            start_ms = millis();
            curr_ms = millis();
            mjpeg.setup(&vFile, mjpeg_buf, drawMCU, true, true);
            next_frame_ms = start_ms + (++next_frame * 1000 / FPS);

            // prefetch first audio buffer
            aFile.read(aBuf, 980);
            i2s_write_bytes((i2s_port_t)0, (char *)aBuf, 980, 0);

            while (vFile.available() && aFile.available()) {
              /*if (IfVision == 1){
                if (touch.Pressed()){
                  if (touch.X() > 400){
                    break;
                  }
                }
              }*/
              // Read audio
              aFile.read(aBuf, 2940);
              // aFile.read(aBuf, 5880);
              total_read_audio += millis() - curr_ms;
              curr_ms = millis();

              // Play audio
              i2s_write_bytes((i2s_port_t)0, (char *)aBuf, 980, 0);
              i2s_write_bytes((i2s_port_t)0, (char *)(aBuf + 980), 980, 0);
              i2s_write_bytes((i2s_port_t)0, (char *)(aBuf + 1960), 980, 0);

              total_play_audio += millis() - curr_ms;
              curr_ms = millis();

              // Read video
              mjpeg.readMjpegBuf();
              total_read_video += millis() - curr_ms;
              curr_ms = millis();
              // check show frame or skip frame
              if (millis() < next_frame_ms) {
                // Play video
                mjpeg.drawJpg();
                total_play_video += millis() - curr_ms;

                int remain_ms = next_frame_ms - millis();
                total_remain += remain_ms;
                if (remain_ms > 0) {
                  //                  delay(remain_ms);
                }
              }
              else {
                ++skipped_frames;
                Serial.println(F("Skip frame"));
              }

              curr_ms = millis();
              next_frame_ms = start_ms + (++next_frame * 1000 / FPS);
            }
            int time_used = millis() - start_ms;
            Serial.println(F("PCM audio MJPEG video end"));
            vFile.close();
            aFile.close();
            // avoid unexpected output at audio pins
            i2s_driver_uninstall((i2s_port_t)0); //stop & destroy i2s driver
            gfx->fillScreen(TFT_BLACK);
            int played_frames = next_frame - 1 - skipped_frames;
            float fps = 1000.0 * played_frames / time_used;
#define CHART_MARGIN 24
#define LEGEND_A_COLOR 0xE0C3
#define LEGEND_B_COLOR 0x33F7
#define LEGEND_C_COLOR 0x4D69
#define LEGEND_D_COLOR 0x9A74
#define LEGEND_E_COLOR 0xFBE0
#define LEGEND_F_COLOR 0xFFE6
#define LEGEND_G_COLOR 0xA2A5
            gfx->setCursor(0, 0);
            gfx->setTextColor(WHITE);
            gfx->printf("Played frames: %d\n", played_frames);
            gfx->printf("Skipped frames: %d (%0.1f %%)\n", skipped_frames, 100.0 * skipped_frames / played_frames);
            gfx->printf("Actual FPS: %0.1f\n\n", fps);
            int16_t r1 = ((gfx->height() - CHART_MARGIN - CHART_MARGIN) / 2);
            int16_t r2 = r1 / 2;
            int16_t cx = gfx->width() - gfx->height() + CHART_MARGIN + CHART_MARGIN - 1 + r1;
            int16_t cy = r1 + CHART_MARGIN;
            float arc_start = 0;
            float arc_end = max(2.0, 360.0 * total_read_audio / time_used);
            for (int i = arc_start + 1; i < arc_end; i += 2) {
              gfx->fillArc(cx, cy, r1, r2, arc_start - 90.0, i - 90.0, LEGEND_D_COLOR);
            }
            gfx->fillArc(cx, cy, r1, r2, arc_start - 90.0, arc_end - 90.0, LEGEND_D_COLOR);
            gfx->setTextColor(LEGEND_D_COLOR);
            gfx->printf("Read PCM:\n%0.1f %%\n", 100.0 * total_read_audio / time_used);

            arc_start = arc_end;
            arc_end += max(2.0, 360.0 * total_play_audio / time_used);
            for (int i = arc_start + 1; i < arc_end; i += 2) {
              gfx->fillArc(cx, cy, r1, r2, arc_start - 90.0, i - 90.0, LEGEND_C_COLOR);
            }
            gfx->fillArc(cx, cy, r1, r2, arc_start - 90.0, arc_end - 90.0, LEGEND_C_COLOR);
            gfx->setTextColor(LEGEND_C_COLOR);
            gfx->printf("Play audio:\n%0.1f %%\n", 100.0 * total_play_audio / time_used);

            arc_start = arc_end;
            arc_end += max(2.0, 360.0 * total_read_video / time_used);
            for (int i = arc_start + 1; i < arc_end; i += 2) {
              gfx->fillArc(cx, cy, r1, r2, arc_start - 90.0, i - 90.0, LEGEND_B_COLOR);
            }
            gfx->fillArc(cx, cy, r1, r2, arc_start - 90.0, arc_end - 90.0, LEGEND_B_COLOR);
            gfx->setTextColor(LEGEND_B_COLOR);
            gfx->printf("Read MJPEG:\n%0.1f %%\n", 100.0 * total_read_video / time_used);
            arc_start = arc_end;
            arc_end += max(2.0, 360.0 * total_play_video / time_used);
            for (int i = arc_start + 1; i < arc_end; i += 2) {
              gfx->fillArc(cx, cy, r1, 0, arc_start - 90.0, i - 90.0, LEGEND_A_COLOR);
            }
            gfx->fillArc(cx, cy, r1, 0, arc_start - 90.0, arc_end - 90.0, LEGEND_A_COLOR);
            gfx->setTextColor(LEGEND_A_COLOR);
            gfx->printf("Play video:\n%0.1f %%\n", 100.0 * total_play_video / time_used);
            next_frame = 0;
            skipped_frames = 0;
            total_read_audio = 0;
            total_play_audio = 0;
            total_read_video = 0;
            total_play_video = 0;
            total_remain = 0;
            total_show_video = 0;
            EnableVC = 1;
            delay(2000);
            gfx->fillScreen(TFT_BLACK);
          }
        }
      }
    }
  }
}

//jpeg Player:
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>

TFT_eSPI tft = TFT_eSPI();

int jpgnum = 0;
String jpgdir;

void drawSdJpeg(const char *filename, int xpos, int ypos);
void jpegRender(int xpos, int ypos);
void jpegInfo();
void showTime(uint32_t msTime);
void SD_read_Time(uint32_t msTime);

void drawSdJpeg(const char *filename, int xpos, int ypos) {
  uint32_t readTime = millis();
  File jpegFile = SD_MMC.open(filename, FILE_READ);

  if ( !jpegFile ) {
    Serial.print("ERROR: File \"");
    Serial.print(filename);
    Serial.println("\" not found!");
    return;
  }
  boolean decoded = JpegDec.decodeSdFile(jpegFile);
  SD_read_Time(millis() - readTime);

  if (decoded) {
    jpegInfo();
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}
void jpegRender(int xpos, int ypos) {
  uint32_t drawTime = millis();
  uint16_t *pImg;
  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;
  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  bool swapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);
  uint32_t min_w = (mcu_w < (max_x % mcu_w) ? mcu_w : (max_x % mcu_w));
  uint32_t min_h = (mcu_h < (max_y % mcu_h) ? mcu_h : (max_y % mcu_h));
  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;
  max_x += xpos;
  max_y += ypos;
  while (JpegDec.read()) {
    pImg = JpegDec.pImage;
    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;
    if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
    else win_w = min_w;
    if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
    else win_h = min_h;
    if (win_w != mcu_w) {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;
      for (int h = 1; h < win_h; h++) {
        p += mcu_w;
        for (int w = 0; w < win_w; w++) {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }
    //    uint32_t mcu_pixels = win_w * win_h;
    if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    else if ( (mcu_y + win_h) >= tft.height())
      JpegDec.abort();
  }
  tft.setSwapBytes(swapBytes);
  showTime(millis() - drawTime);
}
void jpegInfo() {
  Serial.println("JPEG image info");
  Serial.println("===============");
  Serial.print("Width      :");
  Serial.println(JpegDec.width);
  Serial.print("Height     :");
  Serial.println(JpegDec.height);
  Serial.print("Components :");
  Serial.println(JpegDec.comps);
  Serial.print("MCU / row  :");
  Serial.println(JpegDec.MCUSPerRow);
  Serial.print("MCU / col  :");
  Serial.println(JpegDec.MCUSPerCol);
  Serial.print("Scan type  :");
  Serial.println(JpegDec.scanType);
  Serial.print("MCU width  :");
  Serial.println(JpegDec.MCUWidth);
  Serial.print("MCU height :");
  Serial.println(JpegDec.MCUHeight);
  Serial.print("===============");
  Serial.println("");
}
void showTime(uint32_t msTime) {
  Serial.print(F(" JPEG drawn in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}
void SD_read_Time(uint32_t msTime) {
  Serial.print(F(" SD JPEG read in "));
  Serial.print(msTime);
  Serial.println(F(" ms "));
}

//Game-TH-Mini
#include <TFT_Touch.h>

#define DOUT 32  // Data out pin    (T_DO)  of touch screen
#define DIN  27  // Data in pin     (T_DIN) of touch screen
#define DCS  16  // Chip select pin (T_CS)  of touch screen
#define DCLK 0   // Clock pin       (T_CLK) of touch screen
TFT_Touch touch = TFT_Touch(DCS, DCLK, DIN, DOUT);

int y = 0;
int spd = 25;
int obj = 1;
int score = 0;
int state = 0;

void THMini() {
  tft.setRotation(0);
  touch.setRotation(2);
  tft.setTextSize(2);
  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(0, 0);
  tft.setTextDatum(MC_DATUM);
  while (1) {
    if (state == 0) {
      tft.setCursor(5, 5);
      tft.setTextColor(TFT_BLUE);
      Userdir = String ("/User/") + User + String("/Data/Game/TouHou/Mini/image/start.jpg");
      drawSdJpeg(Userdir.c_str(), 0, 0);
      state = 1;
    }
    if (touch.Pressed()) {
      Userdir = String ("/User/") + User + String("/Data/Game/TouHou/Mini/image/background.jpg");
      drawSdJpeg(Userdir.c_str(), 0, 0);
      tft.print("score:");
      tft.setCursor(5, 25);
      tft.print(score);
      while (0 == 0) {
        if (touch.Pressed()) {
          //tft.fillRect(127,y - spd,64,64,TFT_WHITE);
          Userdir = String ("/User/") + User + String("/Data/Game/TouHou/Mini/image/background.jpg");
          drawSdJpeg(Userdir.c_str(), 0, 0);
          tft.fillRect(40, 0, 280, 30, TFT_WHITE);
          if (y < 100) {
            score = score + 50;
          }
          else if (y < 250) {
            score = score + 100;
          }
          else if (y < 400) {
            score = score + 200;
          }
          tft.setCursor(5, 5);
          tft.print("score:");
          tft.setCursor(5, 25);
          tft.print(score);
          spd = random(30, 40);
          obj = random(1, 3);
          y = 0;
        }
        else {
          tft.fillRect(127, y - spd, 64, 64, TFT_WHITE);
          if (obj == 1) {
            Userdir = String ("/User/") + User + String("/Data/Game/TouHou/Mini/image/object1.jpg");
            drawSdJpeg(Userdir.c_str(), 127, y);
          }
          else if (obj == 2) {
            Userdir = String ("/User/") + User + String("/Data/Game/TouHou/Mini/image/object2.jpg");
            drawSdJpeg(Userdir.c_str(), 127, y);
          }
          y = y + spd;

          if (y > 400) {
            y = 0;
            score = 0;
            state = 0;
            spd = random(30, 40);
            obj = random(1, 3);
            break;
          }
          else {
            delay(30);
          }
        }
      }
    }
    else {
      delay(30);
    }
  }
}

//Draw
int ColorPaletteHigh = 30; // Height of palette boxes
int color = TFT_WHITE;     //Starting paint brush color
int X_Coord;
int Y_Coord;
unsigned int colors[10] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_BLACK, TFT_CYAN, TFT_YELLOW, TFT_WHITE, TFT_MAGENTA, TFT_BLACK, TFT_BLACK};

void draw() {
  tft.setRotation(1);
  touch.setRotation(3);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  for (int i = 0; i < 10; i++)
  {
    tft.fillRect(i * 32, 0, 32, ColorPaletteHigh, colors[i]);
  }
  tft.setCursor(264, 7, 2);
  tft.setTextColor(TFT_WHITE);
  tft.print("Clear");
  tft.drawRect(435,0,45,ColorPaletteHigh,TFT_WHITE);
  tft.setCursor(445, 7);
  tft.print("Quit");
  tft.drawRect(0, 0, 319, 30, TFT_WHITE);
  tft.fillRect(300, 9, 12, 12, color);

  while (1) {
    if (touch.Pressed()) {
      X_Coord = touch.X();
      Y_Coord = touch.Y();
      if (Y_Coord < ColorPaletteHigh + 2) {
        if ((X_Coord / 32 > 7) && (X_Coord < 320)) {
          // Clear the screen to current colour
          tft.fillRect(0, 30, /*399*/ 480 , /*239*/ 320, color);
        }
        else if ((X_Coord > 435) && (Y_Coord < 25)){
          break;
        }
        else {
          color = colors[X_Coord / 32];
          tft.fillRect(300, 9, 12, 12, color);
        }
      }
      else {
        tft.fillCircle(X_Coord, Y_Coord, 2, color);
      }
    }
  }
}

//mp3 Player:
#include <AudioFileSourceFS.h>
#include <AudioFileSourceID3.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

AudioGeneratorMP3 *mp3;
AudioFileSourceFS *file;
AudioOutputI2S *out;

int fileline = 1;

void MP3_start(const char *filename) {
  file = new AudioFileSourceFS(SD_MMC, filename);
  out = new AudioOutputI2S(0, 1, 128);
  mp3 = new AudioGeneratorMP3();
  mp3->begin(file, out);
  while (0 == 0) {
    if (mp3->isRunning()) {
      if (!mp3->loop()) {
        fileline = 1;
        mp3->stop();
      }
    }
    else {
      i2s_driver_uninstall((i2s_port_t)0);
      fileline = 1;
      Serial.printf("MP3 done\n");
      break;
    }
  }
}

//PCM Player:
void PCM_start(const char *AUDIOFILENAME) {
  i2s_config_t i2s_config_dac = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_PCM | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // lowest interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = 490,
    .use_apll = false,
  };
  Serial.printf("%p\n", &i2s_config_dac);
  if (i2s_driver_install(I2S_NUM_0, &i2s_config_dac, 0, NULL) != ESP_OK) {
    Serial.println(F("ERROR: Unable to install I2S drives!"));
  }
  else {
    i2s_set_pin((i2s_port_t)0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
    i2s_zero_dma_buffer((i2s_port_t)0);
    File aFile = SD_MMC.open(AUDIOFILENAME);
    if (!aFile || aFile.isDirectory()) {
      Serial.println(F("ERROR: Failed to open  file for reading!"));
    }
    else {
      uint8_t *aBuf = (uint8_t *)malloc(2940);
      if (!aBuf) {
        Serial.println(F("aBuf malloc failed!"));
      }
      Serial.println(F("PCM audio start"));
      aFile.read(aBuf, 980);
      i2s_write_bytes((i2s_port_t)0, (char *)aBuf, 980, 0);
      while (aFile.available()) {
        aFile.read(aBuf, 2940);
        i2s_write_bytes((i2s_port_t)0, (char *)aBuf, 980, 0);
        i2s_write_bytes((i2s_port_t)0, (char *)(aBuf + 980), 980, 0);
        i2s_write_bytes((i2s_port_t)0, (char *)(aBuf + 1960), 980, 0);
      }
      Serial.println(F("PCM audio end"));
      aFile.close();
      i2s_driver_uninstall((i2s_port_t)0);
    }
  }
}

//txt Player:
char buf[128];
unsigned long line = 1;
int tftline = 1;

String readFileLine(const char* path, int num) {
  Serial.printf("Reading file: %s line: %d\n", path, num);
  File file = SD_MMC.open(path);
  if (!file) {
    return ("Failed to open file for reading");
  }
  char* p = buf;
  while (file.available()) {
    char c = file.read();
    if (c == '\n') {
      num--;
      if (num == 0) {
        *(p++) = '\0';
        String s(buf);
        s.trim();
        return s;
      }
    }
    else if (num == 1) {
      *(p++) = c;
    }
  }
  file.close();
  return String("error");
}

//listDir
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.print(file.size() / 1024);
      Serial.println("KB");
      tft.print("  FILE: ");
      tft.print(file.name());
      tft.print("  SIZE: ");
      tft.print(file.size() / 1024);
      tft.println("KB");
    }
    file = root.openNextFile();
  }
}
String filesname;
void appendDir(fs::FS &fs, const char * dirname, String filename, uint8_t levels) {
  File root = fs.open(filename.c_str());
  Serial.println(filename);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else {
      Serial.println(file.name());
      File Configfile = SD_MMC.open(Userdir.c_str(), FILE_APPEND);
      Configfile.println(file.name());
      Configfile.close();
    }
    file = root.openNextFile();
  }
}

//Ble-internal DAC
#include <BluetoothA2DPSink.h>

BluetoothA2DPSink a2dp_sink;

void BleAudio(){
  const i2s_config_t i2s_config = {
      .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
      .sample_rate = 44100,
      .bits_per_sample = (i2s_bits_per_sample_t) 16,
      .channel_format =  I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB,
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = false
  };
  a2dp_sink.set_i2s_config(i2s_config);  
  a2dp_sink.start("ESP-HMI");
}

//Scrolling Words
TFT_eSprite img = TFT_eSprite(&tft);

void build_banner(int IWIDTH, int IHEIGHT,String msg, int xpos,int size,int font, uint16_t TextColor) {
  // We could just use fillSprite(color) but lets be a bit more creative...
  // Now print text on top of the graphics
  img.setTextSize(size);           // Font size scaling is x1
  img.setTextFont(font);           // Font 4 selected
  img.setTextColor(TextColor);  // Black text, no background colour
  img.setTextWrap(false);       // Turn of wrap so we can print past end of sprite
  // Need to print twice so text appears to wrap around at left and right edges
  img.setCursor(xpos, 2);  // Print text at xpos
  img.print(msg);
  img.setCursor(xpos - IWIDTH, 2); // Print text at xpos - sprite width
  img.print(msg);
}
void CreatCrollWords(int IWIDTH, int IHEIGHT, int WAIT, int X, int Y,int size,int font, uint16_t TextColor, String msg){
  //Serial.println("CreatCrollWords");
  // Create the sprite and clear background to black
  while(a == 1){
    img.createSprite(IWIDTH, IHEIGHT);
    //Serial.println("This will be exed once.");
    a = 0;
  }
  //Serial.println(pos);
  pos--;
  img.fillSprite(tft.color565(0, 128, 255));
  build_banner(IWIDTH, IHEIGHT, msg, pos, size, font, TextColor);
  img.pushSprite(X, Y);
  delay(WAIT);
  if (pos < 0) {
    img.deleteSprite();
    pos = IWIDTH;
    a = 1;
  }
}/*
void CreatCrollWords(int IWIDTH,int pos, int IHEIGHT, int WAIT, int X, int Y,int size,int font, uint16_t TextColor, String msg){
  while (1) {
    // Create the sprite and clear background to black
    img.createSprite(IWIDTH, IHEIGHT);
    for (int pos = IWIDTH; pos > 0; pos--) {
      img.fillSprite(tft.color565(0, 128, 255));
      build_banner(IWIDTH,IHEIGHT,msg, pos, size, font, TextColor);
      img.pushSprite(X, Y);
      delay(WAIT);
    }
    // Delete sprite to free up the memory
    img.deleteSprite();
  }
}*/