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

#include "Esp32AtGattServer.h"
#include "mbed.h"
#include "Esp32AtGap.h"

Esp32AtGattServer &Esp32AtGattServer::getInstance()
{
    static Esp32AtGattServer m_instance;
    return m_instance;
}

Esp32AtGattServer::Esp32AtGattServer() : characteristic_buf(NULL), characteristic_count(0)
{
    _esp = ESP32::getESP32Inst();
    _esp->ble_set_role(INIT_SERVER_ROLE);
}

ble_error_t Esp32AtGattServer::addService_(GattService &service)
{
    ESP32::gatt_service_t * service_base;
    ESP32::gatt_service_t * service_list;

    // Determine the attribute list length
    unsigned int attListLen = 1;
    for (int i = 0; i < service.getCharacteristicCount(); i++) {
        attListLen += 2;
        GattCharacteristic *p_char = service.getCharacteristic(i);

        bool cccCreated = false;

        for (int j = 0; i < p_char->getDescriptorCount(); j++) {
            GattAttribute *p_att = p_char->getDescriptor(j);
            if (p_att->getUUID() == UUID(BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG)) {
                cccCreated = true;
            }
            attListLen++;
        }
        if (!cccCreated && (p_char->getProperties() & (GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY
                                                     | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE))) {
            attListLen++;
        }
    }

    characteristic_count = service.getCharacteristicCount();
    characteristic_buf = new characteristic_buf_t[characteristic_count];
    if (characteristic_buf == NULL) {
        return BLE_ERROR_BUFFER_OVERFLOW;
    }

    service_base = new ESP32::gatt_service_t[attListLen];
    if (service_base == NULL) {
        delete [] characteristic_buf;
        return BLE_ERROR_BUFFER_OVERFLOW;
    }

    service_list = service_base;

    // Primary Service
    service_list->uuid_type   = 1;
    service_list->uuid.data   = BLE_UUID_SERVICE_PRIMARY;
    service_list->uuid_size   = 2;
    service_list->val_max_len = 2;
    service_list->permissions = 1;
    if (service.getUUID().shortOrLong() == UUID::UUID_TYPE_LONG) {
        service_list->value_type = 0;
        service_list->value.addr = (uint8_t *)service.getUUID().getBaseUUID();
    } else {
        service_list->value_type = 1;
        service_list->value.data = *(uint16_t *)service.getUUID().getBaseUUID();
    }
    service_list->value_size  = service.getUUID().getLen();
    service_list++;

    // Add characteristics to the service
    for (int i = 0; i < service.getCharacteristicCount(); i++) {
        GattCharacteristic *p_char = service.getCharacteristic(i);

        /* Skip any incompletely defined, read-only characteristics. */
        if ((p_char->getValueAttribute().getValuePtr() == NULL) &&
            (p_char->getValueAttribute().getLength() == 0) &&
            (p_char->getProperties() == GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ)) {
            continue;
        }

        // Create Characteristic Attribute
        service_list->uuid_type   = 1;
        service_list->uuid.data   = BLE_UUID_CHARACTERISTIC;
        service_list->uuid_size   = 2;
        service_list->val_max_len = 1;
        service_list->permissions = 1;
        service_list->value_type  = 1;
        service_list->value.data  = p_char->getProperties();
        service_list->value_size  = 1;
        service_list++;

        // Create Value Attribute
        if (p_char->getValueAttribute().getUUID().shortOrLong() == UUID::UUID_TYPE_LONG) {
            service_list->uuid_type = 0;
            service_list->uuid.addr = (uint8_t *)p_char->getValueAttribute().getUUID().getBaseUUID();
        } else {
            service_list->uuid_type = 1;
            service_list->uuid.data = *(uint16_t *)p_char->getValueAttribute().getUUID().getBaseUUID();
        }
        service_list->uuid_size   = p_char->getValueAttribute().getUUID().getLen();
        service_list->val_max_len = p_char->getValueAttribute().getMaxLength();
        service_list->permissions = 0;
        if (p_char->getProperties() & GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ)  {
            service_list->permissions |= 0x01;
        }
        if (p_char->getProperties() & GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE) {
            service_list->permissions |= 0x10;
        }
        service_list->value_type  = 0;
        service_list->value.addr  = p_char->getValueAttribute().getValuePtr();
        service_list->value_size  = *p_char->getValueAttribute().getLengthPtr();

        characteristic_buf[i].max_len = service_list->val_max_len;
        characteristic_buf[i].data = new uint8_t[service_list->val_max_len];
        if (characteristic_buf[i].data != NULL) {
            characteristic_buf[i].cur_len = service_list->value_size;
            memcpy(characteristic_buf[i].data, service_list->value.addr, service_list->value_size);
        }
        service_list++;

        bool cccCreated = false;

        for (int j = 0; i < p_char->getDescriptorCount(); j++) {
            GattAttribute *p_att = p_char->getDescriptor(j);

            if (p_att->getUUID().shortOrLong() == UUID::UUID_TYPE_LONG) {
                service_list->uuid_type = 0;
                service_list->uuid.addr = (uint8_t *)p_att->getUUID().getBaseUUID();
            } else {
                service_list->uuid_type = 1;
                service_list->uuid.data = *(uint16_t *)p_att->getUUID().getBaseUUID();
            }
            service_list->uuid_size   = p_att->getUUID().getLen();
            service_list->val_max_len = p_att->getMaxLength();
            service_list->value_type  = 0;
            service_list->value.addr  = p_att->getValuePtr();
            service_list->value_size  = *p_att->getLengthPtr();
            if (p_att->getUUID() == UUID(BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG)) {
                cccCreated = true;
                service_list->permissions = 0x11; /* read write */
            } else {
                service_list->permissions = 0;
            }
            service_list++;
        }

        if (!cccCreated && (p_char->getProperties() & (GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY
                                                     | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE))) {
            /* There was not a CCCD included in the descriptors, but this
             * characteristic is notifiable and/or indicatable. A CCCD is
             * required so create one now.
             */

            service_list->uuid_type   = 1;
            service_list->uuid.data   = BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG;
            service_list->uuid_size   = 2;
            service_list->val_max_len = sizeof(uint16_t);
            service_list->value_type  = 1;
            service_list->value.data  = 0x0000;
            service_list->value_size  = sizeof(uint16_t);
            service_list->permissions = 0x11; /* read write */
            service_list++;
        }
    }

    _esp->ble_attach_write(callback(this, &Esp32AtGattServer::write_cb));
    _esp->ble_set_service(service_base, attListLen);

    delete [] service_base;

    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGattServer::read_(
    GattAttribute::Handle_t attributeHandle, uint8_t buffer[], uint16_t *const lengthP)
{
    if (attributeHandle >= characteristic_count) {
        return BLE_ERROR_PARAM_OUT_OF_RANGE;
    }
    if ((buffer == NULL) || (lengthP == NULL)) {
        return BLE_ERROR_PARAM_OUT_OF_RANGE;
    }

    uint16_t copy_len = characteristic_buf[attributeHandle].cur_len;

    if (copy_len > *lengthP) {
        copy_len = *lengthP;
    }
    memcpy(buffer, characteristic_buf[attributeHandle].data, copy_len);
    *lengthP = copy_len;

    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGattServer::read_(
    Gap::Handle_t connectionHandle, GattAttribute::Handle_t attributeHandle,
    uint8_t buffer[], uint16_t *lengthP)
{
    return read(attributeHandle, buffer, lengthP);
}

ble_error_t Esp32AtGattServer::write_(
    GattAttribute::Handle_t attributeHandle, const uint8_t buffer[], uint16_t len, bool localOnly)
{
    if (attributeHandle >= characteristic_count) {
        return BLE_ERROR_PARAM_OUT_OF_RANGE;
    }

    uint16_t copy_len = characteristic_buf[attributeHandle].max_len;

    if (copy_len > len) {
        copy_len = len;
    }
    memcpy(characteristic_buf[attributeHandle].data, buffer, copy_len);

    if (!_esp->ble_set_characteristic(attributeHandle + 1, 1, buffer, len)) {
        return BLE_ERROR_PARAM_OUT_OF_RANGE;
    }
    if (localOnly == false) {
        if (!_esp->ble_notifies_characteristic(attributeHandle + 1, 1, buffer, len)) {
            return BLE_ERROR_PARAM_OUT_OF_RANGE;
        }
    }

    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGattServer::write_(
    Gap::Handle_t connectionHandle, GattAttribute::Handle_t attributeHandle,
    const uint8_t buffer[], uint16_t len, bool localOnly)
{
    return write(attributeHandle, buffer, len, localOnly);
}

void Esp32AtGattServer::write_cb(ESP32::ble_packet_t * ble_packet)
{
    if (ble_packet != NULL) {
        GattWriteCallbackParams write_params;
        GattAttribute::Handle_t attributeHandle = ble_packet->char_index - 1;

        write_params.connHandle = 0;
        write_params.handle     = attributeHandle;
        write_params.writeOp    = GattWriteCallbackParams::OP_WRITE_REQ; // ??
        write_params.offset     = 0;
        if ((characteristic_buf != NULL) && (characteristic_buf[attributeHandle].data != NULL)) {
            characteristic_buf[attributeHandle].cur_len = ble_packet->len;
            memcpy(characteristic_buf[attributeHandle].data, (uint8_t *)ble_packet->data, ble_packet->len);
            write_params.len    = characteristic_buf[attributeHandle].cur_len;
            write_params.data   = characteristic_buf[attributeHandle].data;
        } else {
            write_params.len    = ble_packet->len;
            write_params.data   = (uint8_t *)ble_packet->data;
        }

        handleDataWrittenEvent(&write_params);
    }
}

void Esp32AtGattServer::doEvent(uint32_t id, void * arg)
{
    // do nothing
}

