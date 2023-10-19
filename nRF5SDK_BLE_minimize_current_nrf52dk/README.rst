Minimizing current consumption of BLE peripheral device
##########################

.. contents::
   :local:
   :depth: 2

이 예제는 BLE peripheral 장치의 current consumption을 최소화 하기 위한 firwmare 수정 부분에 대해 알려드리는 프로젝트 입니다.

Requirements
************

Software Delopment Kit : **nRF5 SDK 17.1.0**

Development Kit : **nRF52 DK**

Notice
************
예제에서 아래 list된 항목들의 주석(comment)을 확인하면 Ble peripheral device의 current consumption을 최소화 하기 위해서 수정된 부분을 확인할 수 있습니다.

1. STEP_1 Log Disable
#. STEP_2 idle Sleep
#. STEP_3 DCDC Mode enable
#. STEP_4 Advertising interval
#. STEP_5 Non Connectable Advertising(**if your device doesn't need to connect any other BLE central device, use 'ble_advertising_non_connectable.c'**)
#. STEP_6 Connection Parameter (Connection interval, Slave Latency, Supervisor timeout)
#. STEP_7 TX Power
#. STEP_8 Advertising Stop / Start
#. STEP_9 System Off Sleep
#. STEP_10 UART Disable 


