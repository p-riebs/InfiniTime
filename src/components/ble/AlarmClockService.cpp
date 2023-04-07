#include "components/ble/AlarmClockService.h"
#include "components/alarmclock/AlarmClockController.h"
#include <cstring>
#include "systemtask/SystemTask.h"

namespace {
  // 1111yyxx-78fc-48fe-8e23-433b3a1942d0
  constexpr ble_uuid128_t CharUuid(uint8_t x, uint8_t y) {
    return ble_uuid128_t {.u = {.type = BLE_UUID_TYPE_128},
                          .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, x, y, 0x11, 0x11}};
  }

  // 10000000-78fc-48fe-8e23-433b3a1942d0
  constexpr ble_uuid128_t BaseUuid() {
    return CharUuid(0x00, 0x00);
  }

  constexpr ble_uuid128_t acsUuid {BaseUuid()};

  constexpr ble_uuid128_t acsTimeCharUuid {CharUuid(0x01, 0x00)};
  constexpr ble_uuid128_t acsStateCharUuid {CharUuid(0x02, 0x00)};

  constexpr uint8_t MaxStringSize {40};

  int AlarmClockTimeCallback(uint16_t /*conn_handle*/, uint16_t /*attr_handle*/, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    return static_cast<Pinetime::Controllers::AlarmClockService*>(arg)->OnAlarmClockTime(ctxt);
  }
  int AlarmClockStateCallback(uint16_t /*conn_handle*/, uint16_t /*attr_handle*/, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    return static_cast<Pinetime::Controllers::AlarmClockService*>(arg)->OnAlarmClockState(ctxt);
  }

  int GetAlarmStateAsInt(Pinetime::Controllers::AlarmClockController::AlarmState alarmState) {
    switch(alarmState) {
      case Pinetime::Controllers::AlarmClockController::AlarmState::Not_Set:
        return 0;
      case Pinetime::Controllers::AlarmClockController::AlarmState::Set:
        return 1;
      case Pinetime::Controllers::AlarmClockController::AlarmState::Alerting:
        return 2;
      default:
        return -1;
    }
  }
}


Pinetime::Controllers::AlarmClockService::AlarmClockService(Pinetime::System::SystemTask& system, 
                                                            Pinetime::Controllers::AlarmClockController& alarmClockController)
  : m_system(system), alarmClockController {alarmClockController} {
  characteristicDefinition[0] = {.uuid = &acsTimeCharUuid.u,
                                 .access_cb = AlarmClockTimeCallback,
                                 .arg = this,
                                 .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                                 .val_handle = &alarmClockTimeHandle};
  characteristicDefinition[1] = {.uuid = &acsStateCharUuid.u,
                                 .access_cb = AlarmClockStateCallback,
                                 .arg = this,
                                 .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                                 .val_handle = &alarmClockStateHandle};
  characteristicDefinition[2] = {0};

  serviceDefinition[0] = {.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &acsUuid.u, .characteristics = characteristicDefinition};
  serviceDefinition[1] = {0};
  // TODO refactor to prevent this loop dependency (service depends on controller and controller depends on service)
  alarmClockController.SetService(this);
}

void Pinetime::Controllers::AlarmClockService::Init() {
  int res;
  res = ble_gatts_count_cfg(serviceDefinition);
  ASSERT(res == 0);

  res = ble_gatts_add_svcs(serviceDefinition);
  ASSERT(res == 0);
}

int Pinetime::Controllers::AlarmClockService::OnAlarmClockTime(struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    size_t notifSize = OS_MBUF_PKTLEN(ctxt->om);
    size_t bufferSize = notifSize;
    if (notifSize > MaxStringSize) {
      bufferSize = MaxStringSize;
    }

    char data[bufferSize + 1];
    os_mbuf_copydata(ctxt->om, 0, bufferSize, data);

    data[bufferSize] = '\0';

    char* s = &data[0];
    char* timeSplit = strtok(s, ":");
    uint8_t hour = strtol(timeSplit, nullptr, 10);
    timeSplit = strtok(NULL, " :");
    uint8_t minute = strtol(timeSplit, nullptr, 10);
    alarmClockController.SetAlarmTime(hour, minute);
    // Reset the alarm timer so the alarm will go off with the new time.
    if(alarmClockController.State() == AlarmClockController::AlarmState::Set) {
      alarmClockController.ScheduleAlarm();
    } else if (alarmClockController.State() == AlarmClockController::AlarmState::Alerting) {
      alarmClockController.ScheduleAlarm();
      alarmClockController.StopAlerting();
    }

  } else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
    uint8_t buffer[2] = {alarmClockController.Hours(), alarmClockController.Minutes()};

    int res = os_mbuf_append(ctxt->om, &buffer, sizeof(buffer));
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  return 0;
}

int Pinetime::Controllers::AlarmClockService::OnAlarmClockState(struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    char data;
    os_mbuf_copydata(ctxt->om, 0, 1, &data);
    if(data) {
      alarmClockController.ScheduleAlarm();
    } else {
      alarmClockController.DisableAlarm();
    }
  } else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
    uint8_t buffer[1];
    buffer[0] = GetAlarmStateAsInt(alarmClockController.State());

    int res = os_mbuf_append(ctxt->om, &buffer, sizeof(buffer));
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  return 0;
}

void Pinetime::Controllers::AlarmClockService::OnAlarmTimeChange(uint8_t hours, uint8_t minutes) {
  uint8_t buffer[2] = {hours, minutes};
  auto* om = ble_hs_mbuf_from_flat(buffer, 2);

  uint16_t connectionHandle = m_system.nimble().connHandle();

  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    return;
  }

  ble_gattc_notify_custom(connectionHandle, alarmClockTimeHandle, om);
}

//void Pinetime::Controllers::AlarmClockService::OnAlarmStateChange(Pinetime::Controllers::AlarmClockController::AlarmState alarmState) {
void Pinetime::Controllers::AlarmClockService::OnAlarmStateChange() {
  uint8_t buffer[1];
  buffer[0] = GetAlarmStateAsInt(alarmClockController.State());
  auto* om = ble_hs_mbuf_from_flat(buffer, 1);

  uint16_t connectionHandle = m_system.nimble().connHandle();

  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    return;
  }

  ble_gattc_notify_custom(connectionHandle, alarmClockStateHandle, om);
}