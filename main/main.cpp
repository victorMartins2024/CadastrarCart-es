/*-----------------------------------------------------------------

  Hourmeter V0.21 main.cpp

  MQtt
  Flash memory
  Hourmeter counter      
  Ampere reading
  Display OLED

  Compiler: VsCode 1.88.1
  MCU: Heltec ESP32 LoraWan v2

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
#include "WiFi.h"
#include "Wire.h"
#include "SPI.h"
// -----------------------------------------------------------------  

// -----------------------------------------------------------------  
//----I²C Adresses------
#define LCDADRESS 0x27 
// -----------------------------------------------------------------  

// -----------------------------------------------------------------  
//----pins------
#define   SS    22 
#define   RST   21
#define   SCK   14
#define   MISO  2
#define   MOSI  15
#define   SDA   1
#define   SCL   3     
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
// --Topics-----
const char *Topic_RFID =   "teste/proto/rfid";    
// -----------------------------------------------------------------

void reconW();
void reconB();

// -----------------------------------------------------------------
// ------------------------Objects--------------------------- 
WiFiClient TestClient;
MFRC522 rfid(SS, RST);
PubSubClient client(TestClient);
LiquidCrystal_I2C lcd(LCDADRESS, 16, 2);
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
  Wire.begin(SDA, SCL, 9600);
  SPI.begin(SCK, MISO, MOSI, RST);
  rfid.PCD_Init();
  WiFi.begin(ssid, pass); 
  lcd.init();
  lcd.backlight();

  while(WiFi.status() != WL_CONNECTED);                                                                                               

  client.setServer(mqtt, port);           
  while (!client.connected()){                            
    client.connect("ESP32ClientU", user, passwd);    
    delay(500);                                         
  }  

  lcd.clear();
  lcd.home();
  lcd.print("INICIALIZANDO");
  delay(90);   

  while(1){
    client.loop();
    if (WiFi.status() != WL_CONNECTED){             //   -----------------------------------------------------
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
      client.publish(Topic_RFID, uid_buffer);
      lcd.print(uid_buffer);


      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
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
