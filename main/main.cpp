/*----------------------------------------------------------------

  Telemetry V1.0.1 main.cpp

  Notes: 

  Features:
    1. Inserir um jeito de calcular o desvio padrão e comparar com
    o desvio padrão do efeito hall informado pelo fabricante junto 
    com a comparação da leitura do shunt e assegurar a saude do 
    sensor

      
      # float buf[10];
      # for (int i = 0; i <10; i++){
      #     buf[i] = leitura;
      #     delay(10);
      # }
      #
      # float valormedio = 0;
      # for  (int i = 0; i < 10; i++){
      #    valormedio += buf[i];
      # }    
      #
      # corrente = valormedio / 6.0
      #
      # dp = (corrente)*(1/2)

    2. Efeito Hall on/off para saber qual corrente o shunt ta lendo
    
  Notes: 

  Features:
    1. Inserir um jeito de calcular o desvio padrão e comparar com
    o desvio padrão do efeito hall informado pelo fabricante junto 
    com a comparação da leitura do shunt e assegurar a saude do 
    sensor

      
      # float buf[10];
      # for (int i = 0; i <10; i++){
      #     buf[i] = leitura;
      #     delay(10);
      # }
      #
      # float valormedio = 0;
      # for  (int i = 0; i < 10; i++){
      #    valormedio += buf[i];
      # }    
      #
      # corrente = valormedio / 6.0
      #
      # dp = (corrente)*(1/2)

    2. Efeito Hall on/off para saber qual corrente o shunt ta lendo
    

  Compiler: VsCode 1.94.1
  MCU: ESP32
  Espressif: 5.3.1
  Arduino Component: 3.1.0
  Board: Dev module 38 pins

  Author: João  G. Uzêda & Victor Hugo
  date: 2024, Set
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
#include "LoRa.h"
#include "Wire.h"
#include "SPI.h"

// ---------------------------------------------------------------- 
// ---defines---
#define   SF                      9
#define   FREQ                    915E6
#define   SBW                     62.5E3	
#define   SYNC_WORD               0x34
#define   TX_POWER_DB             17
#define   PREAMBLE_LENGTH         8
#define   SHUNT_RESISTENCE        0.75
#define   CODING_RATE_DENOMINATOR 5

char keypad[19] = "123A456B789C*0#DNF";

// ----------------------------------------------------------------  
//----I²C Adresses------
#define LCD_ADRESS 0x27 
#define INA_ADRESS 0x40 
#define Key_Adress 0x21

// ----------------------------------------------------------------  
//----pins------
#define   RFID_CS       22
#define   RFID_RST      5

#define   SPI_SCK       14
#define   SPI_MISO      2
#define   SPI_MOSI      15

#define   I2C_SDA       32
#define   I2C_SCL       33 

#define   LORA_CS       19
#define   LORA_RST      18 

// ----------------------------------------------------------------
// -----Tasks----
void xTaskTelemetry(void *pvParameters);
void xTaskNav(void *pvParameters);
void xTaskSend(void *pvParameters);

// ----------------------------------------------------------------
// -----Func. Prototype----
void erease(char key, int buffer);
void tag(char key, int buffer);
//void CarregarCartoesCadastrados();
void CadastrarCartao();
void dell(int buffer);
void resetPassword();
void aprovadoPass();
void ina226_setup();
void emergencia();
void RFIDConfig();
void manutencao();
void LoRaconfig();
void telafinal();
void cadastrar();
void vazamento();
void hourcheck();
void readpref();
void formatar();
void Taskconf();
void screens();
void bateria();
void comando();
void excluir();
void status();
void garfos();
void format();
void telas();
void input();
void recon();
void eng();
void apx();

// ----------------------------------------------------------------
// -----Objects----
Preferences pref;
INA226_WE INA(INA_ADRESS);
I2CKeyPad kpd(Key_Adress);
MFRC522 rfid(RFID_CS, RFID_RST);
LiquidCrystal_I2C lcd(LCD_ADRESS, 20, 4);
Password password = Password((char*)"2552");

// ----------------------------------------------------------------
// ----Variables----
typedef struct  {
  unsigned Hour   : 20;
  unsigned minute : 6;
  unsigned sec    : 6; 
} HourmeterTractionEngine;

HourmeterTractionEngine THour;

typedef struct {
  unsigned Hour;
  unsigned minute;
  unsigned sec; 
} GeneralHourmeter;

GeneralHourmeter GHour;

typedef struct {
  unsigned Hour;
  unsigned minute;
  unsigned sec; 
} HourmeterHidraulicBomb; 

HourmeterHidraulicBomb BHour;

typedef struct {
  unsigned Answer1 : 1;
  unsigned Answer2 : 1;
  unsigned Answer3 : 1;
  unsigned Answer4 : 1;
  unsigned Answer5 : 1;
} Answers;

Answers  Ans;

float GeneralCurrent;
float CurrentHidraulicBomb;
float CurrentTractionEngine;
float Voltage;

uint8_t priority = 0;

// ----------------------------------------------------------------
// ----Constants----
byte a = 7;
byte b = 5;
byte c = 10;
byte maxtaglen = 6; 
byte maxpasslen = 5;  
uint8_t MID = 150;

// ----------------------------------------------------------------
// ----Buffers----
byte currentpasslen = 0;  
byte currenttaglen = 0;
char uid_buffer[32];
char CAD[32];
int  manup =  0; 
int  manuc =  0; 
bool opnav;
bool psswdcheck;
String cartoesCadastrados[2000]; // Defina o tamanho da lista
int savecard = 0; // Variável para controlar o índice da lista

// ----------------------------------------------------------------
// --Preferences Key---
const char *cadaspref =   "Cadastro";
const char *prevpref  =   "Manupreve";
const char *correpref =   "Manucorre";
const char *HourmeterPreference  =   "hour";
const char *listapref      =   "Cadastro Cartoes";
const char *GenerealMinutePreference      =   "min";
const char *MinuteEnginePreference        =   "trac";
const char *MinuteHidraulicPreference     =   "minbomb";
const char *HourmeterEnginePreference     =   "hourtrac";
const char *HourmeterHidraulicPreference  =   "hourbomb";

// ----------------------------------------------------------------
// -----UIDS-----
String OpeCard  =   "6379CF9A"; //"E6A1191E"; 
String AdmCard  =   "29471BC2";  
String TecCard  =   "D2229A1B";  
String PesCard  =   "B2B4BF1B";
String UIDLists =   " ";
String TAGLists =   " ";
String listaCartoes = "";

TaskHandle_t SenderHandle;

// ----------------------------------------------------------------
// -----Main Func-----
extern "C" void app_main(){
  initArduino();
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  lcd.init();
  ina226_setup();
  RFIDConfig();
  LoRaconfig();
  kpd.begin();
  pref.begin("GT", false);
  //CarregarCartoesCadastrados();
  
  kpd.setKeyPadMode(I2C_KEYPAD_4x4);

  kpd.loadKeyMap(keypad);
  lcd.backlight();
  lcd.noCursor();
  lcd.noBlink();

  readpref();

  lcd.setCursor(3, 2);
  lcd.print("INICIALIZANDO");
  Serial.println("System Initizalized");

  Taskconf();

  vTaskDelay(2500);
  lcd.clear();      
}

/*---------------------------------------------------------------------------------
--------------------------------------Tasks----------------------------------------
---------------------------------------------------------------------------------*/

