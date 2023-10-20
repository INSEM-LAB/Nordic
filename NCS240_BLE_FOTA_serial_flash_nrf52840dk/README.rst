BLE FOTA using external serial flash on nRF52840 DK
##########################

.. contents::
   :local:
   :depth: 2

nRF52840 DK의 external serial flash를 memory partition으로 사용하여 BLE FOTA를 진행할 수 있도록 구성한 샘플 코드 입니다.

nRF connect SDK의 BLE 예제중에서 BLE peripheral uart example을 기준으로 진행하였으며

Using external flash memory partitions를 사용하는 방법에 대한 내용은 아래 링크를 참조하시면 됩니다.

(https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.4.0/nrf/app_dev/bootloaders_and_dfu/bootloader_external_flash.html)

Visual Studio code에서 Actions>Memory report에서 확인하면 external flash partition이 추가된 것을 확인할 수 있습니다.

Requirements
************

Software Delopment Kit : NCS 2.4.0

Development Kit : nRF52840 DK




