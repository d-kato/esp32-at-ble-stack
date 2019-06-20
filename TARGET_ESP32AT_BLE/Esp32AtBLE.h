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

#ifndef _ESP32ATBLE_H_
#define _ESP32ATBLE_H_

#include "ble/BLE.h"
#include "ble/blecommon.h"
#include "ble/BLEInstanceBase.h"
#include "Esp32AtGap.h"
#include "Esp32AtGattClient.h"
#include "Esp32AtGattServer.h"
#include "Esp32AtSecurityManager.h"

#include "ESP32.h"

namespace ble {

class Esp32AtBLE : public BLEInstanceBase
{
public:
    Esp32AtBLE(void);
    virtual ~Esp32AtBLE(void);

    /**
     * Access to the singleton containing the implementation of BLEInstanceBase
     * for the Esp32At stack.
     */
    static Esp32AtBLE& deviceInstance();

    virtual ble_error_t init(BLE::InstanceID_t instanceID, FunctionPointerWithContext<BLE::InitializationCompleteCallbackContext *> initCallback);
    virtual bool        hasInitialized(void) const {
        return initialized;
    }
    virtual ble_error_t shutdown(void);
    virtual const char *getVersion(void);

    virtual impl::Esp32AtGapImpl &getGap() {
        return impl::Esp32AtGapImpl::getInstance();
    };
    virtual const impl::Esp32AtGapImpl &getGap() const {
        return const_cast<const impl::Esp32AtGapImpl&>(impl::Esp32AtGapImpl::getInstance());
    };

    virtual Esp32AtGattServer &getGattServer() {
        return Esp32AtGattServer::getInstance();
    };
    virtual const Esp32AtGattServer &getGattServer() const {
        return Esp32AtGattServer::getInstance();
    };

    virtual impl::Esp32AtGattClientImpl &getGattClient() {
        return impl::Esp32AtGattClientImpl::getInstance();
    };

    virtual SecurityManager &getSecurityManager() {
        return Esp32AtSecurityManager::getInstance();
    };
    virtual const SecurityManager &getSecurityManager() const {
        return Esp32AtSecurityManager::getInstance();
    };

    virtual void waitForEvent(void);

    virtual void processEvents();

    virtual void signalEventsToProcess(BLE::InstanceID_t id);

    /* event process */
    #define EVENT_TYPE_COMMON   0
    #define EVENT_TYPE_SERVER   1
    #define EVENT_TYPE_CLIENT   2

    struct EventQue {
        uint32_t          type;
        uint32_t          id;
        void *            arg;
        struct EventQue * p_next;
    };
    typedef struct EventQue EventQue_t;

    bool setEvent(uint32_t type, uint32_t id, void * arg);

private:
    bool              initialized;
    BLE::InstanceID_t instanceID;
    ESP32 *_esp;
    EventFlags *      p_event_flg;
    EventQue_t *      _event_que_top;

};

} // namespace ble

#endif /* _ESP32ATBLE_H_ */
