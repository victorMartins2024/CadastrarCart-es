/*-------------------------------------------------------------

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

------------------------------------------------------------*/

//------------------------Libraries---------------------------
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "PubSubClient.h"
#include "esp_task_wdt.h"
#include "Preferences.h"
#include "rtc_wdt.h"
#include "Arduino.h"
#include "heltec.h"
#include "WiFi.h"
// -----------------------------------------------------------

//------------------------pins--------------------------------
#define   BOARD_LED   2
#define   pinA       36 

// ----------------------------------------------------------- 

// --------------------connection infos-----------------------
const char *ssid    =    "Viaduto_Administrativo";             
const char *pass    =    "Mcpnet@2022*";   
const char *mqtt    =    "10.244.20.104";            
const char *user    =    "greentech";                           
const char *passwd  =    "Greentech@01";                       
      int   port    =     1883;    
// ----------------------------------------------------------
                                 

// ---------------------Topics--------------------------------
const char *topic_JHour =   "teste/sinal/sim/telemetria/ApenasHora";
const char *topic_FHour =   "teste/sinal/sim/telemetria/HoraTotal";
const char *topic_A2    =   "teste/sinal/sim/telemetria/elevacao";
const char *topic_A3    =   "teste/sinal/sim/telemetria/tracao";
const char *topic_A     =   "teste/sinal/sim/telemetria/geralA";           
const char *topic_T     =   "teste/sinal/sim/telemetria/tensao";                
// -----------------------------------------------------------


// -------------------------Variables-------------------------
float   geralA;        
float   Asinal;   
float   ABombH;
float   ATrac;
float   Volt;           
char    Ageral[30]; 
char    CABombH[30];
char    CATrac[30];
char    CVolt[30];            
char    Justhour[30];    
char    FullHour[30];    
char    justMin[30];     
int     hourmeter;       
int     sec;             
int     minute;           
int     pwm;   
// ----------------------------------------------------------         
  
// ------------------------Objects---------------------------    
Preferences pref;
WiFiClient ESP32ClientJ;       
PubSubClient client(ESP32ClientJ); 
// ----------------------------------------------------------    

// ------------------Preferences Key------------------------- 
const char* minpref= "min";
const char* hourpref= "hour";
// ---------------------------------------------------------- 


// tasks
void xTaskPublish(void *pvParameters);
void xTaskHoumeter(void *pvParameters);
void xTaskConvertion(void *pvParameters);

// functions
void reconB();
void reconW();

extern "C" void app_main(){
  initArduino();
  Serial.begin(115200);
  pref.begin("GT", false);
  WiFi.begin(ssid, pass);

  Heltec.begin(true, false, true); /*start the heltec (enable dislpay, disable lora,  enable serial debug,
                                                        anable paboost (only if lora is enable),
                                                        band( only if lora is enable))*/ 
  Heltec.display->init();
  
  pinMode(pinA, INPUT);
  pinMode(BOARD_LED, OUTPUT);

  Heltec.display->clear();                                 //  -----------------------
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);       // 
  Heltec.display->setFont(ArialMT_Plain_10);               //     Display config
  Heltec.display->display();                               // 
  //Heltec.display->flipScreenVertically();                  //  -----------------------

  while(WiFi.status() != WL_CONNECTED){                    // ------------------------
    Heltec.display->drawString(0, 0, "Wifi desconneted");   //         
    delay(2500);                                           //     connect to wifi
  }                                                        // ------------------------

  client.setServer(mqtt, port);                            // ------------------------
  while (!client.connected()){                             //
    client.connect("ESP32ClientJ", user, passwd);           //     connect to broker
    delay(500);                                            //
  }                                                        // ------------------------
                       // ------------------------
  
  xTaskCreatePinnedToCore(xTaskHoumeter, // function name
                          "hourmeter",   // task name
                          15000,         // stack size in byte (in esp32 framework)
                          NULL,          // input parameter to pass
                          2,             // priority
                          NULL,          // task handle
                          1);            // core

  xTaskCreatePinnedToCore(xTaskConvertion,
                          "Conversion",
                          2000,
                          NULL,
                          3,
                          NULL,
                          1);

  xTaskCreatePinnedToCore(xTaskPublish,
                          "Publish",
                          10000,
                          NULL,
                          1,
                          NULL,
                          1);
} 

/*---------------------------------------
-----------------TASKS-------------------
---------------------------------------*/

