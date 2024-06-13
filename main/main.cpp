/*----------------------------------------------------------------

  Telemetry V0.6.5 main.cpp
     
  INA226
  MFRC522 
  LCD 20x4 I²c 
  Keypad 4x4 I²c 

  Compiler: VsCode 1.9.0
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
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "PubSubClient.h"
#include "Preferences.h"
#include "esp_system.h"
#include "I2CKeyPad.h"
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
//const char *ssid    =    "Greentech_Visitamtes";  
const char *ssid    =   "Greentech_Administrativo";             
//const char *pass    =    "Visitantes4.0";        
const char *pass    =   "Gr3enTech@2O24*";   
const char *mqtt    =    "192.168.30.130";        // rasp nhoqui
//const char *mqtt    =    "192.168.30.212";      // rasp eng
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
void erease(char key, int buffer);
void tag(char key, int buffer);
void CadastrarCartao();
void resetPassword();
void aprovadoPass();
void ina226_setup();
void emergencia();
void manutencao();
void telafinal();
void cadastrar();
void vazamento();
void readpref();
void formatar();
void screens();
void bateria();
void comando();
void excluir();
void status();
void garfos();
void format();
void recon();
void telas();
void dell();
void eng();
void apx();

// ----------------------------------------------------------------
// -----Objects----
Preferences pref;
WiFiClient TestClient;
MFRC522 rfid(SS, RST);
INA226_WE INA(INAADRESS);
I2CKeyPad kpd(KeyAdress);
PubSubClient client(TestClient);
Password password = Password("2552");
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
bool psswdcheck;
bool passvalue = true;

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
  Wire.begin(SDA, SCL);
  SPI.begin(SCK, MISO, MOSI, RST);
  lcd.init();
  rfid.PCD_Init();
  ina226_setup();
  WiFi.begin(ssid, pass); 
  kpd.begin();
  pref.begin("GT", false);
  
  kpd.setKeyPadMode(I2C_KEYPAD_4x4);
  kpd.loadKeyMap(keypad);

  if(WiFi.status() != WL_CONNECTED)
    WiFi.reconnect();                                                                                               
  
  client.setServer(mqtt, port);           
  if (!client.connected())                            
    client.connect("TestClient", user, passwd);                                       
  
  readpref();

  lcd.setCursor(3, 2);
  lcd.print("INICIALIZANDO");
  vTaskDelay(2000);

  xTaskCreatePinnedToCore(xTaskTelemetry, // function name
                          "Telemetry",    // task name
                          3000,           // stack size in word
                          NULL,           // input parameter
                          1,              // priority
                          NULL,           // task handle
                          0);             // core

  xTaskCreatePinnedToCore(xTaskNav,
                          "Navegation",
                          6000,
                          NULL,
                          1,
                          NULL,
                          1);
                             
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
        if (minuteB >= 20) // normaly minuteB == 10 
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
    sprintf(hourmeter1, "%02d:%02d:%02d", hourmeterB, 
                                      minuteB, secB);
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
    vTaskDelay(1000);
  }
} 

// -----------------------------------------------------------------
// -----Navegation task-----
void xTaskNav(void *pvParameters){
  esp_task_wdt_add(NULL);      //  enable watchdog

  while(1){
    rtc_wdt_feed();                  //  feed watchdog 
    
    lcd.backlight();
    lcd.noCursor();
    lcd.noBlink();
    lcd.clear();

    if (WiFi.status() != WL_CONNECTED  || !client.connected())
      recon();
    client.loop();

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
        }
      } else {
        opnav = true;  // Para tela de Operação
        apx();
      }
    }
    esp_task_wdt_reset();            // reset watchdog if dont return any error
    vTaskDelay(1000);
  }
}

/*---------------------------------------------------------------------------------
---------------------------------------------Functions-------------------------------*/

// -----------------------------------------------------------------
// -----read flash memory-----
void readpref(){
  minuteB     = pref.getInt(minbomb, minuteB);
  minute      = pref.getInt(minpref, minute);
  hourmeter   = pref.getInt(hourpref, hourmeter); 
  hourmeterT  = pref.getInt(hourtrac, hourmeterT);
  hourmeterB  = pref.getInt(hourbomb, hourmeterB);
  manup       = pref.getInt(prevpref, manup);
  manuc       = pref.getInt(correpref, manuc);
}

