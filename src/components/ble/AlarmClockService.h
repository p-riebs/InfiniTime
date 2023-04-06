#pragma once
#include <cstdint>
#include <string>
#include <array>
#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#undef max
#undef min

namespace Pinetime {

  namespace System {
    class SystemTask;
  }
  namespace Controllers {
    class AlarmClockController;

    class AlarmClockService {
    public:
      explicit AlarmClockService(Pinetime::System::SystemTask& systemTask, Pinetime::Controllers::AlarmClockController& alarmClockController);
      void Init();

      int OnCommand(struct ble_gatt_access_ctxt* ctxt);

      void OnAlarmTimeChange(uint8_t hours, uint8_t minutes);

    private:
      struct ble_gatt_chr_def characteristicDefinition[3];
      struct ble_gatt_svc_def serviceDefinition[2];

      uint16_t alarmClockTimeHandle {};

      Pinetime::System::SystemTask& m_system;
      Pinetime::Controllers::AlarmClockController& alarmClockController;
    };
  }
}
