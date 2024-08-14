/*----------------------------------------------------------------

  Telemetry V0.9.0 main.cpp
     
  INA226
  MFRC522 
  LCD 20x4 I²c 
  Keypad 4x4 I²c 

  Compiler: VsCode 1.92
  MCU: ESP32
  Board: Dev module 38 pins

  Author: João  G. Uzêda & Victor Hugo
  date: 2024, June
-----------------------------------------------------------------*/

// ---------------------------------------------------------------- 
// ---Libraries---
#include "freertos/FreeRTOSConfig.h"
#include "LiquidCrystal_I2C.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "Preferences.h"
#include "esp_system.h"
#include "I2CKeyPad.h"
#include "Password.h"
#include "rtc_wdt.h"
#include "Arduino.h"
#include "MFRC522.h"
#include "INA226.h"
#include "stdio.h"
#include "Wire.h"
#include "LoRa.h"
#include "SPI.h"

// ---------------------------------------------------------------- 
// ---LoRa Settings---
#define   SF            9
#define   FREQ          915E6
#define   SBW           62.5E3	
#define   SYNC_WORD     0x34
#define   TX_POWER_DB   17

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
#define   RFID_SS       22
#define   RFID_RST      5
#define   RFID_SCK      14
#define   RFID_MISO     2
#define   RFID_MOSI     15
#define   INA_SDA       32
#define   INA_SCL       33 
#define   LORA_SS       19  
#define   LORA_RST      18 
#define   LORA_SCK      25 
#define   LORA_MISO     26 
#define   LORA_MOSI     27

// ----------------------------------------------------------------
// -----Functions----
void xTaskTelemetry(void *pvParameters);
void xTaskSend(void *pvParameters);
void xTaskNav(void *pvParameters);
void erease(char key, int buffer);
void tag(char key, int buffer);
void CadastrarCartao();
void dell(int buffer);
void resetPassword();
void aprovadoPass();
void ina226_setup();
void emergencia();
void manutencao();
void telafinal();
void cadastrar();
void vazamento();
void hourcheck();
void readpref();
void formatar();
void screens();
void bateria();
void comando();
void excluir();
void status();
void garfos();
void format();
void telas();
void input();
void eng();
void apx();

// ----------------------------------------------------------------
// -----Objects----
Preferences pref;
SPIClass *vspi = NULL;
SPIClass *hspi = NULL;
INA226_WE INA(INAADRESS);
I2CKeyPad kpd(KeyAdress);
MFRC522 rfid(SS, RFID_RST);
Password password = Password("2552");
LiquidCrystal_I2C lcd(LCDADRESS, 20, 4);

// ----------------------------------------------------------------
// ----Variables----
float General_Current;
float Current_Hidraulic_Bomb;
float Current_Traction_Engine;
float Voltage;

byte General_Sec;
byte Sec_Traction_Engine;
byte Sec_Hidraulic_Bomb;
byte General_Minute;
byte Minute_Traction_Engine;
byte Minute_Hidraulic_Bomb;
int General_Hourmeter;
int Hourmeter_Traction_Engine;
int Hourmeter_Hidraulic_Bomb;

byte Answer_1;
byte Answer_2;
byte Answer_3;
byte Answer_4;
byte Answer_5;

// ----------------------------------------------------------------
// ----Constants----
byte a = 7;
byte b = 5;
byte c = 10;
byte maxtaglen = 6; //PassLenghtMax
byte maxpasslen = 5;  //maxPasswordLength
byte priority = 0;

// ----------------------------------------------------------------
// ----Buffers----
byte currentpasslen = 0;   //currentPasswordLength
byte currenttaglen = 0; //PassLenghtAtual
char uid_buffer[32];
char CAD[32];
byte manup =  0; //preve
byte manuc =  0; //corre
bool opnav;
bool psswdcheck;