// -----------------------------------------------------------------
// -----telemetry-----
void xTaskTelemetry(void *pvParameters){            
  esp_task_wdt_add(NULL);
  while(1){  
    rtc_wdt_feed();   

    INA.readAndClearFlags();

    Voltage = INA.getBusVoltage_V();
    GeneralCurrent = INA.getShuntVoltage_mV() / SHUNT_RESISTENCE;

    CurrentHidraulicBomb = GeneralCurrent - 1.5;
    CurrentTractionEngine = GeneralCurrent - 2.5;

    if (GeneralCurrent >= 3.5){ 
      GHour.sec++;   
      if (GHour.sec >= 60){         
        GHour.sec -= 60;
        GHour.minute++;
        if (GHour.minute == 10)
          pref.putInt(GenerealMinutePreference, GHour.minute);
        else if (GHour.minute == 20)
          pref.putInt(GenerealMinutePreference, GHour.minute);
        else if (GHour.minute == 30)
          pref.putInt(GenerealMinutePreference, GHour.minute);
        else if (GHour.minute == 40)
          pref.putInt(GenerealMinutePreference, GHour.minute);
        else if (GHour.minute == 50)
          pref.putInt(GenerealMinutePreference, GHour.minute);
      }                       
      if (GHour.minute >= 60){
        GHour.minute -= 60;
        GHour.Hour++;
        priority = 1;
        pref.remove(GenerealMinutePreference);
        pref.putInt(HourmeterPreference, GHour.Hour);
      }                     
    } 
    if (CurrentHidraulicBomb >= 13){ 
      BHour.sec++;
      if (BHour.sec >= 60){        
        BHour.sec-=60;
        BHour.minute++;
        if (BHour.minute >= 10)
          pref.putInt(MinuteHidraulicPreference, BHour.minute);
        else if (BHour.minute == 30)
          pref.putInt(MinuteHidraulicPreference, BHour.minute);       
      }
      if (BHour.minute >= 60){
        BHour.minute-=60;
        BHour.Hour++;
        priority = 1;
        pref.remove(MinuteHidraulicPreference);
        pref.putInt(MinuteHidraulicPreference, BHour.Hour);
      }
    } 
    if (GeneralCurrent >= 13){ 
      THour.sec++;
      if (THour.sec >= 60){        
        THour.sec-=60;
        THour.minute++;
        if (THour.minute == 20)
          pref.putInt(MinuteEnginePreference, THour.minute);
        else if (THour.minute == 40)
          pref.putInt(MinuteEnginePreference, THour.minute);
      }
      if (THour.minute >= 60){
        THour.minute-=60;
        THour.Hour++;
        priority = 1;
        pref.remove(MinuteEnginePreference);
        pref.putInt(HourmeterEnginePreference, THour.Hour);
      }
    } 
    esp_task_wdt_reset();       
    vTaskDelay(500);
  }
} 

