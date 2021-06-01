
#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"
#include "SD_MMC.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h" 
#include "driver/rtc_io.h"
#include "WiFi.h"
#include "system.hpp"
#include "time.h"
#include <arduino-timer.h>
#include <ArduinoJson.h>
StaticJsonDocument<256> g_jsonFile;

bool g_bContPicEnableFlag = false;
int g_nContPicIntervel = NORMAL_PHOTO_TIME_WIDTH;
int g_nCurrWifiStatus = WL_DISCONNECTED;
bool g_bCamerConfigStatus = false;
bool g_TakeSnapShotFlagNormal = false;
bool g_TakeSnapShotFlagBrust = false;
ustcConfig  u_ustcConfig;
camera_config_t config;
auto e_TimerNormal = timer_create_default();
auto e_TimerBrust = timer_create_default();
char e_arr_cSSID[LENGTH_OF_SSID_PASS] = {0};
char e_arr_cPassKey[LENGTH_OF_SSID_PASS] = {0};

#define DEBOUNCE_TIME 2000 // Filtre anti-rebond (debouncer)
volatile uint32_t g_u_n32DebounceTimer = 0;
bool g_bPirDetected = false;

bool configCamera(void)
{
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


  if(psramFound())//check availablity of PSRM
  {
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } 
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) 
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return false;
  }
  return true;
}
bool initSDcard(void)
{
  if(!SD_MMC.begin())
  {
    return false;
  }
  return true;
}
bool checkSDCardStatus(void)
{
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE)
  {
    return false;
  }
  return true;
}
bool SnapPhoto(void)
{
  bool bRet = false;
  unsigned int nPicNo = 0;
  String SFileName;
  String sFileConfig = CONFIG_FILE_NAME;
  camera_fb_t *ptrCamera = NULL;
  fs::FS &fs = SD_MMC; 
  if(true != g_bCamerConfigStatus)
  {
    Serial.println("Capture error - Camera not initialized");
    return bRet;
  }    
  ptrCamera = esp_camera_fb_get();
  if(!ptrCamera)
  {
    Serial.println("Camera capture failed");
    return bRet;
  }
  if(WL_CONNECTED == g_nCurrWifiStatus)
  {
    if(false == FetchTimeFromNTP(u_ustcConfig.stcConfig.arr_cDate,u_ustcConfig.stcConfig.arr_cTime)
                    || 0 == u_ustcConfig.stcConfig.arr_cDate[0] || 0 == u_ustcConfig.stcConfig.arr_cTime[0])
    {
      strncpy(u_ustcConfig.stcConfig.arr_cDate,DEFAULT_DATE,sizeof(u_ustcConfig.stcConfig.arr_cDate));
      strncpy(u_ustcConfig.stcConfig.arr_cTime,DEFAULT_TIME,sizeof(u_ustcConfig.stcConfig.arr_cTime));
    }
  }
  else
  {
      strncpy(u_ustcConfig.stcConfig.arr_cDate,DEFAULT_DATE,sizeof(u_ustcConfig.stcConfig.arr_cDate));
      strncpy(u_ustcConfig.stcConfig.arr_cTime,DEFAULT_TIME,sizeof(u_ustcConfig.stcConfig.arr_cTime));
  }

  SFileName = "/P" + String(u_ustcConfig.stcConfig.arr_cDate) + "_" +
                String(u_ustcConfig.stcConfig.arr_cTime) + "_" +String(u_ustcConfig.stcConfig.u_nNumOfPics) +".jpg";
  File fFile = fs.open(SFileName.c_str(), FILE_WRITE);
  if(fFile)
  {
    fFile.write(ptrCamera->buf, ptrCamera->len);
    fFile.close();
    Serial.printf("Saved file to path: %s\n", SFileName.c_str());
    bRet = true;
    File fFileConfig = fs.open(sFileConfig.c_str(),  FILE_READ);
    if(fFileConfig)
    {
      fFileConfig.read((uint8_t*)u_ustcConfig.arr_cConfig,sizeof(u_ustcConfig.arr_cConfig));
      fFileConfig.close();
      File fFileConfig = fs.open(sFileConfig.c_str(),FILE_WRITE);
      u_ustcConfig.stcConfig.u_nNumOfPics++;
      fFileConfig.write((uint8_t*)u_ustcConfig.arr_cConfig,sizeof(u_ustcConfig.arr_cConfig));
      fFileConfig.close();
      Serial.printf("Num of photos: %d\n",u_ustcConfig.stcConfig.u_nNumOfPics);
    }
  }
  else
  {
    bRet = false;
  }
  esp_camera_fb_return(ptrCamera); 
  delay(CAMERA_DEFAULT_DELAY);
  return bRet;
}
void InitializeBuffers(void)
{
    fs::FS &fs = SD_MMC; 
    String sFileConfig = CONFIG_FILE_NAME;

    memset(u_ustcConfig.stcConfig.arr_cDate,0,sizeof(u_ustcConfig.stcConfig.arr_cDate));
    memset(u_ustcConfig.stcConfig.arr_cTime,0,sizeof(u_ustcConfig.stcConfig.arr_cTime));
    memset(u_ustcConfig.stcConfig.arr_cMacid,0,sizeof(u_ustcConfig.stcConfig.arr_cMacid));
    
    File fFileConfig = fs.open(sFileConfig.c_str(), FILE_READ);
    if(fFileConfig)
    {
      fFileConfig.read((uint8_t*)u_ustcConfig.arr_cConfig,sizeof(u_ustcConfig.arr_cConfig));
      Serial.print("Config file read sucessfull , No of photo : ");
      Serial.println(u_ustcConfig.stcConfig.u_nNumOfPics);
    }
    else
    {
      fFileConfig = fs.open(sFileConfig.c_str(),FILE_WRITE);
      
      u_ustcConfig.stcConfig.u_nNumOfPics = 0;

      fFileConfig.write((uint8_t*)u_ustcConfig.arr_cConfig,sizeof(u_ustcConfig.arr_cConfig));
    }
    fFileConfig.close();
    String strMacId = WiFi.macAddress();
    sprintf(u_ustcConfig.stcConfig.arr_cMacid,"%s",strMacId.c_str());
}
void ReadConnectionParameters(void)
{
   DEFAULT_CONN_DATA_FILE_VALUES;
   char arr_cBuff[256] = {0};
   fs::FS &fs = SD_MMC; 
   String sFileConn = SSID_PASS_FILE_NAME;

  File fFileConn = fs.open(sFileConn.c_str(), FILE_READ);
  if(fFileConn)
  {
    fFileConn.read((uint8_t*)arr_cBuff,MAX_SIZE_OF_CONNECTION_FILE);
    DeserializationError error = deserializeJson(g_jsonFile, arr_cBuff);
    if(error) 
    {
      Serial.print(F("Connection details read - deserializeJson() failed: "));
      Serial.println(error.f_str());
      Serial.println("Connection file deleted");
      fs.remove(sFileConn.c_str());
    }
    else
    {
      strncpy(e_arr_cSSID,g_jsonFile["SSID"],sizeof(e_arr_cSSID));
      strncpy(e_arr_cPassKey,g_jsonFile["Password"],sizeof(e_arr_cPassKey));

      g_bContPicEnableFlag = (bool)atoi(g_jsonFile["ContinousPicEnable"]);
      g_nContPicIntervel = atoi(g_jsonFile["ContinousPicintervel"]);
      strncpy(e_arr_cPassKey,g_jsonFile["Password"],sizeof(e_arr_cPassKey));
      strncpy(e_arr_cPassKey,g_jsonFile["Password"],sizeof(e_arr_cPassKey));
      
      Serial.println("Wifi connection details read sucessfull");
      Serial.print("SSID : ");
      Serial.println(e_arr_cSSID);
      Serial.print("Password : ");
      Serial.println(e_arr_cPassKey);
      Serial.print("Continous capture enable : ");
      Serial.println(g_bContPicEnableFlag);
      Serial.print("Continous caprture intervel : ");
      Serial.println(g_nContPicIntervel);
    }
  }
  else
  {
    fFileConn = fs.open(sFileConn.c_str(),FILE_WRITE);

    g_bContPicEnableFlag = NORMAL_PHOTO_TIME_WIDTH;
    g_bContPicEnableFlag = true;
    
    strncpy(e_arr_cSSID,DEFAULT_SSID,sizeof(e_arr_cSSID));
    strncpy(e_arr_cPassKey,DEFAULT_PASSKEY,sizeof(e_arr_cPassKey));

    strncpy(arr_cBuff,DEFAULT_CONN_DATA_FILE_VALUES,sizeof(arr_cBuff));

    fFileConn.write((uint8_t*)arr_cBuff,strlen(arr_cBuff));
  }
  fFileConn.close();
}
bool FetchTimeFromNTP(char *ptr_arr_cDate,char *ptr_arr_cTime)
{
  const char *g_ptr_arr_cntpServer = "pool.ntp.org";
  const long  g_lgmtOffset_sec = (5 * 3600) + (60 * 30);
  const int   g_ndaylightOffset_sec = 0;

  struct tm stcNtpTime;
  configTime(g_lgmtOffset_sec,g_ndaylightOffset_sec,g_ptr_arr_cntpServer);
  if(!getLocalTime(&stcNtpTime))
  {
    Serial.println("Failed to obtain NTP date and time");
    return false;
  }

  sprintf(ptr_arr_cDate,"%02d-%02d-%02d",stcNtpTime.tm_mday,stcNtpTime.tm_mon + 1,(1900 + stcNtpTime.tm_year) - 2000);
  sprintf(ptr_arr_cTime,"%02d-%02d-%02d",stcNtpTime.tm_hour,stcNtpTime.tm_min,stcNtpTime.tm_sec);
  Serial.print("NTP time get : ");
  Serial.print(ptr_arr_cDate);
  Serial.print(" ");
  Serial.println(ptr_arr_cTime);
  return true;
}
void InitializaAllGpio(void)
{
  pinMode(PIN_LED_FLASH, OUTPUT);
  digitalWrite(PIN_LED_FLASH, LOW);
  pinMode(PIN_PIR_DETECT,INPUT);  
}
bool TimerCallbackTimerNormal(void *)
{
  static int nTimeDivider = 0;
  if(g_nContPicIntervel <= nTimeDivider)
  {
    g_TakeSnapShotFlagNormal = true;
    nTimeDivider = 0;
  }
  ++nTimeDivider;
  return true;
}
bool TimerCallbackTimerTimerBrust(void *)
{
  static int nPhotoCount = 0;
  if(true == g_bPirDetected)
  {
    g_TakeSnapShotFlagBrust = true;
    Serial.print("Brust mode capture : ");
    Serial.println(nPhotoCount + 1);
    if(MAX_BRUST_PHOTO_COUNT <= nPhotoCount)
    {
      g_bPirDetected = false;
      nPhotoCount = 0;
    }
    else
    {
      ++nPhotoCount;
    }
  }

  return true;
}
bool checkforpirdetect(void)
{
  bool bRet = false;
  static int bPrevStatus = false;
  int nPinStatus = digitalRead(PIN_PIR_DETECT);
  if(nPinStatus && false == bPrevStatus)
  {
    g_bPirDetected = bRet = true;
  }
  bPrevStatus = nPinStatus;
}
void setup() 
{
  bool bErrStatus = true;
  InitializaAllGpio();
  Serial.begin(SERIAL_BAUDRATE);
  if(false == initSDcard())
  {
    bErrStatus = false;
    Serial.println("SD Card Mount Failed");
  }
  if(false == checkSDCardStatus())
  {
    bErrStatus = false;
    Serial.println("No SD Card attached");
  }
  InitializeBuffers();
  ReadConnectionParameters();
  Serial.print("Wifi connecting to :");
  Serial.println(e_arr_cSSID);
  WiFi.begin(e_arr_cSSID,e_arr_cPassKey);
  if(true == bErrStatus)//init camera only if SD mount is sucessfull.
  {
    g_bCamerConfigStatus = configCamera();
    if(false != g_bContPicEnableFlag)
    {
      e_TimerNormal.every(NORMAL_TIMER_WIDTHIN_mSEC,TimerCallbackTimerNormal);
    }
    e_TimerBrust.every(BRUST_TIMER_WIDTHIN_mSEC,TimerCallbackTimerTimerBrust);
  }
  else
  {
    unsigned int nErrCnt = 0;
    while(1)//error loop
    {
      delay(100);
      nErrCnt++;
      if(10 <= nErrCnt)
      {
        nErrCnt = 0;
        Serial.println("Error Init SD failed");
      }
    }
  }
}

