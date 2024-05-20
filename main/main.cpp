/*-----------------------------------------------------------------

  Telemetry V0.3 main.cpp
     
  INA226
  MFRC522 
  LCD 16x2 display

  Compiler: VsCode 1.88.1
  MCU: ESP32
  Board: Dev module 28 pins

  Author: João  G. Uzêda
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
const char *mqtt    =    "192.168.30.130";      //192.168.30.139
const char *user    =    "greentech";                           
const char *passwd  =    "Greentech@01";                       
int         port    =    1883;    
// -----------------------------------------------------------------

// -----------------------------------------------------------------  
// -----Topics-----
const char *topic_JHour   =    "proto/sim/horimetro/ApenasHora";
const char *topic_FHour   =    "proto/sim/horimetro/HoraTotal";
const char *topic_HBomb   =   "proto/sim/horimetro/Elevação";
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

void reconW();
void reconB();
void telemetry();
void ina226_setup();

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
  WiFi.begin(ssid, pass); 
  lcd.init();
  lcd.backlight();

  ina226_setup();

  lcd.home();
  lcd.print("INICIALIZANDO");
  delay(1000);
  lcd.clear();

  while(WiFi.status() != WL_CONNECTED);                                                                                               

  client.setServer(mqtt, port);           
  while (!client.connected()){                            
    client.connect("ESP32ClientU", user, passwd);    
    delay(500);                                         
  }  

  minute = pref.getInt(minpref, minute);
  hourmeter = pref.getInt(hourpref, hourmeter); 
  hourmeterT = pref.getInt(hourtrac, hourmeterT);
  hourmeterB = pref.getInt(hourbomb, hourmeterB);

  while(1){
    if (WiFi.status() != WL_CONNECTED){            //   -----------------------------------------------------
      reconW();                                    //     if the esp lost conection with
      delay(250);                                  //      wifi and/or broke this section will
    }else if (!client.connected()){                //      take care of reconnet to wifi and/or broker
      reconB();                                    //
      delay(250);                                  //   ------------------------------------------------------
    }   
    telemetry();
    delay(1000);
  }   
}


/*-----------------------------------------------------------------------------
--------------------------------FUNCTIONS--------------------------------------
-----------------------------------------------------------------------------*/


// -----------------------------------------------------------------
// -----Reconection-----
void reconW(){
  while (WiFi.status() != WL_CONNECTED){
    WiFi.disconnect();        
    vTaskDelay(1000);         
    WiFi.begin(ssid, pass);   
    vTaskDelay(1000);         
  }                           
}

void reconB(){
  while (!client.connected()){                        
    client.connect("TestClient", user, passwd);     
    vTaskDelay(5000);                                  
  }                                                   
} 
// -----------------------------------------------------------------   

// -----------------------------------------------------------------
// -----telemetry-----
void telemetry(){  
  char CVolt[30];
  char Ageral[30];  
  char CATrac[30];
  char CABombH[30];
  char Justhour[30];
  char FullHour[30];
  char Amp_buffer[30];
  char Volt_buffer[30];

  char JusthourT[30];
  char JusthourB[30];

  INA.readAndClearFlags();
  float Volt = INA.getBusVoltage_V();
  float geralA = INA.getShuntVoltage_mV() / SHUNT_RESISTENCE;

  ABombH = geralA - 3;
  ATrac = geralA - 2;

  if (geralA >= 4){ // --------------General Hourmeter--------------------------
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
  }  // -----------------------------------------------------------------------   

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
  }// --------------------------------------------------------------------------

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
  
  sprintf(CABombH, "{\"Corrente Bomba\": %.02f}", ABombH);
  sprintf(Ageral, "{\"Corrente geral\": %.02f}", geralA);
  sprintf(CATrac, "{\"Corrente tração\": %.02f}", ATrac);
  sprintf(CVolt, "{\"Tensão geral\": %.02f}", Volt);
  sprintf(FullHour, "{\"hourmeter\": %02d:%02d:%02d}",
                              hourmeter, minute, sec);
  sprintf(Justhour, "{\"JustHour\":%d}", hourmeter);
  sprintf(JusthourB, "{\"TracHour\":%d}", hourmeterB);
  sprintf(JusthourT, "{\"HbombHour\":%d}", hourmeterT);
  sprintf(Amp_buffer, "%.02f", geralA);
  sprintf(Volt_buffer, "%.02f", Volt);
    
  lcd.setCursor(0, 0);
  lcd.print("H:");
  lcd.setCursor(3, 0);
  lcd.print(hourmeter);

  lcd.setCursor(0, 1);
  lcd.print("A:");
  lcd.setCursor(3, 1);
  lcd.print(Amp_buffer);

  lcd.setCursor(9, 1);
  lcd.print("V:");
  lcd.setCursor(11, 1);
  lcd.print(Volt);

  client.publish(topic_A,     Ageral);
  client.publish(topic_A2,    CABombH);
  client.publish(topic_A3,    CATrac);
  client.publish(topic_T,     CVolt);
  client.publish(topic_HBomb, JusthourB);
  client.publish(topic_Htrac, JusthourT);
  client.publish(topic_FHour, FullHour);
  client.publish(topic_JHour, Justhour);

  client.loop();
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