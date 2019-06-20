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

#ifndef _ESP32AT_GATT_CLIENT_H_
#define _ESP32AT_GATT_CLIENT_H_

#include <stddef.h>

#include "ble/GattClient.h"
#include "ESP32.h"

namespace ble {
namespace atcmd {

//class Esp32AtGattClient : public ble::interface::GattClient<Esp32AtGattClient>
class Esp32AtGattClient : public interface::GattClient<Esp32AtGattClient>
{
public:
    static Esp32AtGattClient &getInstance();

    /**
     * @see GattClient::launchServiceDiscovery
     */
    ble_error_t launchServiceDiscovery_(
        connection_handle_t connection_handle,
        ServiceDiscovery::ServiceCallback_t service_callback,
        ServiceDiscovery::CharacteristicCallback_t characteristic_callback,
        const UUID& matching_service_uuid,
        const UUID& matching_characteristic_uuid
    );

    /**
     * @see GattClient::isServiceDiscoveryActive
     */
    bool isServiceDiscoveryActive_() const;

    /**
     * @see GattClient::terminateServiceDiscovery
     */
    void terminateServiceDiscovery_();

    /**
     * @see GattClient::read
     */
    ble_error_t read_(
        connection_handle_t connection_handle,
        GattAttribute::Handle_t attribute_handle,
        uint16_t offset
    ) const;

    /**
     * @see GattClient::write
     */
    ble_error_t write_(
        GattClient::WriteOp_t cmd,
        connection_handle_t connection_handle,
        GattAttribute::Handle_t attribute_handle,
        size_t length,
        const uint8_t* value
    ) const;

    /**
     * @see GattClient::onServiceDiscoveryTermination
	 */
    void onServiceDiscoveryTermination_(
        ServiceDiscovery::TerminationCallback_t callback
    );

    /* event process */
    void doEvent(uint32_t id, void * arg);

private:
    typedef struct {
        connection_handle_t connection_handle;
        ServiceDiscovery::ServiceCallback_t service_callback;
        ServiceDiscovery::CharacteristicCallback_t characteristic_callback;
        UUID matching_service_uuid;
        UUID matching_characteristic_uuid;
    } event_launchServiceDiscovery_t;

    typedef struct {
        connection_handle_t connection_handle;
        GattAttribute::Handle_t attribute_handle;
        uint16_t offset;
    } event_read_t;

    typedef struct {
        GattClient::WriteOp_t cmd;
        connection_handle_t connection_handle;
        GattAttribute::Handle_t attribute_handle;
        size_t length;
        const uint8_t* value;
    } event_write_t;

    #define EVENT_LAUNCH_SERVICE_DISCOVERY     1
    #define EVENT_READ                         2
    #define EVENT_WRITE                        3

    ESP32 *_esp;
    ServiceDiscovery::TerminationCallback_t _termination_callback;
    bool _is_service_discovery;

    Esp32AtGattClient();
    void _event_launchServiceDiscovery(event_launchServiceDiscovery_t * param);
    void _event_read(event_read_t * param);
    void _event_write(event_write_t * param);

};

} // namespace atcmd
} // namespace ble

#endif /* _ESP32AT_GATT_CLIENT_H_ */
