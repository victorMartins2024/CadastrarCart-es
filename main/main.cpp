/*-----------------------------------------------------------------

  Hourmeter V0.2 main.cpp

  MQtt
  Flash memory
  Hourmeter counter      
  Ampere reading
  Display OLED

  Compiler: VsCode 1.88.1
  MCU: ESP32 DEV MODULE

  Author: João  G. Uzêda
  date: 2024, March

-----------------------------------------------------------------*/

// ----------------------------------------------------------------- 
// ---Libraries---
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "LiquidCrystal_I2C.h"
#include "freertos/task.h"
#include "PubSubClient.h"
#include "esp_task_wdt.h"
#include "rtc_wdt.h"
#include "Arduino.h"
#include "MFRC522.h"
#include "INA226.h"
#include "WiFi.h"
#include "Wire.h"
#include "SPI.h"
// -----------------------------------------------------------------  
#define SHUNT_RESISTENCE 1
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
int         port    =     1883;    
// -----------------------------------------------------------------

// -----------------------------------------------------------------  
// -----Topics-----
const char *topic_JHour   =    "teste/proto/ApenasHora";
const char *topic_FHour   =    "teste/proto/HoraTotal";
const char *topic_A2      =    "teste/proto/elevacao";
const char *topic_A3      =    "teste/proto/tracao";
const char *topic_A       =    "teste/proto/geralA";
const char *topic_T       =    "teste/proto/tensao";  
const char *Topic_user    =    "teste/proto/rfid";  
// -----------------------------------------------------------------

void reconW();
void reconB();
void telemetry();

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
float ABombH;
float ATrac;
int hourmeter;
int sec;
int minute;
int pwm;
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// UIDS
String card = "E6A1191E";
String battery2 = "F4E0A08D";

// -----------------------------------------------------------------
// buffers 
char uid_buffer[32];

extern "C" void app_main(){
  initArduino();
  INA.init();
  Wire.begin(SDA, SCL);
  
  SPI.begin(SCK, MISO, MOSI, RST);
  rfid.PCD_Init();
  WiFi.begin(ssid, pass); 
  lcd.init();
  lcd.backlight();

  INA.setResistorRange(SHUNT_RESISTENCE,300.0); 
  INA.setConversionTime(CONV_TIME_1100); 
  INA.setAverage(AVERAGE_4);
    

  while(WiFi.status() != WL_CONNECTED);                                                                                               

  client.setServer(mqtt, port);           
  while (!client.connected()){                            
    client.connect("ESP32ClientU", user, passwd);    
    delay(500);                                         
  }  

  INA.waitUntilConversionCompleted();

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

    if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial() ){     
    lcd.clear();
    snprintf(uid_buffer, sizeof(uid_buffer), "%02X%02X%02X%02X",
                       rfid.uid.uidByte[0], rfid.uid.uidByte[1],
                      rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

      client.publish(Topic_user, uid_buffer);

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
    telemetry();
    delay(1000);
  }    
}


/*---------------------------------------
----------------FUNCTIONS----------------
---------------------------------------*/

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
  
  float Volt = INA.getShuntVoltage_mV();
  float geralA = Volt / SHUNT_RESISTENCE;

  ABombH = geralA - 72.671;
  ATrac = geralA - 53.555;

  if (geralA >= 50){
    sec++;
  if (sec >= 60){
    sec -= 60;
    minute++;
    }
      if (minute >= 60){
      minute -= 60;
      hourmeter++;
    }
  }

  sprintf(CABombH,  "{\"Corrente Bomba\": %.02f}", ABombH);
  sprintf(Ageral,   "{\"Corrente geral\": %.02f}", geralA);
  sprintf(CATrac,   "{\"Corrente tração\": %.02f}", ATrac);
  sprintf(CVolt,    "{\"Tensão geral\": %.02f}", Volt);
  sprintf(FullHour, "{\"hourmeter\": %02d:%02d:%02d}",
                              hourmeter, minute, sec);
  sprintf(Justhour, "{\"JustHour\":%d}", hourmeter);
    
  lcd.setCursor(0, 0);
  lcd.print("H:");
  lcd.setCursor(3, 0);
  lcd.print(hourmeter);

  lcd.setCursor(0, 1);
  lcd.print("A:");
  lcd.setCursor(3, 1);
  lcd.print(geralA);

  lcd.setCursor(10, 1);
  lcd.print("V:");
  lcd.setCursor(13, 1);
  lcd.print(Volt);

  client.publish(topic_A, Ageral);
  client.publish(topic_A2, CABombH);
  client.publish(topic_A3, CATrac);
  client.publish(topic_T, CVolt);
  client.publish(topic_FHour, FullHour);
  client.publish(topic_JHour, Justhour);

  client.loop();
} 