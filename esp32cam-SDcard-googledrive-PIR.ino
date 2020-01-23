#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"

#include "esp_camera.h"

#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory
#define EEPROM_SIZE 1

const char* ssid     = "SSID";   //your network SSID
const char* password = "PASSWORD";   //your network password
const char* myDomain = "script.google.com";
String myScript = "/macros/s/xxxxxxxxxxxxxxxxxxxxxxxxx/exec";    //Replace with your own url
String myFilename = "filename=ESP32-CAM.jpg";
String mimeType = "&mimetype=image/jpeg";
String myImage = "&data=";

int waitingTime = 30000; //Wait 30 seconds to google response.

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

int pictureNumber = 0;
int WIFI_flag = 0;
unsigned long previous_time;

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(10);

  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  //  while (WiFi.status() != WL_CONNECTED) {
  //    Serial.print(".");
  //    delay(500);
  //  }
  delay(10000);
  if (WiFi.status() == WL_CONNECTED) {
    WIFI_flag = 1;
  }
  else {
    WIFI_flag = 0;
  }

  Serial.println("");
  Serial.println("STAIP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }
  previous_time = millis();
}

boolean enviar = true;

void loop() {
  unsigned long current_time = millis();
  unsigned long time_passed;

  if (WIFI_flag == 0) {
    Serial.println("NO WIFI");
    time_passed = current_time - previous_time;
    if (time_passed > 10000) {
      Serial.println("REconnecting WIFI");
      WiFi.begin(ssid, password);
      delay(10000);
      if (WiFi.status() != WL_CONNECTED) {
        WIFI_flag = 0;
      }
      else {
        WIFI_flag = 1;
      }
    }
    previous_time = current_time;
  }

  if (digitalRead(13) == 1) {
    saveCapturedImage();
    enviar = false;
    //delay(60000);
    //Serial.println("PIR OK");
    delay(10000);
  }
  else {
    Serial.println("OFF");
  }
  //}
}

void saveCapturedImage() {
  Serial.println("Connect to " + String(myDomain));
  WiFiClientSecure client;

  //Serial.println("Starting SD Card");
  if (!SD_MMC.begin()) {
    Serial.println("SD Card Mount Failed");
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD Card attached");
    return;
  }

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return;
  }
  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;

  // Path where new picture will be saved in SD Card
  String path = "/picture" + String(pictureNumber) + ".jpg";

  fs::FS &fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();
  }
  file.close();

//  pinMode(4, OUTPUT);
//  digitalWrite(4, LOW);
//  rtc_gpio_hold_en(GPIO_NUM_4);

  //if there is connection
  if (client.connect(myDomain, 443)) {
    Serial.println("Connection successful");

    //camera_fb_t * fb = NULL;
    //fb = esp_camera_fb_get();
    //if(!fb) {
    //  Serial.println("Camera capture failed");
    //  delay(1000);
    //  ESP.restart();
    //  return;
    //}

    char *input = (char *)fb->buf;
    char output[base64_enc_len(3)];
    String imageFile = "";
    for (int i = 0; i < fb->len; i++) {
      base64_encode(output, (input++), 3);
      if (i % 3 == 0) imageFile += urlencode(String(output));
    }
    String Data = myFilename + mimeType + myImage;

    esp_camera_fb_return(fb);

    Serial.println("Send a captured image to Google Drive.");

    client.println("POST " + myScript + " HTTP/1.1");
    client.println("Host: " + String(myDomain));
    client.println("Content-Length: " + String(Data.length() + imageFile.length()));
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println();

    client.print(Data);
    int Index;
    for (Index = 0; Index < imageFile.length(); Index = Index + 1000) {
      client.print(imageFile.substring(Index, Index + 1000));
    }

    Serial.println("Waiting for response.");
    long int StartTime = millis();
    while (!client.available()) {
      Serial.print(".");
      delay(100);
      if ((StartTime + waitingTime) < millis()) {
        Serial.println();
        Serial.println("No response.");
        //If you have no response, maybe need a greater value of waitingTime
        break;
      }
    }
    Serial.println();
    while (client.available()) {
      Serial.print(char(client.read()));
    }
  } else {
    Serial.println("Connected to " + String(myDomain) + " failed.");
    esp_camera_fb_return(fb);
  }
  client.stop();
}

//https://github.com/zenmanenergy/ESP8266-Arduino-Examples/
String urlencode(String str)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
      //encodedString+=code2;
    }
    yield();
  }
  return encodedString;
}