// ----------------------------------------------------------------
// --Preferences Key---
const char *Minute_preference               =   "min";
const char *Hourmeter_Preference            =   "hour";
const char *Minute_Engine_Preference        =   "trac";
const char *cadaspref                       =   "Cadastro";
const char *prevpref                        =   "Manupreve";
const char *correpref                       =   "Manucorre";
const char *listapref                       =   "Cadastro Cartoes";
const char *Hourmeter_Engine_Preference     =   "Hourmeter_Engine_Preference";
const char *Minute_Hidraulic_Preference     =   "Minute_Hidraulic_Preference";
const char *Hourmeter_Hidraulic_Preference  =   "Hourmeter_Hidraulic_Preference";

// ----------------------------------------------------------------
// -----UIDS-----
String OpeCard  =   "6379CF9A"; //"E6A1191E"; 
String AdmCard  =   "29471BC2";  
String TecCard  =   "D2229A1B";  
String PesCard  =   "B2B4BF1B";
String UIDLists =   " ";
String TAGLists =   " ";

// ----------------------------------------------------------------
// ----main----
extern "C" void app_main(){
  initArduino();
  vspi = new SPIClass(VSPI);
  hspi = new SPIClass(HSPI);
  Wire.begin(INA_SDA, INA_SCL);
  vspi->begin(RFID_SCK, RFID_MISO, RFID_MOSI, RFID_RST);
  hspi->begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_RST);
  lcd.init();
  lcd.backlight();
  rfid.PCD_Init();
  ina226_setup();
  kpd.begin();
  pref.begin("GT", false);
  lcd.backlight();
  
  pinMode(RFID_SS, OUTPUT);
  pinMode(LORA_SS, OUTPUT);
  LoRa.setPins(LORA_SS, LORA_RST);
  kpd.setKeyPadMode(I2C_KEYPAD_4x4);
  kpd.loadKeyMap(keypad); 

  while(!LoRa.begin(FREQ))
    Serial.println("Failed to start LoRa");
  
  readpref();
    
  LoRa.setSyncWord(SYNC_WORD);              
  LoRa.setTxPower(TX_POWER_DB); 
  LoRa.setSignalBandwidth(SBW);
  LoRa.setSpreadingFactor(SF);   

  lcd.setCursor(3, 2);
  lcd.print("INICIALIZANDO");

  xTaskCreatePinnedToCore(xTaskTelemetry, // function name
                          "Telemetry",    // task name
                          2000,           // stack size in word
                          NULL,           // input parameter
                          1,              // priority
                          NULL,           // task handle
                          0);             // core

  xTaskCreatePinnedToCore(xTaskNav,
                          "Navegation",
                          4500,
                          NULL,
                          1,
                          NULL,
                          1);
  
  xTaskCreatePinnedToCore(xTaskSend,
                          "LoRa",
                          2000,
                          NULL,
                          2,
                          NULL,
                          1);
  
  vTaskDelay(2500);
  lcd.clear();             
} 

/*---------------------------------------------------------------------------------
-------------------------------------Tasks-----------------------------------------
---------------------------------------------------------------------------------*/

