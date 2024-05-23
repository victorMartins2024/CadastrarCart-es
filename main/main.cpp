/*-----------------------------------------------------------------

  Telemetry V0.5.1 main.cpp
     
  INA226
  MFRC522 
  LCD 16x2 display

  Compiler: VsCode 1.88.1
  MCU: ESP32
  Board: Dev module 28 pins

  Author: João  G. Uzêda & Victor Hugo
  date: 2024, May

-----------------------------------------------------------------*/

// ----------------------------------------------------------------- 
// ---Libraries---
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "LiquidCrystal_I2C.h"
#include "freertos/task.h"
#include "PubSubClient.h"
#include "esp_task_wdt.h"
#include "Preferences.h"
#include "rtc_wdt.h"
#include "Arduino.h"
#include "MFRC522.h"
#include "INA226.h"
#include "WiFi.h"
#include "Wire.h"
#include "SPI.h"
// -----------------------------------------------------------------  

// ----------------------------------------------------------------- 
// ---defines---
#define SHUNT_RESISTENCE 0.75

// ----------------------------------------------------------------- 

// -----------------------------------------------------------------  
//----I²C Adresses------
#define LCDADRESS 0x27 
#define INAADRESS 0x40 
// -----------------------------------------------------------------  

// -----------------------------------------------------------------  
//----pins------
#define   SS        22
#define   RST       5
#define   SCK       14
#define   MISO      2
#define   MOSI      15
#define   SDA       1
#define   SCL       3 

// -----------------------------------------------------------------  

// ----------------------------------------------------------------- 
// ---connection infos--
const char *ssid    =    "Greentech_Administrativo";             
const char *pass    =    "Gr3enTech@2O24*";   
const char *mqtt    =    "192.168.30.130";      // rasp nhoqui
//const char *mqtt    =    "192.168.30.212";    // rasp eng
const char *user    =    "greentech";                           
const char *passwd  =    "Greentech@01";                       
int         port    =    1883;    
// -----------------------------------------------------------------

// -----------------------------------------------------------------  
// -----Topics-----
const char *topic_JHour   =    "proto/sim/horimetro/ApenasHora";
const char *topic_FHour   =    "proto/sim/horimetro/HoraTotal";
const char *topic_HBomb   =    "proto/sim/horimetro/Elevação";
const char *topic_Htrac   =    "proto/sim/horimetro/Tração";
const char *topic_A2      =    "proto/sim/Corrente/elevacao";
const char *topic_A3      =    "proto/sim/Corrente/tracao";
const char *topic_A       =    "proto/sim/Corrente";
const char *topic_T       =    "proto/sim/tensao";  
const char *Topic_user    =    "proto/sim/Usuario";  
const char *topic_V       =    "proto/sim/checklist/P1:";                   
const char *topic_G       =    "proto/sim/checklist/P2:";                      
const char *topic_E       =    "proto/sim/checklist/P3:";               
const char *topic_F       =    "proto/sim/checklist/P4";              
const char *topic_B       =    "proto/sim/checklist/P5:";  
const char *topic_TEC     =    "proto/sim/manutenção";
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----Functions----
void recon();
void ina226_setup();
void xTaskNav(void *pvParameters);
void xTaskTelemetry(void *pvParameters);
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----Objects----
Preferences pref;
WiFiClient TestClient;
MFRC522 rfid(SS, RST);
INA226_WE INA(INAADRESS);
PubSubClient client(TestClient);
LiquidCrystal_I2C lcd(LCDADRESS, 16, 2);
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// ----Variables----
float geralA;
float Volt;
float ABombH;
float ATrac;
int hourmeter;
int hourmeterT;
int hourmeterB;
int minute;
int minuteT;
int minuteB;
int sec;
int secT;
int secB;
int opa = 0;
byte a = 7;
byte b = 5;
byte maxtaglen = 6;
byte maxpasslen = 5;
byte currentlenlen = 0;
byte currentpasslen = 0;
bool passvalue = true;
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// --Preferences Key---
const char *minpref   =   "min";
const char *mintrac   =   "trac";
const char *minbomb   =   "minbomb";
const char *hourpref  =   "hour";
const char *hourtrac  =   "hourtrac";
const char *hourbomb  =   "hourbomb";
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// UIDS
String OpeCard = "6379CF9A";  
String AdmCard = "29471BC2";  
String TecCard = "D2229A1B";  
String PesCard = "B2B4BF1B";
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// buffers 
char uid_buffer[32];
bool opnav;
bool engnav;
// -----------------------------------------------------------------