// -----------------------------------------------------------------
// -----Navegation task-----
void xTaskNav(void *pvParameters){ 
  esp_task_wdt_add(NULL); 
  while(1){
    rtc_wdt_feed();
    digitalWrite(RFID_CS, LOW);                 
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
          opnav = false; 
          vTaskDelay(80);
          eng();        
        }
      }else {
        opnav = true; 
        apx();
      }
    }
    
    esp_task_wdt_reset(); 
    vTaskDelay(1500);
  }
}

// -----------------------------------------------------------------
// -----Send data task-----
void xTaskSend(void *pvParameters){
  //esp_task_wdt_add(NULL);
  while(1){
    //rtc_wdt_feed();
    if (LoRa.available()){
      Serial.print("Lora disponivel");
      if (priority == 0){
        LoRa.beginPacket();
        LoRa.print(0);
        LoRa.print("OPA!");
        LoRa.endPacket();
      }else if (priority == 1){
        LoRa.beginPacket();
        LoRa.print(1);
        LoRa.print("OPA!");
        LoRa.endPacket();
        Serial.printf("Horimetro1: %d\nHorimetro2: %d\nHorimetro3: %d", GHour.Hour, 
        THour.Hour, BHour.Hour);
      }else if(priority == 2){
        LoRa.beginPacket();
        LoRa.print(2);
        LoRa.println(uid_buffer);
        Serial.print(uid_buffer);
        LoRa.endPacket();
        priority = 0;
      }else if (priority == 3){
       LoRa.beginPacket();
        LoRa.print(3);
        LoRa.print("OPA!");
        LoRa.endPacket();
      }
    }else recon();
    vTaskDelay(1000);
  }
}

