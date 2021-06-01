#ifndef SYSTEM_H
#define SYSTEM_H


#define EEPROM_SIZE 1// define the number of bytes you want to access

#define SERIAL_BAUDRATE 115200
/*Pin definitions of camera - START*/
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
/*Pin definitions of camera - END*/
#define LENGTH_OF_SSID_PASS 32
#define DEFAULT_SSID    "DEFAULT SSID"
#define DEFAULT_PASSKEY "DEFAULT PASSWORD"
#define DEFAULT_DATE    "DD_MM_YYYY"
#define DEFAULT_TIME    "HH_MM_SS"
#define DEFAULT_CONN_DATA_FILE_VALUES "{\"SSID\":\"Aldrin\",\"Password\":\"1351824120\",\"ContinousPicEnable\":\"1\",\"ContinousPicintervel\":\"10\"}"

#define CAMERA_DEFAULT_DELAY      500//MS
#define MAX_BRUST_PHOTO_COUNT     3//Number of photos
#define NORMAL_PHOTO_TIME_WIDTH   30// seconds
#define NORMAL_TIMER_WIDTHIN_mSEC (1 * 1000)
#define BRUST_TIMER_WIDTHIN_mSEC  (2 * 1000)

#define PIN_LED_FLASH  4
#define PIN_PIR_DETECT 16
#define MAX_PIR_DETECT_COUNT 100000

#define MAX_SIZE_OF_CONNECTION_FILE   128
#define CONFIG_FILE_NAME  "/Config.bin"
#define SSID_PASS_FILE_NAME "/Connection.json"  
 
extern char e_arr_cSSID[LENGTH_OF_SSID_PASS];
extern char e_arr_cPassKey[LENGTH_OF_SSID_PASS];

typedef struct
{
  unsigned int u_nNumOfPics;
  char arr_cMacid[LENGTH_OF_SSID_PASS];
  char arr_cDate[LENGTH_OF_SSID_PASS];
  char arr_cTime[LENGTH_OF_SSID_PASS];
}Config;
typedef union
{
  Config stcConfig;
  char arr_cConfig[sizeof(Config)];
}ustcConfig;

extern ustcConfig  u_ustcConfig;


bool configCamera(void);
bool initSDcard(void);
bool checkSDCardStatus(void);
bool SnapPhoto(void);
void InitializeBuffers(void);
bool FetchTimeFromNTP(char *ptr_arr_cDate,char *ptr_arr_cTime);
void InitializaAllGpio(void);
void ReadConnectionParameters(void);
bool checkforpirdetect(void);

#endif