// -----------------------------------------------------------------
// -----telemetry-----
void xTaskTelemetry(void *pvParameters){ 
  esp_task_wdt_add(NULL);        //  enable watchdog     
  while(1){  
    rtc_wdt_feed();    //  feed watchdog 

    INA.readAndClearFlags();

    Voltage = INA.getBusVoltage_V();
    General_Current = INA.getShuntVoltage_mV() / SHUNT_RESISTENCE;

    Current_Hidraulic_Bomb = General_Current - 1.5;
    Current_Traction_Engine = General_Current - 2.5;

    if (General_Current >= 3.5){ // --------------General General_Hourmeter--------------------------
      General_Sec++;   
      if (General_Sec >= 60){         
        General_Sec -= 60;
        General_Minute++;
        if (General_Minute == 10)
          pref.putInt(Minute_preference, General_Minute);
        else if (General_Minute == 20)
          pref.putInt(Minute_preference, General_Minute);
        else if (General_Minute == 30)
          pref.putInt(Minute_preference, General_Minute);
        else if (General_Minute == 40)
          pref.putInt(Minute_preference, General_Minute);
        else if (General_Minute == 50)
          pref.putInt(Minute_preference, General_Minute);
      }                       
      if (General_Minute >= 60){
        General_Minute -= 60;
        General_Hourmeter++;
        priority = 1;
        pref.remove(Minute_preference);
        pref.putInt(Hourmeter_Preference, General_Hourmeter);
      }                     
    } // -----------------------------------------------------------------------   
    if (Current_Hidraulic_Bomb >= 13){  // --------Hidraulic bomb General_Hourmeter----------------------
      Sec_Hidraulic_Bomb++;
      if (Sec_Hidraulic_Bomb >= 60){        
        Sec_Hidraulic_Bomb-=60;
        Minute_Hidraulic_Bomb++;
        if (Minute_Hidraulic_Bomb >= 10) // normaly Minute_Hidraulic_Bomb == 10 
          pref.putInt(Minute_Hidraulic_Preference, Minute_Hidraulic_Bomb);
        else if (Minute_Hidraulic_Bomb == 30)
          pref.putInt(Minute_Hidraulic_Preference, Minute_Hidraulic_Bomb);       
      }
      if (Minute_Hidraulic_Bomb >= 60){
        Minute_Hidraulic_Bomb-=60;
        Hourmeter_Hidraulic_Bomb++;
        priority = 1;
        pref.remove(Minute_Hidraulic_Preference);
        pref.putInt(Hourmeter_Hidraulic_Preference, Hourmeter_Hidraulic_Bomb);
      }
    } // --------------------------------------------------------------------------
    if (General_Current >= 13){  // ---------Trasion engine General_Hourmeter----------------------
      Sec_Traction_Engine++;
      if (Sec_Traction_Engine >= 60){        
        Sec_Traction_Engine-=60;
        Minute_Traction_Engine++;
        if (Minute_Traction_Engine == 20)
          pref.putInt(Minute_Engine_Preference, Minute_Traction_Engine);
        else if (Minute_Traction_Engine == 40)
          pref.putInt(Minute_Engine_Preference, Minute_Traction_Engine);
      }
      if (Minute_Traction_Engine >= 60){
        Minute_Traction_Engine-=60;
        Hourmeter_Traction_Engine++;
        priority = 1;
        pref.remove(Minute_Engine_Preference);
        pref.putInt(Hourmeter_Engine_Preference, Hourmeter_Traction_Engine);
      }
    } // -----------------------------------------------------------------------

    esp_task_wdt_reset();        // reset watchdog if dont return any error
    vTaskDelay(1000);
  }
} 

// -----------------------------------------------------------------
// -----Navegation task-----
void xTaskNav(void *pvParameters){
  esp_task_wdt_add(NULL);      //  enable watchdog
  while(1){
    rtc_wdt_feed();                  //  feed watchdog 
    
    lcd.noCursor();
    lcd.noBlink();

    char menu = kpd.getChar();
    vTaskDelay(90);

    if (manup == 1) {
      status();
    } else if (manuc == 1) {
      status();
    } else {
      if (menu != 'N') {
        vTaskDelay(20);
       if (menu == '0') {
          opnav = false;  // Para tela de Engenharia
          eng();
          vTaskDelay(80);
        }
      } else {
        opnav = true;  // Para tela de Operação
        apx();
      }
    }
    esp_task_wdt_reset();            // reset watchdog if dont return any error
    vTaskDelay(1500);
  }
}