/*---------------------------------------------------------------------------------
-------------------------------------Functions-------------------------------------
---------------------------------------------------------------------------------*/

// -----------------------------------------------------------------
// -----LCD config-----
void Taskconf(){
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
                          2500,
                          NULL,
                          2,
                          NULL,
                          0);

}

// -----------------------------------------------------------------
// -----LoRa config-----
void LoRaconfig(){
  LoRa.setPins(LORA_CS, LORA_RST);

  pinMode(LORA_CS, OUTPUT);
  pinMode(LORA_RST, OUTPUT);

  if (!LoRa.begin(FREQ)){
    LoRa.begin(FREQ);
    Serial.println("Starting LoRa failed!");
  }

  LoRa.setSyncWord(SYNC_WORD);
  LoRa.setSpreadingFactor(SF);
  LoRa.setSignalBandwidth(SBW);
  LoRa.setTxPower(TX_POWER_DB); 
  LoRa.setPreambleLength(PREAMBLE_LENGTH);
  LoRa.setCodingRate4(CODING_RATE_DENOMINATOR);
  Serial.println("LoRa Initizalized!");
}

// -----------------------------------------------------------------
// -----RFID config-----
void RFIDConfig(){
  rfid.PCD_Init();
  Serial.println("RFID Initizalized");
}

// -----------------------------------------------------------------
// -----read flash memory-----
void readpref(){
  GHour.Hour         =   pref.getInt(HourmeterPreference, GHour.Hour); 
  GHour.minute       =   pref.getInt(GenerealMinutePreference, GHour.minute);
  THour.Hour         =   pref.getInt(HourmeterEnginePreference, THour.Hour);
  THour.minute       =   pref.getInt(MinuteEnginePreference, THour.minute);
  BHour.Hour         =   pref.getInt(MinuteHidraulicPreference, BHour.Hour);
  BHour.minute       =   pref.getInt(HourmeterHidraulicPreference, BHour.minute);
  manuc              =   pref.getInt(correpref, manuc);
  manup              =   pref.getInt(prevpref, manup);
  listaCartoes       =   pref.getString(listapref,  listaCartoes);

}

// -----------------------------------------------------------------
// -----Reconection-----
void recon(){
  LoRa.begin(FREQ);        
}

// -----------------------------------------------------------------
// -----INA setup-----
void ina226_setup(){
  INA.init();
  INA.setAverage(AVERAGE_4); 
  INA.setConversionTime(CONV_TIME_1100); 
  INA.setResistorRange(SHUNT_RESISTENCE, 300.0); 
  INA.waitUntilConversionCompleted(); 	
  Serial.println("INA Initizalized");
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
  }
}

// -----------------------------------------------------------------
// -----cadastro-----
/* void CarregarCartoesCadastrados() {
  //String conteudo = pref.getString(listapref, "");
  int indice = 0;
  String cartao = "";

  for (int i = 0; i < conteudo.length(); i++) {
    if (conteudo[i] == ';') {
      cartoesCadastrados[indice] = cartao;
      indice++;
      cartao = "";
    } else {
      cartao += conteudo[i];
    }
  }

  savecard = indice;
} */

void CadastrarCartao() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    snprintf(CAD, sizeof(CAD), "%02X%02X%02X%02X",
              rfid.uid.uidByte[0], rfid.uid.uidByte[1],
              rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

    // Verifica se o cartão já existe na lista de cartões cadastrados
    bool cartaoJaCadastrado = false;
    for (int i = 0; i < savecard; i++) {
      if  (cartoesCadastrados[i] == CAD) {
        cartaoJaCadastrado = true;
        break;
      }
    }

    if (cartaoJaCadastrado) {
      // Exibe mensagem de cartão já cadastrado
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("CARTAO JA CADASTRADO!");
      delay(1500);
      lcd.clear();
      cadastrar();
    } else {
        // Adiciona o cartão à lista de cartões cadastrados
        cartoesCadastrados[savecard] = CAD;
        savecard++;

        // Salva a lista de cartões cadastrados na EEPROM
        
        for (int i = 0; i < savecard; i++) {
        listaCartoes += cartoesCadastrados[i];
        if (i < savecard - 1) {
        listaCartoes += ";";
        }
      }
      pref.putString(listapref, listaCartoes);

        // Atualiza a exibição no LCD
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("CARTAO CADASTRADO:");
      lcd.setCursor(1, 2);
      lcd.print(CAD);
      delay(1000);
      lcd.clear();
      cadastrar();
    }
  }
}

