/*-----------------------------------------------------------------

  P.O. Prototype V0.0.1 main.cpp
     
  INA226 + Shunt
  MFRC522 RFID
  Keypad 4x4
  WiFi
  LCD 20x4 display

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
#include "Keypad_I2C.h"
#include "Password.h"
#include "rtc_wdt.h"
#include "Arduino.h"
#include "MFRC522.h"
#include "INA226.h"
#include "Keypad.h"
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
void apx();
void eng();
void status();
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
bool preve;
bool corre;
byte currentlenlen = 0;
byte currentpasslen = 0;
bool passvalue = true;
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// --Preferences Key---
byte a = 7;
byte b = 5;
byte maxtaglen = 6;
byte maxpasslen = 5;
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

/*------------------------------------------------------------------
--------------------------------Tasks-------------------------------
------------------------------------------------------------------*/

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

  minute = pref.getInt(minpref, minute);
  hourmeter = pref.getInt(hourpref, hourmeter); 
  hourmeterT = pref.getInt(hourtrac, hourmeterT);
  hourmeterB = pref.getInt(hourbomb, hourmeterB);

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

    client.publish(topic_A,     Ageral);
    client.publish(topic_A2,    CABombH);
    client.publish(topic_A3,    CATrac);
    client.publish(topic_T,     CVolt);
    client.publish(topic_HBomb, JusthourB);
    client.publish(topic_Htrac, JusthourT);
    client.publish(topic_FHour, FullHour);
    client.publish(topic_JHour, Justhour);

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

    if (WiFi.status() != WL_CONNECTED  || !client.connected())
      recon();

    client.loop();

    char menu = kpd.getKey();

    if (preve == 1)
      status();
    else if (corre == 1)
      status();    
    else{
      if (menu != NO_KEY){
        if (menu == '0') {
          engnav = true;
          opnav  = false;
          eng();
        }else{
          engnav = false;
          opnav  = true;
          apx();
        }
      }
    }
    esp_task_wdt_reset();            // reset watchdog if dont return any error
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
}


/*-------------------------------------------------------------------
--------------------------------FUNCTIONS----------------------------
-------------------------------------------------------------------*/


