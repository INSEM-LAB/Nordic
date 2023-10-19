BLE throughput enhancement based on peripheral uart example 
##########################

.. contents::
   :local:
   :depth: 2

이 예제는 BLE peripheral uart example에서 central 장치와의 BLE throughput을 증대시키기 위해서 수정 및 테스트를 진행한 프로젝트입니다. BLE perpheral uart reference와 비교하시면 수정된 부분을 쉽게 확인할 수 있습니다.

Requirements
************

Software Delopment Kit : NCS 2.3.0

Development Kit : nRF52840 DK

Notice
************

테스트 진행을 위해 nRF connect for smartphone App과 connection 후 진행하였으며 connection interval은 100ms기준으로 진행이 됩니다. App에서 설정 부분을 이용하여 MTU update request를 nRF52840으로 보내야 하며(테스트를 위해 사용자가 수정으로 선택) 높은 MTU 값으로 설정이 되어야 BLE throughput을 향상 시킬 수 있습니다.