// -----------------------------------------------------------------
// -----Erease uid list-----
void format(){
  lcd.clear();
  lcd.setCursor(5, 2);
  lcd.print("FORMATADO");
  
  UIDLists = "/0";
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
    psswdcheck = false;  // mostra que psswdcheck, apaga a mensagem anterior e  volta para a tela de Password e colocar a senha correta
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
    //esp_task_wdt_reset();
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
      priority = 2;

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

      if (manup == 1) {;
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
  while (opnav == true) {
    lcd.setCursor(2, 2);
    lcd.print("APROXIMAR CARTAO");
    //esp_task_wdt_reset();
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
        vTaskDelay(500);
        vazamento();
      } else if (strcmp(uid_buffer, TecCard.c_str()) == 0) {
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("TECNICO");
        lcd.setCursor(4, 2);
        lcd.print("IDENTIFICADO");
        vTaskDelay(500);
        manutencao();
      } else if (strcmp(uid_buffer, AdmCard.c_str()) == 0) {
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("ADMINISTRADOR");
        lcd.setCursor(4, 2);
        lcd.print("IDENTIFICADO");
        vTaskDelay(500);
        eng();
      }
    }
    vTaskDelay(500);
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
      else tag(key, 0);
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
  /* lcd.setCursor(1, 1);
  lcd.print("TAG:"); */
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
      // Aqui chamamos a função para cadastrar o cartão
        CadastrarCartao();
  }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
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
      priority = 2;

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
// -----GeneralHourmeter check-----
void hourcheck(){
  lcd.setCursor(0, 0);
  lcd.print("O HORIMETRO ESTÁ CORRETO?");
  lcd.setCursor(0, 1);
  lcd.print(GHour.Hour);
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
  lcd.print("0 - NAO");

  while (opnav == true) {

    char key = kpd.getChar();

    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        Ans.Answer1 = 1;
        garfos();
      } else if (key == '0') {
        Ans.Answer1  = 0;
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
  lcd.print("0 - NAO");

  while (opnav == true) {
    char key = kpd.getChar();

    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        Ans.Answer2 = 1;
        emergencia();
      } else if (key == '0') {
        Ans.Answer2  = 0;
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
  lcd.print("0 - NAO");

  while (opnav == true) {
    char key = kpd.getChar();

    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        Ans.Answer3 = 1;
        comando();
      } else if (key == '0') {
        Ans.Answer3 = 0;
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
        Ans.Answer4 = 1;
        bateria();
      } else if (key == '0') {
        Ans.Answer4 = 0;
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
  lcd.print("0 - NAO");

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
        Ans.Answer5 = 1;
        vTaskDelay(1000);
        opnav = true;  
        telafinal();
      } else if (key == '0') {
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("CONCLUIDO");
        lcd.setCursor(2, 2);
        lcd.print("MAQUINA LIBERADA");
        Ans.Answer5 = 0;
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
  lcd.print("IHM-V0.9");
  lcd.setCursor(5, 1);
  lcd.print("GREENTECH");
  lcd.setCursor(1, 2);
  lcd.print("PRONTO PARA OPERAR");
  lcd.setCursor(5, 3);
  lcd.print("SHOWROOM-SP");
  //vTaskResume(SenderHandle);
  while (opnav == true) { 
    char key = kpd.getChar();
    if (key != 'N'){
      vTaskDelay(70);
      if (key == 'A')
        esp_restart();
      else if  (key == 'B')
        apx();
    }
  } 
}

// end code