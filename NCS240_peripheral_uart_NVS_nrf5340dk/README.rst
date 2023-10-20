Adding NVS function in BLE peripheral uart on nRF5340 DK
##########################

.. contents::
   :local:
   :depth: 2

nRF5340 DK에서 User data를 저장하기 위해 nRF5340의 일정한 flash partition을 설정하여 사용할 수 있도록 구성한 example 입니다.

nRF connect SDK의 BLE 예제중에서 BLE peripheral uart example을 기준으로 진행하였으며

project 폴더에 있는 pm_static.yml 파일을 이용하여 user_storage영역을 설정하였습니다. 

설정된 user_storage 영역은 Visual Studio Code의 Actions>Memory report를 실행하면 설정된 partitions을 확인할 수 있으며 

NVS에 대한 내용은 아래 링크를 참조하시면 됩니다.

(https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.4.0/zephyr/services/storage/nvs/nvs.html)

Requirements
************

Software Delopment Kit : NCS 2.4.0

Development Kit : nRF5340 DK