// -----------------------------------------------------------------
// -----LoRa-----
void xTaskSend(void *pvParameters){   
  esp_task_wdt_add(NULL);
  while (1){
    rtc_wdt_feed();  

    if (priority == 0){
      LoRa.beginPacket();
      LoRa.print(Voltage);
      LoRa.print(Current_Hidraulic_Bomb);
      LoRa.print(Current_Traction_Engine);
      LoRa.print(General_Current);
      LoRa.endPacket();
      priority = 0;
    }else if (priority == 1){
      LoRa.beginPacket();
      LoRa.println(General_Hourmeter);
      LoRa.println(Hourmeter_Traction_Engine);
      LoRa.println(Hourmeter_Hidraulic_Bomb);
      LoRa.endPacket();
      priority = 0;
    }else if(priority == 2){
      LoRa.beginPacket();
      LoRa.print(uid_buffer);
      LoRa.endPacket();
      priority = 0;
    }else if (priority == 3){
      LoRa.begin();
      LoRa.print(Answer_1);
      LoRa.print(Answer_2);
      LoRa.print(Answer_3);
      LoRa.print(Answer_4);
      LoRa.print(Answer_5);
      LoRa.endPacket();
      priority = 0;
    }
    esp_task_wdt_reset(); 
    vTaskDelay(1000);
  }  
}

/*------------------------------------------------------------------------------------
--------------------------------------Functions---------------------------------------
------------------------------------------------------------------------------------*/

// -----------------------------------------------------------------
// -----read flash memory-----
void readpref(){
  Minute_Hidraulic_Bomb     = pref.getInt(Minute_Hidraulic_Preference, Minute_Hidraulic_Bomb);
  General_Minute      = pref.getInt(Minute_preference, General_Minute);
  General_Hourmeter   = pref.getInt(Hourmeter_Preference, General_Hourmeter); 
  Hourmeter_Traction_Engine  = pref.getInt(Hourmeter_Engine_Preference, Hourmeter_Traction_Engine);
  Hourmeter_Hidraulic_Bomb  = pref.getInt(Hourmeter_Hidraulic_Preference, Hourmeter_Hidraulic_Bomb);
  manup       = pref.getInt(prevpref, manup);
  manuc       = pref.getInt(correpref, manuc);
}

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
// -----dell-----
void dell(int buffer){
  lcd.clear();
  if(buffer == 1){
    currenttaglen = 0;
    b = 5;
    cadastrar();
  }else if (buffer == 2){
    c = 10;
    input();
  }
}

// -----------------------------------------------------------------
// -----tag-----
void tag(char key, int buffer){
  if (buffer == 1){
    lcd.setCursor(b, 1);
    lcd.print(key);
    b++;

    if (b == 11) 
      b = 5;  

    currenttaglen++;
    vTaskDelay(20); 

  }else if(buffer == 0){
    lcd.setCursor(a, 2);
    lcd.print("*");
    a++;

    if (a == 11) 
      a = 4; 
    
    currentpasslen++;
    password.append(key);

    if (currentpasslen == maxpasslen) 
      aprovadoPass();
    
  }else if(buffer == 2){
    lcd.setCursor(c, 0);
    lcd.print(key);
    c++;

    if (c == 16)
      c = 10;

    vTaskDelay(20); 
  }
}

// -----------------------------------------------------------------
// -----cadastro-----
void CadastrarCartao(){
  String conteudo = "";
  while(!rfid.PICC_IsNewCardPresent() && !rfid.PICC_ReadCardSerial());

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    snprintf(CAD, sizeof(CAD), "%02X%02X%02X%02X",
             rfid.uid.uidByte[0], rfid.uid.uidByte[1],
             rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
    conteudo.concat(CAD);  // Juntando os valores do CAD no conteudo
    conteudo = conteudo + ";";
    UIDLists = UIDLists + conteudo;

    lcd.setCursor(6, 2);
    lcd.print(CAD);
    b = 5;
    pref.putString(listapref, UIDLists);
    vTaskDelay(500);
    lcd.clear();
    lcd.setCursor(5, 2);
    lcd.print("CADASTRADO");
    vTaskDelay(1000);
    cadastrar();
  }
   rfid.PICC_HaltA();
   rfid.PCD_StopCrypto1();
}

// -----------------------------------------------------------------
// -----Erease uid list-----
void format(){
  lcd.clear();
  lcd.setCursor(5, 2);
  lcd.print("FORMATADO");
  
  UIDLists = "/0";
  vTaskDelay(50);
  pref.putString(listapref, UIDLists);
}

