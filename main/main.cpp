/*----------------------------------------------------------------

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

// ---------------------------------------------------------------- 
// ---Libraries---
#include "freertos/FreeRTOSConfig.h"
#include "LiquidCrystal_I2C.h"
#include"freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "PubSubClient.h"
#include "Preferences.h"
#include "Keypad_I2C.h"
#include "esp_system.h"
#include "Password.h"
#include "rtc_wdt.h"
#include "Arduino.h"
#include "MFRC522.h"
#include "INA226.h"
#include "stdio.h"
#include "WiFi.h"
#include "Wire.h"
#include "SPI.h"

// ---------------------------------------------------------------- 
// ---defines---
#define SHUNT_RESISTENCE  0.75
char keypad[19] = "123A456B789C*0#DNF";

// ----------------------------------------------------------------  
//----I²C Adresses------
#define LCDADRESS 0x27 
#define INAADRESS 0x40 
#define KeyAdress 0x21

// ----------------------------------------------------------------  
//----pins------
#define   SS        22
#define   RST       5
#define   SCK       14
#define   MISO      2
#define   MOSI      15
#define   SDA       1
#define   SCL       3 

// ---------------------------------------------------------------- 
// ---connection infos--
const char *ssid    =    "Greentech_Administrativo";             
const char *pass    =    "Gr3enTech@2O24*";   
const char *mqtt    =    "192.168.30.130";      // rasp nhoqui
//const char *mqtt    =    "192.168.30.212";    // rasp eng
const char *user    =    "greentech";                           
const char *passwd  =    "Greentech@01";                       
int         port    =    1883;    

// ----------------------------------------------------------------  
// -----Topics-----
const char *topic_JHour   =    "proto/sim/horimetro/ApenasHora";
const char *topic_FHour   =    "proto/sim/horimetro/HoraTotal";
const char *topic_HBomb   =    "proto/sim/horimetro/Elevação";
const char *topic_A2      =    "proto/sim/Corrente/elevacao";
const char *topic_Htrac   =    "proto/sim/horimetro/Tração";
const char *topic_A3      =    "proto/sim/Corrente/tracao";
const char *topic_V       =    "proto/sim/checklist/P1:";                   
const char *topic_G       =    "proto/sim/checklist/P2:";                      
const char *topic_E       =    "proto/sim/checklist/P3:"; 
const char *topic_B       =    "proto/sim/checklist/P5:";
const char *topic_F       =    "proto/sim/checklist/P4";  
const char *topic_TEC     =    "proto/sim/manutenção";
const char *topic_CAD     =    "sinal/sim/Cadastro";
const char *topic_A       =    "proto/sim/Corrente";
const char *Topic_user    =    "proto/sim/Usuario";
const char *topic_T       =    "proto/sim/tensao";  

// ----------------------------------------------------------------
// -----Functions----
void xTaskTelemetry(void *pvParameters);
void xTaskNav(void *pvParameters);
void ina226_setup();
void readpref();





// ----------------------------------------------------------------
// -----Objects----
Preferences pref;
WiFiClient TestClient;
MFRC522 rfid(SS, RST);
INA226_WE INA(INAADRESS);
Keypad_I2C kpd(KeyAdress, SDA);  
PubSubClient client(TestClient);
Password password = Password("2552" ); 
LiquidCrystal_I2C lcd(LCDADRESS, 20, 4);

// ----------------------------------------------------------------
// ----Variables----
float geralA;
float ABombH;
float ATrac;
float Volt;
int sec;
int secT;
int secB;
int minute;
int minuteT;
int minuteB;
int hourmeter;
int hourmeterT;
int hourmeterB;

// ----------------------------------------------------------------
// ----Constants----
byte a = 7;
byte b = 5;
byte maxtaglen = 6; //PassLenghtMax
byte maxpasslen = 5;  //maxPasswordLength

// ----------------------------------------------------------------
// ----Buffers----
byte currentpasslen = 0;   //currentPasswordLength
byte currenttaglen = 0; //PassLenghtAtual
char uid_buffer[32];
char CAD[32];
int  manup =  0; //preve
int  manuc =  0; //corre
bool opnav;
bool engnav;
bool psswdcheck;
bool passvalue = true;
int  manup =  0; //preve
int  manuc =  0; //corre

// ----------------------------------------------------------------
// --Preferences Key---
const char *minpref   =   "min";
const char *mintrac   =   "trac";
const char *minbomb   =   "minbomb";
const char *hourpref  =   "hour";
const char *hourtrac  =   "hourtrac";
const char *hourbomb  =   "hourbomb";
const char *prevpref  =   "Manupreve";
const char *correpref =   "Manucorre";
const char *cadaspref =   "Cadastro";
const char *listapref =   "Cadastro Cartoes";

// ----------------------------------------------------------------
// UIDS
String OpeCard  =   "6379CF9A";  
String AdmCard  =   "29471BC2";  
String TecCard  =   "D2229A1B";  
String PesCard  =   "B2B4BF1B";
String UIDLists =   " ";
String TAGLists =   " ";

extern "C" void app_main(){
  initArduino();
  kpd.begin();
  lcd.init();
  rfid.PCD_Init();
  ina226_setup();
  Wire.begin(SDA, SCL);
  WiFi.begin(ssid, pass); 
  pref.begin("GT", false);
  SPI.begin(SCK, MISO, MOSI, RST);

  lcd.backlight();

  if(WiFi.status() != WL_CONNECTED)
    WiFi.reconnect();                                                                                               
  
  client.setServer(mqtt, port);           
  if (!client.connected())                            
    client.connect("TestClient", user, passwd);                                       
  
  readpref();

  xTaskCreatePinnedToCore(xTaskTelemetry, // function name
                          "Telemetry",    // task name
                          8000,           // stack size in word
                          NULL,           // input parameter
                          1,              // priority
                          NULL,           // task handle
                          1);             // core

  xTaskCreatePinnedToCore(xTaskNav,
                          "Navegation",
                          16000,
                          NULL,
                          1,
                          NULL,
                          0);

  lcd.setCursor(0, 0);
  lcd.print("INICIALIZANDO");
  vTaskDelay(1000);                            
}

/*---------------------------------------------------------------------------------
---------------------------------------------Tasks-------------------------------*/
// -----------------------------------------------------------------
// -----telemetry-----
void xTaskTelemetry(void *pvParameters){ 
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
  char hourmeter1[30];

  esp_task_wdt_add(NULL);        //  enable watchdog     
  while(1){  
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
        if (minuteB >= 1) // normaly minuteB == 10 
          pref.putInt(minbomb, minuteB);
        else if (minuteB == 20)
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
    sprintf(hourmeter1, "%02d:%02d:%02d", hourmeterB, 
                                      minuteB, secB);
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

    esp_task_wdt_reset();        // reset watchdog if dont return any error
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
} 




void readpref(){
  minuteB     = pref.getInt(minbomb, minuteB);
  minute      = pref.getInt(minpref, minute);
  hourmeter   = pref.getInt(hourpref, hourmeter); 
  hourmeterT  = pref.getInt(hourtrac, hourmeterT);
  hourmeterB  = pref.getInt(hourbomb, hourmeterB);
  manup       = pref.getInt(prevpref, manup);
  manuc       = pref.getInt(correpref, manuc);
}







































