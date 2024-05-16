/*-----------------------------------------------------------------

  Telemetry V0.3 main.cpp

  MQtt
  Flash memory
  Hourmeter counter      
  INA + shunt system
  Display OLED
  LCD 16x2 display

  Compiler: VsCode 1.88.1
  MCU: ESP32 DEV MODULE

  Author: João  G. Uzêda && Victor Martins
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
#include "Password.h"
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
const char *mqtt    =    "192.168.30.206";            
const char *user    =    "greentech";                           
const char *passwd  =    "Greentech@01";                       
int         port    =    1883;    
// -----------------------------------------------------------------

// -----------------------------------------------------------------  
// -----Topics-----
const char *topic_JHour   =    "proto/sim/ApenasHora";
const char *topic_FHour   =    "proto/sim/HoraTotal";
const char *topic_A2      =    "proto/sim/elevacao";
const char *topic_A3      =    "proto/sim/tracao";
const char *topic_A       =    "proto/sim/geralA";
const char *topic_T       =    "proto/sim/tensao";  
const char *Topic_user    =    "proto/sim/rfid";  
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
WiFiClient TestClient;
MFRC522 rfid(SS, RST);
INA226_WE INA(INAADRESS);
PubSubClient client(TestClient);
LiquidCrystal_I2C lcd(LCDADRESS, 16, 2);
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// ----Variables----
bool passvalue = true;
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
byte maxpasslen = 5;
byte currentpasslen = 0;
byte maxtaglen = 6;
byte currentlenlen = 0;
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// --Preferences Key---
const char *minpref = "min";
const char *hourpref = "hour";
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// UIDS
String OpeCard = "6379CF9A";  
String AdmCard = "29471BC2";  
String TecCard = "D2229A1B";  
String PesCard = "B2B4BF1B";

// -----------------------------------------------------------------
// buffers 
char uid_buffer[32];
bool opnav;
bool engnav;

extern "C" void app_main(){
  initArduino();
  Wire.begin(SDA, SCL);
  SPI.begin(SCK, MISO, MOSI, RST);
  rfid.PCD_Init();
  WiFi.begin(ssid, pass); 
  lcd.init();
  lcd.backlight();

  ina226_setup();

  while(WiFi.status() != WL_CONNECTED);                                                                                               

  client.setServer(mqtt, port);           
  while (!client.connected()){                            
    client.connect("ESP32ClientU", user, passwd);    
    delay(500);                                         
  }  

  lcd.home();
  lcd.print("INICIALIZANDO");
  delay(1000);
  lcd.clear();  

  while(1){
    client.loop();
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

// ----------------------------------------------------------
// -----telemetry-----
void telemetry(){  
  char Ageral[30];
  char CABombH[30];
  char CATrac[30];
  char CVolt[30];
  char Justhour[30];
  char FullHour[30];

  if (WiFi.status() != WL_CONNECTED){  //   -----------------------------------------------------
    reconW();                          //     if the esp lost conection with
    delay(250);                        //      wifi and/or broke this section will
  }else if (!client.connected()){      //      take care of reconnet to wifi and/or broker
    reconB();                          //
    delay(250);                        //   ------------------------------------------------------
  }
  INA.readAndClearFlags();
  float Volt = INA.getBusVoltage_V();
  float geralA = INA.getShuntVoltage_mV() / SHUNT_RESISTENCE;

  ABombH = geralA - 72.671;
  ATrac = geralA - 53.555;

  if (geralA >= 50){
    sec++;   
    if (sec >= 60){         //  -------------------
      sec -= 60;
      minute++;
    }                       //  horimetro geral
    if (minute >= 60){
      minute -= 60;
      hourmeter++;
    }                       //  -------------------
  }
  if (geralA >= 60){
    secB++;
    if (secB >= 60){         //  -------------------
      secB-=60;
      minuteB++;
    }
    if (minuteB >= 60){
      minuteB-=60;
      hourmeterB++;
    }
  }
    if (geralA >= 60){
    secT++;
    if (secT >= 60){         //  -------------------
      secT-=60;
      minuteT++;
    }
    if (minuteT >= 60){
      minuteT-=60;
      hourmeterT++;
    }
  }

  
  sprintf(CABombH,  "{\"Corrente Bomba\": %.02f}", ABombH);
  sprintf(Ageral,   "{\"Corrente geral\": %.02f}", geralA);
  sprintf(CATrac,   "{\"Corrente tração\": %.02f}", ATrac);
  sprintf(CVolt,    "{\"Tensão geral\": %.02f}", Volt);
  sprintf(FullHour, "{\"hourmeter\": %02d:%02d:%02d}",
                              hourmeter, minute, sec);
  sprintf(Justhour, "{\"JustHour\":%d}", hourmeter);
  sprintf(Justhour, "{\"TracHour\":%d}", hourmeterB);
  sprintf(Justhour, "{\"HbombHour\":%d}", hourmeterT);
    
  lcd.setCursor(0, 0);
  lcd.print("H:");
  lcd.setCursor(3, 0);
  lcd.print(hourmeter);

  lcd.setCursor(0, 1);
  lcd.print("A:");
  lcd.setCursor(3, 1);
  lcd.print(geralA);

  lcd.setCursor(9, 1);
  lcd.print("V:");
  lcd.setCursor(11, 1);
  lcd.print(Volt);

  client.publish(topic_A, Ageral);
  client.publish(topic_A2, CABombH);
  client.publish(topic_A3, CATrac);
  client.publish(topic_T, CVolt);
  client.publish(topic_FHour, FullHour);
  client.publish(topic_JHour, Justhour);

  client.loop();
} 

void ina226_setup(){
  INA.init();
  INA.setAverage(AVERAGE_4); 
  INA.setConversionTime(CONV_TIME_1100); 
  INA.setResistorRange(SHUNT_RESISTENCE,300.0); 
  INA.waitUntilConversionCompleted(); 	
}