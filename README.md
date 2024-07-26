<h1 align="center"> **Telemetria Greentech** </h1>

![Badge em Desenvolvimento](http://img.shields.io/static/v1?label=STATUS&message=EM%20DESENVOLVIMENTO&color=GREEN&style=for-the-badge)

# *Project Description*
The objective is to create a monitoring system for forklifts and other equipment that allows the company to track the voltage and current readings of a machine, as well as the time it is in use. It works by reading analog data from a shunt, which is published to an MQTT broker in JSON format. The data can be exported to Prometheus and visualized on a Grafana dashboard. In addition to the machine readings, the project also includes a navigation system for four types of users:
    *Engineering team
    *Equipment operator
    *Operations administration
    *Technician
Each user type has limited access to specific telemetry functions.

# *Project Features*

1. Read the system voltage using a shunt and an INA266
2. Implement an hour meter based on current calculation
3. Identify each type of user
4. Operator checklist
5. Register a new RFID card
6. Delete RFID card
7. Format the entire card list
8. Register cards in a list
9. Change the machine status
10. Publish all necessary information to the broker


# *Technologies Used*
1. ESP32 
2. LoRaWan
3. MQTT
4. WiFi
5. Sensor de corrente
6. RTOS
7. Arduino component
9. INA226 + shunt
10. Raspberry pi3
11. RFID
12. Display
13. Keypad

# *General Information*

Compiler: VsCode 1.88.1  <br/>
MCU: ESP32  <br/>
Board: Dev module 38 pins <br/>
Date: 2024, June <br/>

# Autores

| [<img loading="lazy" src="https://avatars.githubusercontent.com/u/55409817?v=4" width=115><br><sub>João Uzêda</sub>](https://github.com/joaouzeda) |  [<img loading="lazy" src="https://avatars.githubusercontent.com/u/162138511?v=4" width=115><br><sub>Victor Martins</sub>](https://github.com/victorMartins2024) |  [<img loading="lazy" src="https://avatars.githubusercontent.com/u/167223272?v=4" width=115><br><sub>Fernando Nhoqui</sub>](https://github.com/FernandoNhoqui) |
| :---: | :---: | :---: |

# *Badges*

![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white)
![Mosquitto](https://img.shields.io/badge/mosquitto-%233C5280.svg?style=for-the-badge&logo=eclipsemosquitto&logoColor=white)
![Prometheus](https://img.shields.io/badge/Prometheus-E6522C?style=for-the-badge&logo=Prometheus&logoColor=white)
![Grafana](https://img.shields.io/badge/grafana-%23F46800.svg?style=for-the-badge&logo=grafana&logoColor=white)
![Raspberry Pi](https://img.shields.io/badge/-RaspberryPi-C51A4A?style=for-the-badge&logo=Raspberry-Pi)