extern "C" void app_main(){
  initArduino();
  Wire.begin(SDA, SCL);
  SPI.begin(SCK, MISO, MOSI, RST);
  rfid.PCD_Init();
  pref.begin("GT", false);
  WiFi.begin(ssid, pass); 
  lcd.init();
  lcd.backlight();

  ina226_setup();

  lcd.home();
  lcd.print("INICIALIZANDO");
  vTaskDelay(500);
  
  while(WiFi.status() != WL_CONNECTED);                                                                                               

  client.setServer(mqtt, port);           
  while (!client.connected()){                            
    client.connect("ESP32ClientU", user, passwd);    
    vTaskDelay(500);                                         
  } 

  xTaskCreatePinnedToCore(xTaskTelemetry, // function name
                          "Telemetry",    // task name
                          8000,           // stack size in word
                          NULL,           // input parameter
                          1,              // priority
                          NULL,           // task handle
                          1);                // core

  xTaskCreatePinnedToCore(xTaskNav,
                          "Navegation",
                          8000,
                          NULL,
                          2,
                          NULL,
                          0);


  vTaskDelay(1000);  
}


/*-----------------------------------------------------------------------------
--------------------------------FUNCTIONS--------------------------------------
-----------------------------------------------------------------------------*/


// -----------------------------------------------------------------
// -----Reconection-----
void recon(){
  while (WiFi.status() != WL_CONNECTED){
    WiFi.disconnect();        
    vTaskDelay(1000);         
    WiFi.begin(ssid, pass);   
    vTaskDelay(1000);         
  }         
  vTaskDelay(500);
  while (!client.connected()){                        
    client.connect("TestClient", user, passwd);     
    vTaskDelay(5000);                                  
  }                    
}
// -----------------------------------------------------------------   

// -----------------------------------------------------------------
// -----INA setup-----
void ina226_setup(){
  INA.init();
  INA.setAverage(AVERAGE_4); 
  INA.setConversionTime(CONV_TIME_1100); 
  INA.setResistorRange(SHUNT_RESISTENCE, 300.0); 
  INA.waitUntilConversionCompleted(); 	
}
// -----------------------------------------------------------------


/*-----------------------------------------------------------------------------
--------------------------------Tasks------------------------------------------
-----------------------------------------------------------------------------*/