// -----------------------------------------------------------------
// -----Reconection-----
void recon(){
  WiFi.disconnect();        
  vTaskDelay(300);         
  WiFi.begin(ssid, pass);       
  vTaskDelay(300);
                
  client.connect("TestClient", user, passwd);     
  vTaskDelay(2000);                                  
        
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
void dell(){
  currenttaglen = 0;
  lcd.clear();
  b = 5;
  cadastrar();
}

// -----------------------------------------------------------------
// -----tag-----
void tag(char key, int buffer){
  if (buffer == 1){
    lcd.setCursor(b, 1);
    lcd.print(key);
    b++;

    if (b == 11) {
      b = 5;  
    }
    currenttaglen++;
    vTaskDelay(20); 
  }else{
    lcd.setCursor(a, 2);
    lcd.print("*");
    a++;

    if (a == 11) {
      a = 4; 
    }

    currentpasslen++;
    password.append(key);

    if (currentpasslen == maxpasslen) {
      aprovadoPass();
    }
  }
}

// -----------------------------------------------------------------
// -----cadastro-----
void CadastrarCartao(){
  String conteudo = "";

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    snprintf(CAD, sizeof(CAD), "%02X%02X%02X%02X",
             rfid.uid.uidByte[0], rfid.uid.uidByte[1],
             rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
    conteudo.concat(CAD);  // Juntando os valores do CAD no conteudo
    conteudo = conteudo + ";";
    UIDLists = UIDLists + conteudo;

    client.publish(topic_CAD, CAD);
    lcd.setCursor(6, 2);
    lcd.print(CAD);
    b = 5;
    pref.putString(listapref, UIDLists);
    client.publish("test/lista de cadastro", UIDLists.c_str());
    vTaskDelay(500);
    lcd.clear();
    lcd.print("Cadastrado");
    vTaskDelay(1000);
    cadastrar();
  }
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
  client.publish(topic_CAD, "formatado"); 
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
      client.publish(Topic_user, uid_buffer);

      if (strcmp(uid_buffer, TecCard.c_str()) == 0) {
        manuc = 0;
        manup = 0;
        pref.putInt(correpref, manuc);
        pref.putInt(prevpref, manup);
        client.publish(topic_TEC, " ");
        opnav = true;
        vTaskDelay(80);
        apx();
      }
    } else if (!rfid.PICC_IsNewCardPresent() && !rfid.PICC_ReadCardSerial()) {

      if (manup == 1) {
        client.publish(topic_TEC, "Preventiva");
        lcd.setCursor(5, 3);
        lcd.print("PREVENTIVA");
        vTaskDelay(40);
      } else if (manuc == 1) {
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

      client.publish(Topic_user, uid_buffer);

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
        resetPassword(); //
      }else if (key == '#') {
        apx();
      }else if (key == 'D') {
        if (passvalue == true) {
          aprovadoPass();
          passvalue = false;
        }
      }else if (key == '*') {
        erease(key, 1); 
      }else if(key == 'A' || key == 'B')
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
  lcd.print("#- SAIR");

  while (opnav == true) {

    char key = kpd.getChar();
    vTaskDelay(20);
    if (key != 'N') {
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
      vTaskDelay(20);
      if (key == '1') {
        formatar();
      } else if (key == '2') {
        excluir();
      } else if (key == '#') {
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
  passvalue = false;

  while (opnav == true) {

    char key = kpd.getChar();
    vTaskDelay(50);

    if (key != 'N') {
      vTaskDelay(50);
      if (key == 'C') {
        dell();
      } else if ( key == 'D'){
        CadastrarCartao();
      } else if (key == '#') {
        vTaskDelay(20);
        b = 5;
        screens();
      } else if (key == 'A' || key == 'B') 
        vTaskDelay(5);
      else if (key == '*') {
        erease(key, 0);
      }else {
        tag(key, 1);
      }
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
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
      vTaskDelay(20);
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

      client.publish(topic_CAD, uid_buffer);

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
  vTaskDelay(100);

  while (1) {

    char key = kpd.getChar();
    vTaskDelay(90);
    if (key != 'N') {
      vTaskDelay(20);
      if (key == '1') {
        format();
        vTaskDelay(2000);
        telas();
      } else if (key == '2') {
        telas();
      }
    }  
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
        client.publish(topic_V, "True");
        garfos();
      } else if (key == '2') {
        client.publish(topic_V, "False");
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
        client.publish(topic_G, "True");
        emergencia();
      } else if (key == '2') {
        client.publish(topic_G, "False");
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
        client.publish(topic_E, "True");
        comando();
      } else if (key == '2') {
        client.publish(topic_E, "False");
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
        client.publish(topic_F, "True");
        bateria();
      } else if (key == '2') {
        client.publish(topic_F, "False");
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
      if (key == '1') {
        client.publish(topic_B, "True");
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("CONCLUIDO");
        lcd.setCursor(2, 2);
        lcd.print("MAQUINA LIBERADA");
        vTaskDelay(1000);
        opnav = true;  
        telafinal();
      } else if (key == '2') {
        client.publish(topic_B, "False");
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("CONCLUIDO");
        lcd.setCursor(2, 2);
        lcd.print("MAQUINA LIBERADA");
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
  lcd.print("NMS-V0.8");
  lcd.setCursor(6, 1);
  lcd.print("GREENTECH");
  lcd.setCursor(1, 2);
  lcd.print("PRONTO PARA OPERAR");
  lcd.setCursor(5, 3);
  lcd.print("SHOWROOM-SP");
}