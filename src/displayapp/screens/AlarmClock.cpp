/*  Copyright (C) 2021 mruss77, Florian

    This file is part of InfiniTime.

    InfiniTime is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    InfiniTime is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "displayapp/screens/AlarmClock.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/Symbols.h"

using namespace Pinetime::Applications::Screens;
using Pinetime::Controllers::AlarmClockController;

static void btnEventHandler(lv_obj_t* obj, lv_event_t event) {
  auto* screen = static_cast<AlarmClock*>(obj->user_data);
  screen->OnButtonEvent(obj, event);
}

static void StopAlarmTaskCallback(lv_task_t* task) {
  auto* screen = static_cast<AlarmClock*>(task->user_data);
  screen->StopAlerting();
}

AlarmClock::AlarmClock(DisplayApp* app,
             Controllers::AlarmClockController& alarmClockController,
             Pinetime::Controllers::Settings& settingsController,
             System::SystemTask& systemTask)
  : Screen(app), alarmClockController {alarmClockController}, settingsController {settingsController}, systemTask {systemTask} {

  time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_76);
  lv_obj_set_style_local_text_color(time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0xb0, 0xb0, 0xb0));

  alarmHours = alarmClockController.Hours();
  alarmMinutes = alarmClockController.Minutes();
  lv_label_set_text_fmt(time, "%02hhu:%02hhu", alarmHours, alarmMinutes);

  lv_obj_align(time, lv_scr_act(), LV_ALIGN_CENTER, 0, -25);

  lblampm = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(lblampm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_set_style_local_text_color(lblampm, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0xb0, 0xb0, 0xb0));
  lv_label_set_text_static(lblampm, "  ");
  lv_label_set_align(lblampm, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(lblampm, lv_scr_act(), LV_ALIGN_CENTER, 0, 30);

  btnHoursUp = lv_btn_create(lv_scr_act(), nullptr);
  btnHoursUp->user_data = this;
  lv_obj_set_event_cb(btnHoursUp, btnEventHandler);
  lv_obj_set_size(btnHoursUp, 60, 40);
  lv_obj_align(btnHoursUp, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 20, -85);
  txtHrUp = lv_label_create(btnHoursUp, nullptr);
  lv_label_set_text_static(txtHrUp, "+");

  btnHoursDown = lv_btn_create(lv_scr_act(), nullptr);
  btnHoursDown->user_data = this;
  lv_obj_set_event_cb(btnHoursDown, btnEventHandler);
  lv_obj_set_size(btnHoursDown, 60, 40);
  lv_obj_align(btnHoursDown, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 20, 35);
  txtHrDown = lv_label_create(btnHoursDown, nullptr);
  lv_label_set_text_static(txtHrDown, "-");

  btnMinutesUp = lv_btn_create(lv_scr_act(), nullptr);
  btnMinutesUp->user_data = this;
  lv_obj_set_event_cb(btnMinutesUp, btnEventHandler);
  lv_obj_set_size(btnMinutesUp, 60, 40);
  lv_obj_align(btnMinutesUp, lv_scr_act(), LV_ALIGN_IN_RIGHT_MID, -20, -85);
  txtMinUp = lv_label_create(btnMinutesUp, nullptr);
  lv_label_set_text_static(txtMinUp, "+");

  btnMinutesDown = lv_btn_create(lv_scr_act(), nullptr);
  btnMinutesDown->user_data = this;
  lv_obj_set_event_cb(btnMinutesDown, btnEventHandler);
  lv_obj_set_size(btnMinutesDown, 60, 40);
  lv_obj_align(btnMinutesDown, lv_scr_act(), LV_ALIGN_IN_RIGHT_MID, -20, 35);
  txtMinDown = lv_label_create(btnMinutesDown, nullptr);
  lv_label_set_text_static(txtMinDown, "-");

  btnStop = lv_btn_create(lv_scr_act(), nullptr);
  btnStop->user_data = this;
  lv_obj_set_event_cb(btnStop, btnEventHandler);
  lv_obj_set_size(btnStop, 115, 50);
  lv_obj_align(btnStop, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_local_bg_color(btnStop, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
  txtStop = lv_label_create(btnStop, nullptr);
  lv_label_set_text_static(txtStop, Symbols::stop);
  lv_obj_set_hidden(btnStop, true);

  btnRecur = lv_btn_create(lv_scr_act(), nullptr);
  btnRecur->user_data = this;
  lv_obj_set_event_cb(btnRecur, btnEventHandler);
  lv_obj_set_size(btnRecur, 115, 50);
  lv_obj_align(btnRecur, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
  txtRecur = lv_label_create(btnRecur, nullptr);
  SetRecurButtonState();

  btnInfo = lv_btn_create(lv_scr_act(), nullptr);
  btnInfo->user_data = this;
  lv_obj_set_event_cb(btnInfo, btnEventHandler);
  lv_obj_set_size(btnInfo, 50, 40);
  lv_obj_align(btnInfo, lv_scr_act(), LV_ALIGN_CENTER, 0, -85);
  txtInfo = lv_label_create(btnInfo, nullptr);
  lv_label_set_text_static(txtInfo, "i");

  enableSwitch = lv_switch_create(lv_scr_act(), nullptr);
  enableSwitch->user_data = this;
  lv_obj_set_event_cb(enableSwitch, btnEventHandler);
  lv_obj_set_size(enableSwitch, 100, 50);
  // Align to the center of 115px from edge
  lv_obj_align(enableSwitch, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 7, 0);

  UpdateAlarmTime();

  if (alarmClockController.State() == Controllers::AlarmClockController::AlarmState::Alerting) {
    SetAlerting();
  } else {
    SetSwitchState(LV_ANIM_OFF);
  }

  taskRefresh = lv_task_create(RefreshTaskCallback, 1000, LV_TASK_PRIO_MID, this);
}

AlarmClock::~AlarmClock() {
  if (alarmClockController.State() == AlarmClockController::AlarmState::Alerting) {
    StopAlerting();
  }
  lv_obj_clean(lv_scr_act());
}

void AlarmClock::Refresh() {
  if (alarmMinutes != alarmClockController.Minutes()) {
    alarmMinutes = alarmClockController.Minutes();
  } else if (alarmHours != alarmClockController.Hours()) {
    alarmHours = alarmClockController.Hours();
  }
  UpdateAlarmTime();
}

void AlarmClock::OnButtonEvent(lv_obj_t* obj, lv_event_t event) {
  using Pinetime::Controllers::AlarmClockController;
  if (event == LV_EVENT_CLICKED) {
    if (obj == btnStop) {
      StopAlerting();
      return;
    }
    if (obj == btnInfo) {
      ShowInfo();
      return;
    }
    if (obj == btnMessage) {
      HideInfo();
      return;
    }
    if (obj == enableSwitch) {
      if (lv_switch_get_state(enableSwitch)) {
        alarmClockController.ScheduleAlarm();
      } else {
        alarmClockController.DisableAlarm();
      }
      return;
    }
    // If any other button was pressed, disable the alarm
    // this is to make it clear that the alarm won't be set until it is turned back on
    if (alarmClockController.State() == AlarmClockController::AlarmState::Set) {
      alarmClockController.DisableAlarm();
      lv_switch_off(enableSwitch, LV_ANIM_ON);
    }
    if (obj == btnMinutesUp) {
      if (alarmMinutes >= 59) {
        alarmMinutes = 0;
      } else {
        alarmMinutes++;
      }
      UpdateAlarmTime();
      return;
    }
    if (obj == btnMinutesDown) {
      if (alarmMinutes == 0) {
        alarmMinutes = 59;
      } else {
        alarmMinutes--;
      }
      UpdateAlarmTime();
      return;
    }
    if (obj == btnHoursUp) {
      if (alarmHours >= 23) {
        alarmHours = 0;
      } else {
        alarmHours++;
      }
      UpdateAlarmTime();
      return;
    }
    if (obj == btnHoursDown) {
      if (alarmHours == 0) {
        alarmHours = 23;
      } else {
        alarmHours--;
      }
      UpdateAlarmTime();
      return;
    }
    if (obj == btnRecur) {
      ToggleRecurrence();
    }
  }
}

bool AlarmClock::OnButtonPushed() {
  if (txtMessage != nullptr && btnMessage != nullptr) {
    HideInfo();
    return true;
  }
  if (alarmClockController.State() == AlarmClockController::AlarmState::Alerting) {
    StopAlerting();
    return true;
  }
  return false;
}

bool AlarmClock::OnTouchEvent(Pinetime::Applications::TouchEvents event) {
  // Don't allow closing the screen by swiping while the alarm is alerting
  return alarmClockController.State() == AlarmClockController::AlarmState::Alerting && event == TouchEvents::SwipeDown;
}

void AlarmClock::UpdateAlarmTime() {
  if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
    switch (alarmHours) {
      case 0:
        lv_label_set_text_static(lblampm, "AM");
        lv_label_set_text_fmt(time, "%02d:%02d", 12, alarmMinutes);
        break;
      case 1 ... 11:
        lv_label_set_text_static(lblampm, "AM");
        lv_label_set_text_fmt(time, "%02d:%02d", alarmHours, alarmMinutes);
        break;
      case 12:
        lv_label_set_text_static(lblampm, "PM");
        lv_label_set_text_fmt(time, "%02d:%02d", 12, alarmMinutes);
        break;
      case 13 ... 23:
        lv_label_set_text_static(lblampm, "PM");
        lv_label_set_text_fmt(time, "%02d:%02d", alarmHours - 12, alarmMinutes);
        break;
    }
  } else {
    lv_label_set_text_fmt(time, "%02d:%02d", alarmHours, alarmMinutes);
  }
  alarmClockController.SetAlarmTime(alarmHours, alarmMinutes);
}

void AlarmClock::SetAlerting() {
  lv_obj_set_hidden(enableSwitch, true);
  lv_obj_set_hidden(btnStop, false);
  taskStopAlarm = lv_task_create(StopAlarmTaskCallback, pdMS_TO_TICKS(60 * 1000), LV_TASK_PRIO_MID, this);
  systemTask.PushMessage(System::Messages::DisableSleeping);
}

void AlarmClock::StopAlerting() {
  alarmClockController.StopAlerting();
  SetSwitchState(LV_ANIM_OFF);
  if (taskStopAlarm != nullptr) {
    lv_task_del(taskStopAlarm);
    taskStopAlarm = nullptr;
  }
  systemTask.PushMessage(System::Messages::EnableSleeping);
  lv_obj_set_hidden(enableSwitch, false);
  lv_obj_set_hidden(btnStop, true);
}

void AlarmClock::SetSwitchState(lv_anim_enable_t anim) {
  switch (alarmClockController.State()) {
    case AlarmClockController::AlarmState::Set:
      lv_switch_on(enableSwitch, anim);
      break;
    case AlarmClockController::AlarmState::Not_Set:
      lv_switch_off(enableSwitch, anim);
      break;
    default:
      break;
  }
}

void AlarmClock::ShowInfo() {
  btnMessage = lv_btn_create(lv_scr_act(), nullptr);
  btnMessage->user_data = this;
  lv_obj_set_event_cb(btnMessage, btnEventHandler);
  lv_obj_set_height(btnMessage, 200);
  lv_obj_set_width(btnMessage, 150);
  lv_obj_align(btnMessage, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
  txtMessage = lv_label_create(btnMessage, nullptr);
  lv_obj_set_style_local_bg_color(btnMessage, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_NAVY);

  if (alarmClockController.State() == AlarmClockController::AlarmState::Set) {
    auto timeToAlarm = alarmClockController.SecondsToAlarm();

    auto daysToAlarm = timeToAlarm / 86400;
    auto hrsToAlarm = (timeToAlarm % 86400) / 3600;
    auto minToAlarm = (timeToAlarm % 3600) / 60;
    auto secToAlarm = timeToAlarm % 60;

    lv_label_set_text_fmt(txtMessage,
                          "Time to\nalarm:\n%2lu Days\n%2lu Hours\n%2lu Minutes\n%2lu Seconds",
                          daysToAlarm,
                          hrsToAlarm,
                          minToAlarm,
                          secToAlarm);
  } else {
    lv_label_set_text_static(txtMessage, "Alarm\nis not\nset.");
  }
}

void AlarmClock::HideInfo() {
  lv_obj_del(btnMessage);
  txtMessage = nullptr;
  btnMessage = nullptr;
}

void AlarmClock::SetRecurButtonState() {
  using Pinetime::Controllers::AlarmClockController;
  switch (alarmClockController.Recurrence()) {
    case AlarmClockController::RecurType::None:
      lv_label_set_text_static(txtRecur, "ONCE");
      break;
    case AlarmClockController::RecurType::Daily:
      lv_label_set_text_static(txtRecur, "DAILY");
      break;
    case AlarmClockController::RecurType::Weekdays:
      lv_label_set_text_static(txtRecur, "MON-FRI");
  }
}

void AlarmClock::ToggleRecurrence() {
  using Pinetime::Controllers::AlarmClockController;
  switch (alarmClockController.Recurrence()) {
    case AlarmClockController::RecurType::None:
      alarmClockController.SetRecurrence(AlarmClockController::RecurType::Daily);
      break;
    case AlarmClockController::RecurType::Daily:
      alarmClockController.SetRecurrence(AlarmClockController::RecurType::Weekdays);
      break;
    case AlarmClockController::RecurType::Weekdays:
      alarmClockController.SetRecurrence(AlarmClockController::RecurType::None);
  }
  SetRecurButtonState();
}
