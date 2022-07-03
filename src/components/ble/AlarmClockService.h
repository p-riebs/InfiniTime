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

      int OnCommand(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt);

      void event(char event);

    private:
      struct ble_gatt_chr_def characteristicDefinition[3];
      struct ble_gatt_svc_def serviceDefinition[2];

      uint16_t eventHandle {};

      Pinetime::System::SystemTask& m_system;
      Pinetime::Controllers::AlarmClockController& alarmClockController;
    };
  }
}