// -----------------------------------------------------------------
// -----resetpsswd-----
void resetPassword(){
  password.reset();
  currentpasslen = 0;
  lcd.clear();
  a = 7;

  if(psswdcheck != true)
    eng(); 
}

// -----------------------------------------------------------------
// -----aprovadoPass-----
void aprovadoPass(){
  currentpasslen = 0;

  if (password.evaluate()) {
    lcd.clear();
    lcd.setCursor(7, 2);
    lcd.print("VALIDO");
    vTaskDelay(1000);
    a = 7;
    psswdcheck = true;  // mostra que psswdcheck, apaga a mensagem anterior e segue para a tela screens ou  telas
    if (opnav == true)
      screens();
    else
      telas();  
  } else {
    lcd.clear();
    lcd.setCursor(6, 1);
    lcd.print("INVALIDO");
    lcd.setCursor(2, 2);
    lcd.print("TENTE NOVAMENTE");
    vTaskDelay(1000);
    a = 7;
    psswdcheck = false;  // mostra que psswdcheck, apaga a mensagem anterior e  Voltagea para a tela de Password e colocar a senha correta
  }
  resetPassword();
}

// -----------------------------------------------------------------
// -----erase-----
void erease(char key, int buffer){
  if (buffer == 1){
    if (a == 7)
      a = 7;
    else {
      key = ' ';
      a--;
      lcd.setCursor(a, 2);
      lcd.print(key);
      currentpasslen--;
      vTaskDelay(20); 
    }
  }else if (buffer == 2){
    if (c == 10)
      c = 10;
    else {
      key = ' ';
      c--;
      lcd.setCursor(c, 0);
      lcd.print(key);
    }
  }else{
    if (b == 5)
      b = 5;
    else {
      key = ' ';
      b--;
      lcd.setCursor(b, 1);    // fazer para tag
      lcd.print(key);
    }
    currenttaglen--;
    vTaskDelay(20); 
  }
}

/*---------------------------------------------------------------------------------
-------------------------------------------Screens---------------------------------
---------------------------------------------------------------------------------*/