void loop()
{
  static int nPrevWifiConnStstus = WL_DISCONNECTED;
  if(WL_CONNECTED == WiFi.status() && WL_DISCONNECTED ==  nPrevWifiConnStstus)
  {
    g_nCurrWifiStatus = WL_CONNECTED;
    FetchTimeFromNTP(u_ustcConfig.stcConfig.arr_cDate,u_ustcConfig.stcConfig.arr_cTime);
    Serial.print("Wifi connected to :");
    Serial.println(e_arr_cSSID);
  }
  else if(WL_DISCONNECTED == WiFi.status() && WL_CONNECTED == nPrevWifiConnStstus)
  {
    g_nCurrWifiStatus = WL_DISCONNECTED;
    Serial.print("Wifi disconnected from :");
    Serial.println(e_arr_cSSID);
  }
  else if(WL_CONNECTED == WiFi.status() && WL_CONNECTED ==  nPrevWifiConnStstus)
  {
    g_nCurrWifiStatus = WL_CONNECTED;
  }
  nPrevWifiConnStstus = g_nCurrWifiStatus;
  checkforpirdetect();
  if(true == g_TakeSnapShotFlagBrust)
  {
    SnapPhoto();
    g_TakeSnapShotFlagBrust = false;
  }
  else if(true == g_TakeSnapShotFlagNormal)
  {
    SnapPhoto();
    g_TakeSnapShotFlagNormal = false;
  }
  e_TimerNormal.tick();
  e_TimerBrust.tick();
}
