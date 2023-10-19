ADC function on serial LTE modem example
##########################

.. contents::
   :local:
   :depth: 2

nRF9160 serial LTE modem example은 nRF9160의 GPIO, TWI를 AT command를 이용하여 control할 수 있도록 reference가 구성되어 있습니다.여기에 추가적으로 nRF9160의  ADC 기능을 AT command로 사용할 수 있도록 구성한 샘플 코드 입니다.

nRF9160의  ADC는 다음 devzone 링크를 참조 하였습니다.
(https://devzone.nordicsemi.com/f/nordic-q-a/65766/example-code-for-a-d)

Requirements
************

Software Delopment Kit : NCS 2.0.0

Development Kit : nRF9160 DK

Avaiable AT commands
************

AT#XADC

위 AT command를 입력하면 ADC 동작 후의 결과 값이 return됩니다.