// -----------------------------------------------------------------
// -----telemetry-----
void xTaskTelemetry(void *pvParameters){ 
  lcd.clear(); 
  
  char CVolt[30];
  char Ageral[30];  
  char CATrac[30];
  char CABombH[30];
  char Justhour[30];
  char FullHour[30];
  char JusthourT[30];
  char JusthourB[30];
  char Amp_buffer[30];
  char Volt_buffer[30];

  minute = pref.getInt(minpref, minute);
  hourmeter = pref.getInt(hourpref, hourmeter); 
  hourmeterT = pref.getInt(hourtrac, hourmeterT);
  hourmeterB = pref.getInt(hourbomb, hourmeterB);

  esp_task_wdt_add(NULL);        //  enable watchdog     
  while(1){  
    if (WiFi.status() != WL_CONNECTED  || !client.connected())
      recon();

    rtc_wdt_feed();    //  feed watchdog 

    INA.readAndClearFlags();

    Volt = INA.getBusVoltage_V();
    geralA = INA.getShuntVoltage_mV() / SHUNT_RESISTENCE;

    ABombH = geralA - 1.5;
    ATrac = geralA - 2.5;

    if (geralA >= 3.5){ // --------------General Hourmeter--------------------------
      sec++;   
      if (sec >= 60){         
        sec -= 60;
        minute++;
        if (minute == 10)
          pref.putInt(minpref, minute);
        else if (minute == 20)
          pref.putInt(minpref, minute);
        else if (minute == 30)
          pref.putInt(minpref, minute);
        else if (minute == 40)
          pref.putInt(minpref, minute);
        else if (minute == 50)
          pref.putInt(minpref, minute);
      }                       
      if (minute >= 60){
        minute -= 60;
        hourmeter++;
        pref.remove(minpref);
        pref.putInt(hourpref, hourmeter);
      }                     
    } // -----------------------------------------------------------------------   

    if (ABombH >= 13){  // --------Hidraulic bomb hourmeter----------------------
      secB++;
      if (secB >= 60){        
        secB-=60;
        minuteB++;
        if (minuteB == 20)
          pref.putInt(minbomb, minuteB);
        else if (minuteB == 40)
          pref.putInt(minbomb, minuteB);       
      }
      if (minuteB >= 60){
        minuteB-=60;
        hourmeterB++;
        pref.remove(minbomb);
        pref.putInt(hourbomb, hourmeterB);
      }
    } // --------------------------------------------------------------------------

    if (geralA >= 13){  // ---------Trasion engine hourmeter----------------------
      secT++;
      if (secT >= 60){        
        secT-=60;
        minuteT++;
        if (minuteT == 20)
          pref.putInt(mintrac, minuteT);
        else if (minuteT == 40)
          pref.putInt(mintrac, minuteT);
      }
      if (minuteT >= 60){
        minuteT-=60;
        hourmeterT++;
        pref.remove(mintrac);
        pref.putInt(hourtrac, hourmeterT);
      }
    } // -----------------------------------------------------------------------
    
    sprintf(CABombH, "{\"Corrente Bomba\":%.02f}", ABombH);
    sprintf(Ageral, "{\"Corrente geral\":%.02f}", geralA);
    sprintf(CATrac, "{\"Corrente tracao\":%.02f}", ATrac);
    sprintf(CVolt, "{\"Tensao geral\":%.02f}", Volt);
    sprintf(FullHour, "{\"hourmeter\": %02d:%02d:%02d}",
                                hourmeter, minute, sec);
    sprintf(Justhour, "{\"JustHour\":%d}", hourmeter);
    sprintf(JusthourB, "{\"TracHour\":%d}", hourmeterB);
    sprintf(JusthourT, "{\"HbombHour\":%d}", hourmeterT);
    sprintf(Amp_buffer, "%.02f", geralA);
    sprintf(Volt_buffer, "%.02f", Volt);
    
    client.publish(topic_A,     Ageral);
    client.publish(topic_A2,    CABombH);
    client.publish(topic_A3,    CATrac);
    client.publish(topic_T,     CVolt);
    client.publish(topic_HBomb, JusthourB);
    client.publish(topic_Htrac, JusthourT);
    client.publish(topic_FHour, FullHour);
    client.publish(topic_JHour, Justhour);

    lcd.setCursor(0, 0);
    lcd.print("H:");
    lcd.setCursor(3, 0);
    lcd.print(hourmeter);

    lcd.setCursor(0, 1);
    lcd.print("A:");
    lcd.setCursor(2, 1);
    lcd.print(Amp_buffer);

    lcd.setCursor(9, 1);
    lcd.print("V:");
    lcd.setCursor(11, 1);
    lcd.print(Volt);

    esp_task_wdt_reset();        // reset watchdog if dont return any error

    client.loop();
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
} 
// -----------------------------------------------------------------

void xTaskNav(void *pvParameters){
  esp_task_wdt_add(NULL);      //  enable watchdog     
  while(1){
    rtc_wdt_feed();                  //  feed watchdog 
    
    esp_task_wdt_reset();            // reset watchdog if dont return any error
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
}
