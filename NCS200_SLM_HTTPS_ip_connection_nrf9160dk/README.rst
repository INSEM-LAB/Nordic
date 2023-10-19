HTTP Sever connection by IP address on serial LTE modem example
##########################

.. contents::
   :local:
   :depth: 2

nRF9160 serial LTE modem example을 이용하여 HTTPS server와 연결하려는 경우 hostname을 사용해야 합니다. (https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.0.0/nrf/applications/serial_lte_modem/doc/HTTPC_AT_commands.html)

hostname없이 IP address로 연결할 경우 error가 발생합니다. hostname이 아닌 IP address만을 사용해야 하는 경우 serial LTE modem에서의 수정 부분에 대한 샘플코드 입니다.

Notice
************
slm_at_httpc.c 파일에서 //test라고 주석이 명기되어 있는 부분이 수정된 부분입니다.