// -----------------------------------------------------------------
// -----Reconection-----
void recon(){
  WiFi.disconnect();        
  vTaskDelay(1000);         
  WiFi.begin(ssid, pass);   
  vTaskDelay(1000);         
    
  vTaskDelay(500);
                
  client.connect("TestClient", user, passwd);     
  vTaskDelay(5000);                                  
        
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

// -----------------------------------------------------------------
// -----Screens-----
void screens(){
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("ESCOLHA A OPCAO:");
  lcd.setCursor(0, 1);
  lcd.print("1- CADASTRAR");
  lcd.setCursor(0, 2);
  lcd.print("2- EXCLUIR");
  lcd.setCursor(0, 3);
  lcd.print("#- SAIR");

  while (opnav == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        opnav = true;
        cadastrar();
      } else if (key == '2') {
        opnav = true;
        excluir();
      } else if (key == '#') {
        apx();
      }
    }
  }
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----telafinal-----
void telafinal() {

  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("NMS-V0.3");
  lcd.setCursor(5, 1);
  lcd.print("GREENTECH");
  lcd.setCursor(3, 2);
  lcd.print("OPERANDO");
  lcd.setCursor(0, 3);
  lcd.print("SHOWROOM-SP");
  vTaskDelay(1500);
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----status-----
void status() {

  lcd.clear();

  while (1) {

    lcd.setCursor(6, 0);
    lcd.print("MAQUINA");
    lcd.setCursor(9, 1);
    lcd.print("EM");
    lcd.setCursor(5, 2);
    lcd.print("MANUTENCAO");

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      char SAIR[32];
      snprintf(SAIR, sizeof(SAIR), "%02X%02X%02X%02X",
               rfid.uid.uidByte[0], rfid.uid.uidByte[1],
               rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
      client.publish(topic_user, SAIR);

      if (strcmp(SAIR, TecCard.c_str()) == 0) {
        corre = 0;
        preve = 0;
        prefs.putInt(correpref, corre);
        prefs.putInt(prevpref, preve);
        client.publish(topic_TEC, " ");
        navcheck = true;
        vTaskDelay(80);
        apx();
      }
    } else if (!rfid.PICC_IsNewCardPresent() && !rfid.PICC_ReadCardSerial()) {

      if (preve == 1) {
        client.publish(topic_TEC, "Preventiva");
        lcd.setCursor(5, 3);
        lcd.print("PREVENTIVA");
        vTaskDelay(40);
      } else if (corre == 1) {
        client.publish(topic_TEC, "Corretiva");
        lcd.setCursor(5, 3);
        lcd.print("CORRETIVA");
        vTaskDelay(40);
      }
    }
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----manutencao-----
void manutencao() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("ESCOLHA A OPCAO:");
  lcd.setCursor(0, 1);
  lcd.print("1- MANU PREVENTIVA");
  lcd.setCursor(0, 2);
  lcd.print("2- MANU CORRETIVA");
  lcd.setCursor(14, 3);
  lcd.print("#-SAIR");
  vTaskDelay(80);

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        preve = 1;
        vTaskDelay(30);
        prefs.putInt(prevpref, preve);
        status();
      } else if (key == '2') {
        corre = 1;
        vTaskDelay(30);
        prefs.putInt(correpref, corre);
        status();
      } else if (key == '#') {
        corre = 0;
        preve = 0;
        vTaskDelay(30);
        prefs.putInt(correpref, corre);
        prefs.putInt(prevpref, preve);
        apx();
      }
    }
  }
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----vazamento-----
void vazamento() {

  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("VAZAMENTO?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      delay(20);
      if (key == '1') {
        client.publish(topic_V, "True");
        garfos();
      } else if (key == '2') {
        client.publish(topic_V, "False");
        garfos();
      }
    }
  }
}    
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----garfos-----
void garfos() {

  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("GARFOS?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      delay(20);
      if (key == '1') {
        client.publish(topic_G, "True");
        emergencia();
      } else if (key == '2') {
        client.publish(topic_G, "False");
        emergencia();
      }
    }
  }
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----emergencia-----
void emergencia() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BOTAO DE EMERGENCIA?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      delay(20);
      if (key == '1') {
        client.publish(topic_E, "True");
        comando();
      } else if (key == '2') {
        client.publish(topic_E, "False");
        comando();
      }
    }
  }
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----comando-----
void comando() {

  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("FRENTE E RE?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      delay(20);
      if (key == '1') {
        client.publish(topic_F, "True");
        bateria();
      } else if (key == '2') {
        client.publish(topic_F, "False");
        bateria();
      }
    }
  }
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----bateria-----
void bateria() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BATERIA,CABOS,CONEC?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      delay(20);
      if (key == '1') {
        client.publish(topic_B, "True");
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("CONCLUIDO");
        lcd.setCursor(2, 2);
        lcd.print("MAQUINA LIBERADA");
        delay(1000);
        navcheck = true;  // "Mudar futuramente de 'true' para 'false' quando for para tela de funcionando", deixando true ele não zera todos para false após repetir o checklist novamente
        apx();
      } else if (key == '2') {
        client.publish(topic_B, "False");
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("CONCLUIDO");
        lcd.setCursor(2, 2);
        lcd.print("MAQUINA LIBERADA");
        delay(1000);
        navcheck = true;  // "Mudar futuramente de 'true' para 'false' quando for para tela de funcionando", deixando true ele não zera todos para false após repetir o checklist novamente
        apx();
      }
    }
    corre = 0;
    preve = 0;
    delay(50);
    prefs.putInt(correpref, corre);
    prefs.putInt(prevpref, preve);
  }
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----apx-----
void apx() {

  lcd.clear();
  lcd.setCursor(2, 2);
  lcd.print("APROXIMAR CARTAO");

  while (navcheck == true) {

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(uid_buffer, sizeof(uid_buffer), "%02X%02X%02X%02X",
               rfid.uid.uidByte[0], rfid.uid.uidByte[1],
               rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

      client.publish(topic_user, uid_buffer);

      if (strcmp(uid_buffer, OpeCard.c_str()) == 0) {
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("OPERADOR");
        lcd.setCursor(4, 2);
        lcd.print("IDENTIFICADO");
        vTaskDelay(1000);
        vazamento();
      } else if (strcmp(uid_buffer, TecCard.c_str()) == 0) {
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("TECNICO");
        lcd.setCursor(4, 2);
        lcd.print("IDENTIFICADO");
        vTaskDelay(1000);
        manutencao();
      } else if (strcmp(uid_buffer, AdmCard.c_str()) == 0) {
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("ADMINISTRADOR");
        lcd.setCursor(4, 2);
        lcd.print("IDENTIFICADO");
        vTaskDelay(1000);
        eng();
      }
    }
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----manutencao-----
void manutencao() {

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("ESCOLHA A OPCAO:");
  lcd.setCursor(0, 1);
  lcd.print("1- MANU PREVENTIVA");
  lcd.setCursor(0, 2);
  lcd.print("2- MANU CORRETIVA");
  lcd.setCursor(14, 3);
  lcd.print("#-SAIR");
  delay(80);

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      delay(20);
      if (key == '1') {
        preve = 1;
        delay(30);
        prefs.putInt(prevpref, preve);
        status();
      } else if (key == '2') {
        corre = 1;
        delay(30);
        prefs.putInt(correpref, corre);
        status();
      } else if (key == '#') {
        corre = 0;
        preve = 0;
        delay(30);
        prefs.putInt(correpref, corre);
        prefs.putInt(prevpref, preve);
        apx();
      }
    }
  }
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----status-----
void status() {

  lcd.clear();

  while (1) {

    lcd.setCursor(6, 0);
    lcd.print("MAQUINA");
    lcd.setCursor(9, 1);
    lcd.print("EM");
    lcd.setCursor(5, 2);
    lcd.print("MANUTENCAO");

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      char SAIR[32];
      snprintf(SAIR, sizeof(SAIR), "%02X%02X%02X%02X",
               rfid.uid.uidByte[0], rfid.uid.uidByte[1],
               rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
      client.publish(topic_RFID, SAIR);

      if (strcmp(SAIR, TecCard.c_str()) == 0) {
        corre = 0;
        preve = 0;
        prefs.putInt(correpref, corre);
        prefs.putInt(prevpref, preve);
        client.publish(topic_TEC, " ");
        navcheck = true;
        delay(80);
        apx();
      }
    } else if (!rfid.PICC_IsNewCardPresent() && !rfid.PICC_ReadCardSerial()) {

      if (preve == 1) {
        client.publish(topic_TEC, "Preventiva");
        lcd.setCursor(5, 3);
        lcd.print("PREVENTIVA");
        delay(40);
      } else if (corre == 1) {
        client.publish(topic_TEC, "Corretiva");
        lcd.setCursor(5, 3);
        lcd.print("CORRETIVA");
        delay(40);
      }
    }
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----dell-----
void dell() {

  password.reset();
  currentPasswordLength = 0;
  lcd.clear();
  b = 5;

  if (acertou != true) {
    cadastrar();
  }
}
// -----------------------------------------------------------------
// -----tag-----
void tag(char key) {

  lcd.setCursor(b, 1);
  lcd.print(key);
  b++;

  if (b == 11) {
    b = 5;  // Tamanho da TAG com 6 digitos "333..."
  }
  PassLenghtAtual++;
  password.append(key);
}
// -----------------------------------------------------------------
// -----excluir-----
void excluir() {

  lcd.clear();
  lcd.setCursor(1, 2);
  lcd.print("APROXIME O CARTAO");

  while (1) {

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(uid_buffer, sizeof(uid_buffer), "%02X%02X%02X%02X",
               rfid.uid.uidByte[0], rfid.uid.uidByte[1],
               rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

      client.publish(topic_CAD, uid_buffer);

      if (strcmp(uid_buffer, PesCard.c_str()) == 0) {
        lcd.clear();
        lcd.setCursor(7, 2);
        lcd.print("APAGADO");
        vTaskDelay(1000);
        PesCard = "";
        if (navcheck == true) {
          screens();
        } else {
          telas();
        }
      } else {
        lcd.clear();
        lcd.setCursor(5, 1);
        lcd.print("CARTAO NAO");
        lcd.setCursor(5, 2);
        lcd.print("CADASTRADO");
        vTaskDelay(1000);
        if (navcheck == true) {
          screens();
        } else {
          telas();
        }
      }
    }
  }
}
// -----------------------------------------------------------------
// -----formatar-----
void formatar() {

  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("FORMARTAR?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (1) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        lcd.clear();
        lcd.setCursor(5, 2);
        lcd.print("FORMATADO");
        vTaskDelay(1000);
        telas();
      } else if (key == '2') {
        telas();
      }
    }  // João
  }
}
// -----------------------------------------------------------------
// -----cadastrar-----
void cadastrar() {

  lcd.clear();
  lcd.setCursor(1, 1);
  lcd.print("TAG:");
  lcd.setCursor(1, 2);
  lcd.print("RFID:");
  lcd.setCursor(14, 3);
  lcd.print("#-SAIR");
  acertou = false;

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == 'C') {
        dell();
      } else if (key == '#') {
        b = 5;
        screens();
      } else {
        tag(key);
      }
    }
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(CAD, sizeof(CAD), "%02X%02X%02X%02X",
               rfid.uid.uidByte[0], rfid.uid.uidByte[1],
               rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

      b = 5;  // Sempre que bater o RFID a proxima TAG volta ser digitada na posição 5
      client.publish(topic_CAD, "");
      lcd.setCursor(6, 2);
      lcd.print(CAD);
      vTaskDelay(1000);
      cadastrar();
    }
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
// -----------------------------------------------------------------
// -----CadastrarCartao-----
void CadastrarCartao() {

  String conteudo = "";

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    snprintf(CAD, sizeof(CAD), "%02X%02X%02X%02X",
             rfid.uid.uidByte[0], rfid.uid.uidByte[1],
             rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
    conteudo.concat(CAD);  // Juntando os valores do CAD no conteudo
    conteudo = conteudo + ";";
    cartoesCadastrados = cartoesCadastrados + conteudo;


    prefs.putString(listapref, cartoesCadastrados);
  }
}
// -----------------------------------------------------------------
// -----telas-----
void telas() {

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("ESCOLHA A OPCAO:");
  lcd.setCursor(0, 1);
  lcd.print("1- FORMATAR");
  lcd.setCursor(0, 2);
  lcd.print("2- EXCLUIR");
  lcd.setCursor(0, 3);
  lcd.print("#- SAIR");

  while (1) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        formatar();
      } else if (key == '2') {
        excluir();
      } else if (key == '#') {
        navcheck = true;  // True, após apertar "#" ele habilita a opção para bater os RFIDs novamente para seguir com outras opções.
        apx();
      }
    }
  }
}
// -----------------------------------------------------------------
// -----resetPassword-----
void resetPassword() {

  password.reset();
  currentPasswordLength = 0;
  lcd.clear();
  a = 7;

  if (acertou != true) {
    eng();  // Mostra que errou a senha, apaga a senha sem sumir a mensagem "Password"
  }
}
// -----------------------------------------------------------------
// -----aprovadoPass-----
void aprovadoPass() {

  currentPasswordLength = 0;

  if (password.evaluate()) {
    lcd.clear();
    lcd.setCursor(7, 2);
    lcd.print("VALIDO");
    vTaskDelay(1000);
    a = 7;
    acertou = true;  // mostra que acertou, apaga a mensagem anterior e segue para a tela screens ou  telas
    if (navcheck == true) {
      screens();
    } else {
      telas();
    }
  } else {
    lcd.clear();
    lcd.setCursor(6, 1);
    lcd.print("INVALIDO");
    lcd.setCursor(2, 2);
    lcd.print("TENTE NOVAMENTE");
    vTaskDelay(1000);
    a = 7;
    acertou = false;  // mostra que acertou, apaga a mensagem anterior e  volta para a tela de Password e colocar a senha correta
  }
  resetPassword();
}
// -----------------------------------------------------------------
// -----processNumberKey-----
void processNumberKey(char key) {

  lcd.setCursor(a, 2);
  lcd.print("*");
  a++;
  if (a == 11) {
    a = 4;  // Tamanho da senha com 4 digitos "2552"
  }
  currentPasswordLength++;
  password.append(key);

  if (currentPasswordLength == maxPasswordLength) {
    aprovadoPass();
  }
}
// -----------------------------------------------------------------
// -----eng-----
void eng() {

  lcd.clear();
  lcd.setCursor(5, 1);
  lcd.print("PASSWORD:");
  lcd.setCursor(14, 3);
  lcd.print("#-SAIR");

  while (1) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == 'C') {
        resetPassword();
      } else if (key == '#') {
        apx();
      } else if (key == 'D') {
        if (value == true) {
          aprovadoPass();
          value = false;
        }
      } else {
        processNumberKey(key);
      }
    }
  }
}
// -----------------------------------------------------------------