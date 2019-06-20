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

#ifndef _ESP32AT_GATT_SERVER_H_
#define _ESP32AT_GATT_SERVER_H_

#include <stddef.h>

#include "blecommon.h"
#include "GattServer.h"

#include "ESP32.h"

class Esp32AtGattServer : public ble::interface::GattServer<Esp32AtGattServer>
{
public:
    static Esp32AtGattServer &getInstance();

    /* Functions that must be implemented from GattServer */
    virtual ble_error_t addService_(GattService &);

    virtual ble_error_t read_(GattAttribute::Handle_t attributeHandle, uint8_t buffer[], uint16_t *lengthP);
    virtual ble_error_t read_(Gap::Handle_t connectionHandle, GattAttribute::Handle_t attributeHandle,
                             uint8_t buffer[], uint16_t *lengthP);
    virtual ble_error_t write_(GattAttribute::Handle_t, const uint8_t[], uint16_t, bool localOnly = false);
    virtual ble_error_t write_(Gap::Handle_t connectionHandle, GattAttribute::Handle_t,
                              const uint8_t[], uint16_t, bool localOnly = false);

    /* event process */
    void doEvent(uint32_t id, void * arg);

private:
    typedef struct {
        uint8_t *    data;
        uint16_t     max_len;
        uint16_t     cur_len;
    } characteristic_buf_t;

    ESP32 *_esp;
    characteristic_buf_t * characteristic_buf;
    uint16_t characteristic_count;

    Esp32AtGattServer();
    const Esp32AtGattServer& operator=(const Esp32AtGattServer &);

    void write_cb(ESP32::ble_packet_t * ble_packet);
};

#endif /* _ESP32AT_GATT_SERVER_H_ */
