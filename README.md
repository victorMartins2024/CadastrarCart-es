# **Telemetria Greentech**
Badge em Desenvolvimento



Descrição do Projeto
Utilizando um microcontrolador ESP32 embarcado numa placa de LoRaWan da heltec para desenvolver uma telemetria para empilhadeiras e rebocadores 


# Autores
1. [@joaouzeda](https://github.com/joaouzeda)
2. [@victorMartins2024](https://github.com/victorMartins2024)

# Funcionalidades do Projeto
1. Ler a corrente da bateria por meio de um shunt e um INA266
2. Ler a corrente de tração e bomba hidraulica via sensor de efeito hall
3. Realizar um horimetro com base na leitura da corrente
4. Ler o rfid para identificação do operador e identificação da bateria
5. Checklist do operador
6. Publicar todas as informações necessarias no broker

# Tecnologias utilizadas
1. ESP32 
2. LoRaWan
3. MQTT
4. WiFi
5. Sensor de corrente
6. RTOS
7. Arduino component
8. Sensor de corrente efeito hall
9. Sensor de corrente INA226 + shunt

# Versão
Telemetry V0.5.1 - Implementação do displayt 20x4, keypad I²C e a biblioteca de senha 


![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white)
![Mosquitto](https://img.shields.io/badge/mosquitto-%233C5280.svg?style=for-the-badge&logo=eclipsemosquitto&logoColor=white)
