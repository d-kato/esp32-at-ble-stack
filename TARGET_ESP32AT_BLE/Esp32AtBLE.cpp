/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright (c) 2019 Renesas Electronics Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "Esp32AtBLE.h"

BLEInstanceBase *createBLEInstance(void)
{
    return (&ble::Esp32AtBLE::deviceInstance());
}

namespace ble {

Esp32AtBLE& Esp32AtBLE::deviceInstance()
{
    static Esp32AtBLE instance;
    return instance;
}

Esp32AtBLE::Esp32AtBLE(void) : initialized(false), instanceID(BLE::DEFAULT_INSTANCE), p_event_flg(NULL), _event_que_top(NULL)
{
    _esp = ESP32::getESP32Inst();
}

Esp32AtBLE::~Esp32AtBLE(void)
{
    if (p_event_flg) {
        delete p_event_flg;
    }
}

const char *Esp32AtBLE::getVersion(void)
{
    static char versionString[256];

    if (!_esp->get_version_info(versionString, 256)) {
        strncpy(versionString, "unknown", sizeof("unknown"));
    }

    return versionString;
}

ble_error_t Esp32AtBLE::init(
    BLE::InstanceID_t instanceID,
    FunctionPointerWithContext<BLE::InitializationCompleteCallbackContext *> initCallback
)
{
    BLE::InitializationCompleteCallbackContext context = {
        BLE::Instance(instanceID),
        BLE_ERROR_NONE
    };
    initCallback.call(&context);
    initialized = true;
    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtBLE::shutdown(void)
{
    return BLE_ERROR_NOT_IMPLEMENTED;
}

void Esp32AtBLE::waitForEvent(void)
{
    if (p_event_flg == NULL) {
        p_event_flg = new EventFlags;
    }
    processEvents();
    if (p_event_flg) {
        p_event_flg->wait_all(1);
    }
}

void Esp32AtBLE::processEvents()
{
    EventQue_t * p_event;

    _esp->ble_process_oob(1, false);

    p_event = _event_que_top;
    if (p_event == NULL) {
        return;
    }
    core_util_critical_section_enter();
    _event_que_top = p_event->p_next;
    core_util_critical_section_exit();

    switch (p_event->type) {
        case EVENT_TYPE_COMMON:
            getGap().doEvent(p_event->id, p_event->arg);
            break;
        case EVENT_TYPE_SERVER:
            getGattServer().doEvent(p_event->id, p_event->arg);
            break;
        case EVENT_TYPE_CLIENT:
            getGattClient().doEvent(p_event->id, p_event->arg);
            break;
        default:
            break;
    }

    delete p_event;
}

void Esp32AtBLE::signalEventsToProcess(BLE::InstanceID_t id)
{
    if (p_event_flg) {
        p_event_flg->set(1);
    }
    BLEInstanceBase::signalEventsToProcess(id);
}

bool Esp32AtBLE::setEvent(uint32_t type, uint32_t id, void * arg)
{
    EventQue_t * p_new_event = new EventQue_t;

    if (p_new_event == NULL) {
        return false;
    }

    p_new_event->type   = type;
    p_new_event->id     = id;
    p_new_event->arg    = arg;
    p_new_event->p_next = NULL;

    core_util_critical_section_enter();
    if (_event_que_top == NULL) {
        _event_que_top = p_new_event;
    } else {
        EventQue_t * p_wk_event = _event_que_top;

        while (p_wk_event->p_next != NULL) {
            p_wk_event  = p_wk_event->p_next;
        }
        p_wk_event->p_next = p_new_event;
    }
    core_util_critical_section_exit();

    signalEventsToProcess(::BLE::DEFAULT_INSTANCE);

    return true;
}

} // namespace ble