// -----------------------------------------------------------------
// -----status-----
void status(){
  lcd.clear();
  while (1) {

    lcd.setCursor(6, 0);
    lcd.print("MAQUINA");
    lcd.setCursor(9, 1);
    lcd.print("EM");
    lcd.setCursor(5, 2);
    lcd.print("MANUTENCAO");

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(uid_buffer, sizeof(uid_buffer), "%02X%02X%02X%02X",
               rfid.uid.uidByte[0], rfid.uid.uidByte[1],
               rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

      if (strcmp(uid_buffer, TecCard.c_str()) == 0) {
        manuc = 0;
        manup = 0;
        pref.putInt(correpref, manuc);
        pref.putInt(prevpref, manup);
        opnav = true;
        vTaskDelay(80);
        apx();
      }
    } else if (!rfid.PICC_IsNewCardPresent() && !rfid.PICC_ReadCardSerial()) {

      if (manup == 1) {
        lcd.setCursor(5, 3);
        lcd.print("PREVENTIVA");
        vTaskDelay(40);
      } else if (manuc == 1) {
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
// -----aprox-----
void apx(){
  lcd.clear();
  lcd.setCursor(2, 2);
  lcd.print("APROXIMAR CARTAO");
  
  while (opnav == true) {

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(uid_buffer, sizeof(uid_buffer), "%02X%02X%02X%02X",
               rfid.uid.uidByte[0], rfid.uid.uidByte[1],
               rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
      priority = 2;

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
// -----eng screen-----
void eng(){
  lcd.clear();
  lcd.setCursor(5, 1);
  lcd.print("PASSWORD:");
  lcd.setCursor(14, 3);
  lcd.print("#-SAIR");

  while (1) {

    char key = kpd.getChar();
    vTaskDelay(50);

    if (key != 'N') {  
      vTaskDelay(50);
      if (key == 'C') {
        psswdcheck = false;
        resetPassword(); 
      } else if (key == '#') {
        opnav = true;
        apx();
      } else if (key == 'D') {
          aprovadoPass();
      } else if (key == 'A') {
        lcd.clear();
        esp_restart();
      } else if (key == '*') {
        erease(key, 1); 
      } else if(key == 'B')
        vTaskDelay(5);
      else 
        tag(key, 0);
    }
  }
}

// -----------------------------------------------------------------
// -----screens-----
void screens(){
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("ESCOLHA A OPCAO:");
  lcd.setCursor(0, 1);
  lcd.print("1- CADASTRAR");
  lcd.setCursor(0, 2);
  lcd.print("2- EXCLUIR");
  lcd.setCursor(0, 3);
  lcd.print("3- HORIMETRO/#- SAIR");

  while (opnav == true) {

    char key = kpd.getChar();
    vTaskDelay(20);
    if (key != 'N') {
      vTaskDelay(20);
      if (key == '1') {
        opnav = true;
        vTaskDelay(20);
        cadastrar();
      } else if (key == '2') {
        opnav = true;
        excluir();
      }else if (key == '3') 
        input();  
      else if (key == '#') {
        apx();
      }
    }
  }
}

// -----------------------------------------------------------------
// -----Telas-----
void telas(){
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

    char key = kpd.getChar();

    if (key != 'N') {
      vTaskDelay(30);
      if (key == '1') 
        formatar();
      else if (key == '2') 
        excluir();
      else if (key == '#') {
        opnav = true;  
        apx();
      }
    }
  }
}

// -----------------------------------------------------------------
// -----Cadastrar-----
void cadastrar(){
  lcd.clear();
  lcd.setCursor(1, 1);
  lcd.print("TAG:");
  lcd.setCursor(1, 2);
  lcd.print("RFID:");
  lcd.setCursor(14, 3);
  lcd.print("#-SAIR");


  while (opnav == true) {

    char key = kpd.getChar();
    vTaskDelay(70);

    if (key != 'N' && key != 'F') {
      vTaskDelay(70);
      if (key == 'C')
        dell(1);
      else if ( key == 'D')
        CadastrarCartao();
      else if (key == '#'){ 
        vTaskDelay(20);
        b = 5;
        screens();
      }else if (key == 'A' || key == 'B') 
        vTaskDelay(5);
      else if (key == '*')
        erease(key, 0);
      else 
        tag(key, 1);
    } 
  }
}

// -----------------------------------------------------------------
// -----Manutenção-----
void manutencao(){
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

  while (opnav == true) {

    char key = kpd.getChar();
    vTaskDelay(90);

    if (key != 'N') {
      vTaskDelay(30);
      if (key == '1') {
        manup = 1;
        vTaskDelay(30);
        pref.putInt(prevpref, manup);
        status();
      } else if (key == '2') {
        manuc = 1;
        vTaskDelay(30);
        pref.putInt(correpref, manuc);
        status();
      } else if (key == '#') {
        manuc = 0;
        manup = 0;
        vTaskDelay(30);
        pref.putInt(correpref, manuc);
        pref.putInt(prevpref, manup);
        apx();
      }
    }
  }
}

// -----------------------------------------------------------------
// -----excluir-----
void excluir(){
  lcd.clear();
  lcd.setCursor(1, 2);
  lcd.print("APROXIME O CARTAO");

  while (1) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(uid_buffer, sizeof(uid_buffer), "%02X%02X%02X%02X",
               rfid.uid.uidByte[0], rfid.uid.uidByte[1],
               rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

      if (strcmp(uid_buffer, PesCard.c_str()) == 0) {
        lcd.clear();
        lcd.setCursor(7, 2);
        lcd.print("APAGADO");
        vTaskDelay(1000);
        PesCard = "";
        if (opnav == true) {
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
        if (opnav == true) {
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
void formatar(){
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("FORMARTAR?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (1) {

    char key = kpd.getChar();
    vTaskDelay(90);

    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        format();
        vTaskDelay(1500);
        telas();
      } else if (key == '2') {
        telas();
      }
    }  
  }
}

// -----------------------------------------------------------------
// -----Input data-----
void input(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HORIMETRO:");
  lcd.setCursor(14, 3);
  lcd.print("#-SAIR");

  while (opnav == true){
    char key = kpd.getChar();
    vTaskDelay(50);

    if (key != 'N'){
      vTaskDelay(20);
      if (key == '#'){
        screens();
      }else if(key == 'A' || key == 'B')
        vTaskDelay(5);
      else if (key == 'D')
        hourcheck();
      else if (key == 'C'){
        dell(2);
      }else if (key == '*')
        erease(key, 2);
      else  
        tag(key, 2);
    }  
  }
}

// -----------------------------------------------------------------
// -----General_Hourmeter check-----
void hourcheck(){
  lcd.setCursor(0, 0);
  lcd.print("O HORIMETRO ESTÁ CORRETO?");
  lcd.setCursor(0, 1);
  lcd.print(General_Hourmeter);
  lcd.setCursor(0, 3);
  lcd.print("D-CONFIRMAR");
  lcd.setCursor(15, 3);
  lcd.print("C-CORRIGIR");

  while(opnav == true){
    char key = kpd.getChar();
    vTaskDelay(50);
    if(key == 'D')  
      screens();
    else if (key == 'C')
      input();

  }
}
// -----------------------------------------------------------------
// -----Questions-----
void vazamento(){
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("VAZAMENTO?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (opnav == true) {

    char key = kpd.getChar();

    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        Answer_1 = 1;
        garfos();
      } else if (key == '2') {
        Answer_1 = 2;
        garfos();
      }
    }

  }
}

void garfos(){
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("GARFOS?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (opnav == true) {
    char key = kpd.getChar();

    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        Answer_2 = 1;
        emergencia();
      } else if (key == '2') {
        Answer_2 = 2;
        emergencia();
      }
    }
  }
}

void emergencia(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BOTAO DE EMERGENCIA?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (opnav == true) {
    char key = kpd.getChar();

    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        Answer_3 = 1;
        comando();
      } else if (key == '2') {
        Answer_3 = 2;
        comando();
      }
    }
  }
}

void comando(){
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("FRENTE E RE?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (opnav == true) {
    char key = kpd.getChar();

    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        Answer_4 = 1;
        bateria();
      } else if (key == '2') {
        Answer_4 = 2;
        bateria();
      }
    }
  }
}

void bateria(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BATERIA,CABOS,CONEC?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (opnav == true) {

    char key = kpd.getChar();

    if (key != 'N') {
      vTaskDelay(50);
      priority = 3;
      if (key == '1') {
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("CONCLUIDO");
        lcd.setCursor(2, 2);
        lcd.print("MAQUINA LIBERADA");
        Answer_5 = 1;
        vTaskDlay(1000);
        opnav = true;  
        telafinal();
      } else if (key == '2') {
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("CONCLUIDO");
        lcd.setCursor(2, 2);
        lcd.print("MAQUINA LIBERADA");
        Answer_5 = 2;
        vTaskDelay(1000);
        opnav = true;  
        telafinal();
      }
    }
    manup = 0;
    manuc = 0;
    vTaskDelay(50);
    pref.putInt(correpref, manuc);
    pref.putInt(prevpref, manup);
  }
}

// -----------------------------------------------------------------
// -----final screen-----
void telafinal(){
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("NMS-V0.1");
  lcd.setCursor(5, 1);
  lcd.print("GREENTECH");
  lcd.setCursor(1, 2);
  lcd.print("PRONTO PARA OPERAR");
  lcd.setCursor(5, 3);
  lcd.print("SHOWROOM-SP");

  while (opnav == true) { 

    char key = kpd.getChar();

    if (key != 'N'){
      vTaskDelay(70);
      if (key == 'A') {
        vTaskDelay(50);
        lcd.clear();
        esp_restart();
      }
    }
  } 
}