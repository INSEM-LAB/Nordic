DTM(Direct Test Mode) + BLE peripheral uart example on nRF5340 DK
##########################

.. contents::
   :local:
   :depth: 2

nRF5340 DK를 기준으로 DTM(Direct Test Mode) + BLE peripheral uart example를 merge한 샘플 코드 입니다.

Power on 때 P1.03(app core에서 check)과 P1.04(net core에서 check)을 확인 하여 DTM 모드로 동작할지 BLE로 동작할지 선택하게 됩니다.

P1.03과 P1.04 모두 high state인 경우 DTM 모드로 동작합니다.

github 파일들은 reference NCS 저장 위치에서 아래 위치한 파일로 변경이 되어야 하며 폴더(hci_rpmsg_ohsung)은 아래 위치에 위치시키면 됩니다

<mpsl_init.c>

위치 : nrf\\subsys\\mpsl\\init\\

<CMakeLists.txt, Kconfig>

위치 : nrf\\samples\\

<uart_ipc.c>

위치 : nrf\\drivers\\serial\\

<hci_rpmsg_ohsung> 압축 해제 후 사용

위치 : zephyr\\samples\\bluetooth\\

<v240_dtm_uart_nrf5340dk> 압축 해제 후 사용

BLE peripheral uart 기반으로 DTM 기능을 추가한 application code 입니다.

Visual Studio에서 'Open an existing application'을 선택하여 load한 후 사용합니다.


Requirements
************

Software Delopment Kit : NCS 2.4.0

Development Kit : nRF5340 DK




