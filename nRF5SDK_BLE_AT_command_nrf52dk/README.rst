BLE AT command example
##########################

.. contents::
   :local:
   :depth: 2

이 예제는 PC에서 터미널 프로그램을 사용하여 nRF52 DK의 BLE 동작(Advertising interval, Tx power 등)을 변경하기 위한 프로젝트 입니다.

BLE 동작 변경을 위해 AT command를 사용하며 nRF52 DK로부터 AT command에 대한 response를 확인할 수 있으며 BLE 동작 변경은 Central 장치와 connection하지 않은 상태에서 가능하며 Central 장치와 connection이 되었을 경우 일반적인 BLE 장치로서 동작합니다.

Requirements
************

Software Delopment Kit : nRF5 SDK 17.1.0

Development Kit : nRF52 DK

Avaiable AT commands
************

1. AT?\\r\\n : checks AT command mode
#. AT+VER?\\r\\n : returns AT command version
#. AT+ROLE?\\r\\n : return current BLE mode(peripheral)
#. AT+ADVSTOP\\r\\n : stops BLE advertising
#. AT+ADVSTART\\r\\n : start BLE advertising
#. AT+ADVINT?\\r\\n : returns current advertising interval
#. AT+TXPWR?\\r\\n : returns current tx power
#. AT+NAME?\\r\\n : returns current BLE device name
#. AT+SLEEP\\r\\n : enters system off mode after setup wake up

Advertising interval을 변경하기 위해서는 아래 AT command를 순차적으로 입력합니다..

a. AT+ADVSTOP\\r\\n
#. AT+ADVINT=xxxx\\r\\n : set advertising interval

위 AT command 입력 후 변경된 Advertising interval로 Advertising이 다시 시작됩니다.

Tx power를 변경하기 위해서는 아래 AT command를 순차적으로 입력합니다.

a. AT+ADVSTOP\\r\\n
#. AT+TXPWR=+x\\r\\n or AT+TXPWR=-xx\\r\\n : set tx power

위 AT command 입력 후 변경된 Tx power로 Advertising이 다시 시작됩니다.

Device name을 변경하기 위해서는 아래 AT command를 순차적으로 입력합니다.

a. AT+ADVSTOP\\r\\n
#. AT+NAME=xxxxxx\\r\\n : set BLE device name

위 AT command 입력 후 변경된 Device name으로 Advertising이 다시 시작됩니다.

Testing
********************

nRF52 DK에 download한 후 아래 순서로 진행합니다.

1. nRF52 DK를 USB로 PC와 연결 후 power on 합니다.
#. PC에서 테라텀과 같은 터미널 프로그램을 실행합니다.
#. 터미널 입력창에서 AT command를 입력하여 테스트를 진행합니다. 