void xTaskHoumeter(void *pvParameters){
  minute = pref.getInt(minpref, minute);  
  hourmeter = pref.getInt(hourpref, hourmeter);
  
  esp_task_wdt_add(NULL);                         //  enable watchdog    
  while (1){
    rtc_wdt_feed();                               //    feed watchdog 
    if (geralA >= 50){                            
      sec++;                                     
      if(sec>= 60){                              
        sec-=60;                             
        minute++;  
        
        if(minute == 10)
          pref.putInt(minpref, minute);
        else if(minute == 20)
          pref.putInt(minpref, minute);
        else if(minute == 30)
          pref.putInt(minpref, minute);
        else if (minute == 40)
          pref.putInt(minpref, minute);
        else if (minute == 50)
          pref.putInt(minpref, minute);                                         
      }                                     
      if (minute >= 60){                        
        minute -= 60;                           
        hourmeter++;                             
        pref.putInt(minpref, minute);
        pref.putInt(hourpref, hourmeter); 
      }                                          
    }

    /* if(digitalRead(PINBOTTON) == LOW){       //  ----------------------      
      minute  = 0;                           //
      hourmeter = 0;                         //      Reset Hourmeter
      pref.putInt(minpref, minute);          //
      pref.putInt(hourpref, hourmeter);      //    
    }                                        //  ---------------------- */

    esp_task_wdt_reset();                                   // reset watchdog if dont return any erro

    Heltec.display->clear();                                                           
    Heltec.display->drawString(0, 0, "hourmeter: ");        // -----------------------------------------------               
    Heltec.display->drawString(65, 0, FullHour);            //                Display counter
    Heltec.display->display();                              // -----------------------------------------------

    vTaskDelay(1000 / portTICK_PERIOD_MS);                                       //(1000 = 1s / 60000 = 1min / 3600000 = 1h)
  }
}

void xTaskConvertion(void *pvParameters){
  esp_task_wdt_add(NULL);
  while (1){
    rtc_wdt_feed();
    Asinal = (analogRead(pinA));
    geralA = Asinal / 4095 * 400;
    
    Volt = Asinal / 4095 * 48;
    ABombH = geralA - 72.671;
    ATrac = geralA - 53.555;

    pwm = map(Asinal, 0, 1023, 0, 400);
    analogWrite(25, pwm);

    esp_task_wdt_reset();

    sprintf(FullHour, "{\"hourmeter\": %02d:%02d:%02d}",    
                                hourmeter, minute, sec);    
    sprintf(CABombH,  "{\"Corrente Bomba\": %f", ABombH);
    sprintf(Ageral,   "{\"Corrente geral\": %f", geralA); 
    sprintf(CVolt,    "{\"Voltagem geral\": %f", Volt);
    sprintf(CATrac,   "{\"Corrente trac\": %f", ATrac);
    sprintf(Justhour, "{\"JustHour\":%d", hourmeter); 
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void xTaskPublish(void *pvParameters){
  esp_task_wdt_add(NULL);
  while (1){
    if (WiFi.status() != WL_CONNECTED){            //   -----------------------------------------------------
      reconW();                                    //      if the esp lost conection with    
      vTaskDelay(500 / portTICK_PERIOD_MS);        //      wifi and/or broke this section will
    }else if (!client.connected()){                //      take care of reconnet to wifi and/or broker
      reconB();                                    //
      vTaskDelay(500 / portTICK_PERIOD_MS);        //   ------------------------------------------------------
    }   

    rtc_wdt_feed();

    client.publish(topic_A,     Ageral);
    client.publish(topic_A2,    CABombH);
    client.publish(topic_A3,    CATrac);
    client.publish(topic_T,     CVolt);
    client.publish(topic_FHour, FullHour);
    client.publish(topic_JHour, Justhour);

    client.loop();

    esp_task_wdt_reset();

    //vPortFree();

    vTaskDelay(1500/ portTICK_PERIOD_MS);
  }
}

/*---------------------------------------
----------------FUNCTIONS----------------
---------------------------------------*/

void reconW(){
  while (WiFi.status() != WL_CONNECTED){
    WiFi.disconnect();      // disconnect from wifi
    vTaskDelay(500 / portTICK_PERIOD_MS);       // wait 1 millisec
    WiFi.begin(ssid, pass); // reconnect to wifi
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void reconB(){
  while (!client.connected()){
    client.connect("ESP32ClientJ", user, passwd);
    client.loop();    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}