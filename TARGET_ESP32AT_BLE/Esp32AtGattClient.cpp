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

#include "Esp32AtGattClient.h"
#include "mbed.h"
#include "Esp32AtBLE.h"
#include "Esp32AtGap.h"
#include <ble/DiscoveredService.h>
#include <ble/DiscoveredCharacteristic.h>

namespace ble {
namespace atcmd {

struct characteristic_t : DiscoveredCharacteristic {
    characteristic_t(
        GattClient* _client,
        connection_handle_t _connection_handle,
        UUID _uuid,
        uint16_t _decl_handle,
        int     _idx,
        uint8_t _props
    ) : DiscoveredCharacteristic() {
        gattc = _client;
        uuid = _uuid;
        props = get_properties(_props);
        declHandle = _decl_handle;
        valueHandle = (GattAttribute::Handle_t)_idx;
        lastHandle = 0x0000;
        connHandle = _connection_handle;
    }

    static DiscoveredCharacteristic::Properties_t get_properties(uint8_t value) {
        DiscoveredCharacteristic::Properties_t result;
        result._broadcast = (value & (1 << 0)) ? true : false;
        result._read = (value & (1 << 1)) ? true : false;
        result._writeWoResp = (value & (1 << 2))  ? true : false;
        result._write = (value & (1 << 3))  ? true : false;
        result._notify = (value & (1 << 4))  ? true : false;
        result._indicate = (value & (1 << 5))  ? true : false;
        result._authSignedWrite = (value & (1 << 6))  ? true : false;
        return result;
    }
};


Esp32AtGattClient &Esp32AtGattClient::getInstance()
{
    static Esp32AtGattClient m_instance;
    return m_instance;
}

Esp32AtGattClient::Esp32AtGattClient() : _termination_callback(), _is_service_discovery(false)
{
    _esp = ESP32::getESP32Inst();
    _esp->ble_set_role(INIT_CLIENT_ROLE);
}

bool Esp32AtGattClient::isServiceDiscoveryActive_() const
{
    return _is_service_discovery;
}

void Esp32AtGattClient::terminateServiceDiscovery_(void)
{
    _is_service_discovery = false;
}

void Esp32AtGattClient::onServiceDiscoveryTermination_(
    ServiceDiscovery::TerminationCallback_t callback)
{
    _termination_callback = callback;
}

ble_error_t Esp32AtGattClient::launchServiceDiscovery_(
    connection_handle_t connection_handle,
    ServiceDiscovery::ServiceCallback_t service_callback,
    ServiceDiscovery::CharacteristicCallback_t characteristic_callback,
    const UUID& matching_service_uuid,
    const UUID& matching_characteristic_uuid
)
{
    event_launchServiceDiscovery_t * param = new event_launchServiceDiscovery_t;

    if (param == NULL) {
        return BLE_ERROR_NO_MEM;
    }

    param->connection_handle            = connection_handle;
    param->service_callback             = service_callback;
    param->characteristic_callback      = characteristic_callback;
    param->matching_service_uuid        = matching_service_uuid;
    param->matching_characteristic_uuid = matching_characteristic_uuid;

    if (!Esp32AtBLE::deviceInstance().setEvent(
            EVENT_TYPE_CLIENT, EVENT_LAUNCH_SERVICE_DISCOVERY, (void *)param)) {
        delete param;
        return BLE_ERROR_NO_MEM;
    }

    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGattClient::read_(
    connection_handle_t connection_handle,
    GattAttribute::Handle_t attribute_handle,
    uint16_t offset
) const
{
    event_read_t * param = new event_read_t;

    if (param == NULL) {
        return BLE_ERROR_NO_MEM;
    }

    param->connection_handle            = connection_handle;
    param->attribute_handle             = attribute_handle;
    param->offset                       = offset;

    if (!Esp32AtBLE::deviceInstance().setEvent(
            EVENT_TYPE_CLIENT, EVENT_READ, (void *)param)) {
        delete param;
        return BLE_ERROR_NO_MEM;
    }

    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGattClient::write_(
    GattClient::WriteOp_t cmd,
    connection_handle_t connection_handle,
    GattAttribute::Handle_t attribute_handle,
    size_t length,
    const uint8_t* value
) const
{
    event_write_t * param = new event_write_t;

    if (param == NULL) {
        return BLE_ERROR_NO_MEM;
    }

    param->cmd                          = cmd;
    param->connection_handle            = connection_handle;
    param->attribute_handle             = attribute_handle;
    param->length                       = length;
    param->value                        = value;

    if (!Esp32AtBLE::deviceInstance().setEvent(
            EVENT_TYPE_CLIENT, EVENT_WRITE, (void *)param)) {
        delete param;
        return BLE_ERROR_NO_MEM;
    }

    return BLE_ERROR_NONE;
}

void Esp32AtGattClient::doEvent(uint32_t id, void * arg)
{
    switch (id) {
        case EVENT_LAUNCH_SERVICE_DISCOVERY:
            _event_launchServiceDiscovery((event_launchServiceDiscovery_t *)arg);
            delete (event_launchServiceDiscovery_t *)arg;
            break;
        case EVENT_READ:
            _event_read((event_read_t *)arg);
            delete (event_read_t *)arg;
            break;
        case EVENT_WRITE:
            _event_write((event_write_t *)arg);
            delete (event_write_t *)arg;
            break;
        default:
            break;
    }
}

void Esp32AtGattClient::_event_launchServiceDiscovery(event_launchServiceDiscovery_t * param)
{
    ESP32::ble_primary_service_t services[8];
    ESP32::ble_discovers_char_t  discovers_char[8];
    int services_num;
    int characteristic_num;

    _is_service_discovery = true;
    services_num = 8;
    if (_esp->ble_discovery_service((int)param->connection_handle, &services[0], &services_num)) {
        for (int i = 0; i < services_num; i++) {
            if (!_is_service_discovery) {
                break;
            }
            if ((UUID)services[i].srv_uuid == param->matching_service_uuid) {
                characteristic_num = 8;
                _esp->ble_discovery_characteristics((int)param->connection_handle, services[i].srv_index,
                    &discovers_char[0], &characteristic_num,
                    NULL, NULL
                );

                if (param->service_callback != NULL) {
                    DiscoveredService discovered_service;
                    discovered_service.setup(
                        (UUID)services[i].srv_uuid,
                        (services[i].srv_index << 8) + 1,
                        (services[i].srv_index << 8) + characteristic_num
                    );
                    param->service_callback(&discovered_service);
                }

                if (param->characteristic_callback != NULL) {
                    for (int j = 0; j < characteristic_num; j++) {
                        if ((UUID)discovers_char[j].char_uuid == param->matching_characteristic_uuid) {
                            characteristic_t characteristic(
                                (GattClient*)this, param->connection_handle, (UUID)discovers_char[j].char_uuid, 0, 
                                ((services[i].srv_index << 8) + discovers_char[j].char_index),
                                discovers_char[j].char_prop
                            );
                            param->characteristic_callback(&characteristic);
                            break;
                        }
                    }
                }
                break;
            }
        }
    }
    _is_service_discovery = false;
    if (_termination_callback) {
        _termination_callback(param->connection_handle);
    }
}

void Esp32AtGattClient::_event_read(event_read_t * param)
{
    uint8_t * recv_buf = new uint8_t[512];
    int32_t recv_size;
    GattReadCallbackParams response;

    recv_size = _esp->ble_read_characteristic(
                    param->connection_handle, ((param->attribute_handle >> 8) & 0xFF),
                    (param->attribute_handle & 0xFF), recv_buf, 512);
    response.connHandle = param->connection_handle;
    response.handle     = param->attribute_handle;
    response.status     = BLE_ERROR_NONE;

    if ((recv_size > 0) && (param->offset < recv_size)) {
        response.data       = &recv_buf[param->offset];
        response.len        = recv_size - param->offset;
    } else {
        response.data       = NULL;
        response.error_code = 0x00;
    }
    onDataReadCallbackChain(&response);

    delete [] recv_buf;
}

void Esp32AtGattClient::_event_write(event_write_t * param)
{
    ble_error_t status = BLE_ERROR_NONE;
    uint8_t error_code = 0x00;

    if (!_esp->ble_write_characteristic(
             param->connection_handle, ((param->attribute_handle >> 8) & 0xFF),
             (param->attribute_handle & 0xFF), param->value, param->length))
    {
        status = BLE_ERROR_INVALID_STATE;
        error_code = 0xFF;
    }

    GattWriteCallbackParams response = {
        param->connection_handle,
        param->attribute_handle,
        GattWriteCallbackParams::OP_WRITE_REQ,
        status,
        error_code
    };
    onDataWriteCallbackChain(&response);
}

} // namespace atcmd
} // namespace ble
