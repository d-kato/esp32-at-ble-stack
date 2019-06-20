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

#include "Esp32AtGap.h"
#include "mbed.h"

#include "Esp32AtBLE.h"

#ifndef RANDOM_SEED_AD_PIN
#define RANDOM_SEED_AD_PIN  A0
#endif

#if DEVICE_TRNG
#include "hal/trng_api.h"
#endif

namespace ble {
namespace atcmd {

Esp32AtGap &Esp32AtGap::getInstance() {
    static Esp32AtGap m_instance;
    return m_instance;
}

Esp32AtGap::Esp32AtGap() : _scan(false), _connect(false) {
    _esp = ESP32::getESP32Inst();
    _esp->ble_attach_sigio(callback(this, &Esp32AtGap::ble_sigio_cb));
    _esp->ble_attach_conn(callback(this, &Esp32AtGap::ble_conn_cb));
    _esp->ble_attach_disconn(callback(this, &Esp32AtGap::ble_disconn_cb));
    _esp->ble_attach_scan(callback(this, &Esp32AtGap::ble_scan_cb));
    advertising_param.own_addr_type = BLE_ADDR_TYPE_RANDOM;

#if DEVICE_TRNG
    size_t olen;

    trng_t trng_obj;
    trng_init(&trng_obj);
    int ret = trng_get_bytes(&trng_obj, &randam_addr[0], 6, &olen);
    trng_free(&trng_obj);
#else
    AnalogIn ad(RANDOM_SEED_AD_PIN);
    uint32_t rand_data;

    srand(ad.read_u16());
    rand_data = rand();
    randam_addr[0] = (uint8_t)(rand_data);
    randam_addr[1] = (uint8_t)(rand_data >> 8);
    randam_addr[2] = (uint8_t)(rand_data >> 16);
    srand(ad.read_u16());
    rand_data = rand();
    randam_addr[3] = (uint8_t)(rand_data);
    randam_addr[4] = (uint8_t)(rand_data >> 8);
    randam_addr[5] = (uint8_t)(rand_data >> 16);
#endif
    randam_addr[0] |= 0xC0; // The two most significant bits of the address shall be equal to 1.
}

ble_error_t Esp32AtGap::setAdvertisingData_(const GapAdvertisingData &advData, const GapAdvertisingData &scanResponse)
{
    return BLE_ERROR_NOT_IMPLEMENTED;
}

uint16_t Esp32AtGap::getMinAdvertisingInterval_() const
{
    return GapAdvertisingParams::GAP_ADV_PARAMS_INTERVAL_MIN;
}

uint16_t Esp32AtGap::getMinNonConnectableAdvertisingInterval_() const
{
    return GapAdvertisingParams::GAP_ADV_PARAMS_INTERVAL_MIN_NONCON;
}

uint16_t Esp32AtGap::getMaxAdvertisingInterval_() const
{
    return GapAdvertisingParams::GAP_ADV_PARAMS_INTERVAL_MAX;
}

ble_error_t Esp32AtGap::startAdvertising_(const GapAdvertisingParams &params)
{
    return BLE_ERROR_NOT_IMPLEMENTED;
}

ble_error_t Esp32AtGap::setScanParameters_(const ScanParameters &params)
{
    useVersionTwoAPI();

    int scan_type = 0;

    if (params.get1mPhyConfiguration().isActiveScanningSet()) {
        scan_type = 1;
    }

    if (!_esp->ble_set_scan_param(scan_type,
            (int)params.getOwnAddressType().value(),
            (int)params.getFilter().value(),
            (int)params.get1mPhyConfiguration().getInterval().value(),
            (int)params.get1mPhyConfiguration().getWindow().value())
    ) {
        return BLE_ERROR_INVALID_STATE;
    }

    return BLE_ERROR_NONE;
};

ble_error_t Esp32AtGap::startScan_(
    scan_duration_t duration,
    duplicates_filter_t filtering,
    scan_period_t period
)
{
    useVersionTwoAPI();

    if (!_esp->ble_start_scan()) {
        return BLE_ERROR_INVALID_STATE;
    }
    _scan = true;

    if (duration.valueInMs() > 0) {
        timestamp_t timestamp = duration.valueInMs() * 1000;
        scanTimeout.attach_us(callback(this, &Esp32AtGap::scanTimeoutCallback), timestamp);
    }

    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGap::stopScan_()
{
    _scan = false;
    scanTimeout.detach();
    if (!_esp->ble_stop_scan()) {
        return BLE_ERROR_INVALID_STATE;
    }
    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGap::getAddress_(
    BLEProtocol::AddressType_t *typeP,
    BLEProtocol::AddressBytes_t address
)
{
    if (advertising_param.own_addr_type == BLE_ADDR_TYPE_RANDOM) {
        if (typeP != NULL) {
            *typeP = BLEProtocol::AddressType::RANDOM_STATIC;
        }
        address[0] = randam_addr[5];
        address[1] = randam_addr[4];
        address[2] = randam_addr[3];
        address[3] = randam_addr[2];
        address[4] = randam_addr[1];
        address[5] = randam_addr[0];
    } else {
        uint8_t tmp_buf[6];

        if (typeP != NULL) {
            *typeP = BLEProtocol::AddressType::PUBLIC;
        }
        if (!_esp->ble_get_addr(tmp_buf)) {
            return BLE_ERROR_INVALID_STATE;
        }
        address[0] = tmp_buf[5];
        address[1] = tmp_buf[4];
        address[2] = tmp_buf[3];
        address[3] = tmp_buf[2];
        address[4] = tmp_buf[1];
        address[5] = tmp_buf[0];
    }

    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGap::setDeviceName_(const uint8_t *deviceName)
{
    if (!_esp->ble_set_device_name((char *)deviceName)) {
        return BLE_ERROR_PARAM_OUT_OF_RANGE;
    }
    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGap::getDeviceName_(uint8_t *deviceName, unsigned *lengthP)
{
    if ((deviceName == NULL) || (lengthP == NULL)) {
        return BLE_ERROR_INVALID_PARAM;
    }

    char * wk_name = new char[128];

    if (!_esp->ble_get_device_name(wk_name)) {
        delete [] wk_name;
        return BLE_ERROR_PARAM_OUT_OF_RANGE;
    }

    unsigned len = strlen(wk_name);

    if (len > *lengthP) {
        delete [] wk_name;
        return BLE_ERROR_PARAM_OUT_OF_RANGE;
    }
    *lengthP = len;
    memcpy(deviceName, wk_name, len);
    delete [] wk_name;

    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGap::setAdvertisingParameters_(
    advertising_handle_t handle,
    const AdvertisingParameters &params
)
{
    advertising_param.adv_int_min = params.getMinPrimaryInterval().value();
    if ((advertising_param.adv_int_min < getMinAdvertisingInterval_())
     || (advertising_param.adv_int_min > getMaxAdvertisingInterval_())) {
        return BLE_ERROR_INVALID_PARAM;
    }

    advertising_param.adv_int_max = params.getMaxPrimaryInterval().value();
    if ((advertising_param.adv_int_max < getMinAdvertisingInterval_())
     || (advertising_param.adv_int_max > getMaxAdvertisingInterval_())) {
        return BLE_ERROR_INVALID_PARAM;
    }

    if (advertising_param.adv_int_min > advertising_param.adv_int_max) {
        return BLE_ERROR_INVALID_PARAM;
    }

    if (params.getType() == advertising_type_t::CONNECTABLE_UNDIRECTED) {
        advertising_param.adv_type = ADV_TYPE_IND;
    } else if (params.getType() == advertising_type_t::SCANNABLE_UNDIRECTED) {
        advertising_param.adv_type = ADV_TYPE_SCAN_IND;
    } else if (params.getType() == advertising_type_t::NON_CONNECTABLE_UNDIRECTED) {
        advertising_param.adv_type = ADV_TYPE_NONCONN_IND;
    } else {
        return BLE_ERROR_INVALID_PARAM;
    }

    if (params.getOwnAddressType() == own_address_type_t::PUBLIC) {
        advertising_param.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
        _esp->ble_set_addr(0);
    } else if (params.getOwnAddressType() == own_address_type_t::RANDOM) {
        advertising_param.own_addr_type = BLE_ADDR_TYPE_RANDOM;
        _esp->ble_set_addr(1, randam_addr);
    } else {
        return BLE_ERROR_INVALID_PARAM;
    }

    advertising_param.channel_map = 0;
    if (params.getChannel37()) {
        advertising_param.channel_map = ADV_CHNL_37;
    }
    if (params.getChannel38()) {
        advertising_param.channel_map = ADV_CHNL_38;
    }
    if (params.getChannel39()) {
        advertising_param.channel_map = ADV_CHNL_39;
    }

    if (params.getFilter() == advertising_filter_policy_t::NO_FILTER) {
        advertising_param.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
    } else if (params.getFilter() == advertising_filter_policy_t::FILTER_SCAN_REQUESTS) {
        advertising_param.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_WLST_CON_ANY;
    } else if (params.getFilter() == advertising_filter_policy_t::FILTER_CONNECTION_REQUEST) {
        advertising_param.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST;
    } else if (params.getFilter() == advertising_filter_policy_t::FILTER_SCAN_AND_CONNECTION_REQUESTS) {
        advertising_param.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_WLST_CON_WLST;
    } else {
        return BLE_ERROR_INVALID_PARAM;
    }

    if (params.getPeerAddressType() == target_peer_address_type_t::PUBLIC) {
        advertising_param.peer_addr_type = BLE_ADDR_TYPE_PUBLIC;
    } else if (params.getPeerAddressType() == target_peer_address_type_t::RANDOM) {
        advertising_param.peer_addr_type = BLE_ADDR_TYPE_RANDOM;
    } else {
        return BLE_ERROR_INVALID_PARAM;
    }

    advertising_param.peer_addr[5] = params.getPeerAddress()[0];
    advertising_param.peer_addr[4] = params.getPeerAddress()[1];
    advertising_param.peer_addr[3] = params.getPeerAddress()[2];
    advertising_param.peer_addr[2] = params.getPeerAddress()[3];
    advertising_param.peer_addr[1] = params.getPeerAddress()[4];
    advertising_param.peer_addr[0] = params.getPeerAddress()[5];

    if (!_esp->ble_set_advertising_param(&advertising_param)) {
        return BLE_ERROR_INVALID_STATE;
    }

    if (!_esp->ble_start_services()) {
        return BLE_ERROR_INVALID_STATE;
    }

    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGap::setAdvertisingPayload_(
    advertising_handle_t handle,
    mbed::Span<const uint8_t> payload
)
{
    if (!_esp->ble_set_advertising_data(payload.data(), payload.size())) {
        return BLE_ERROR_INVALID_STATE;
    }

    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGap::setAdvertisingScanResponse_(
    advertising_handle_t handle,
    mbed::Span<const uint8_t> response
)
{
    if (!_esp->ble_set_scan_response(response.data(), response.size())) {
        return BLE_ERROR_INVALID_STATE;
    }

    return BLE_ERROR_NONE;
}

ble_error_t Esp32AtGap::startAdvertising_(
    advertising_handle_t handle,
    adv_duration_t maxDuration,
    uint8_t maxEvents
)
{
    advertising_handle = handle;
    _esp->ble_start_advertising();

    if (maxDuration.valueInMs() > 0) {
        timestamp_t timestamp = maxDuration.valueInMs() * 1000;
        advertisingTimeout.attach_us(callback(this, &Esp32AtGap::advertisingTimeoutCallback), timestamp);
    }

    return BLE_ERROR_NONE;
}

void Esp32AtGap::ble_sigio_cb(void)
{
    Esp32AtBLE::deviceInstance().signalEventsToProcess(::BLE::DEFAULT_INSTANCE);
}

void Esp32AtGap::ble_conn_cb(int conn_index, uint8_t * remote_addr)
{
    int role;
    ble::address_t peerAddress;

    _connect = true;

    _esp->ble_get_role(&role);
    peerAddress[5] = remote_addr[0];
    peerAddress[4] = remote_addr[1];
    peerAddress[3] = remote_addr[2];
    peerAddress[2] = remote_addr[3];
    peerAddress[1] = remote_addr[4];
    peerAddress[0] = remote_addr[5];

    // ConnectionCompleteEvent
    connection_role_t::type connection_role;

    if (role == INIT_CLIENT_ROLE) {
        connection_role = connection_role_t::CENTRAL;
    } else {
        connection_role = connection_role_t::PERIPHERAL;
    }

    if (_eventHandler) {
        _eventHandler->onConnectionComplete(
            ConnectionCompleteEvent(
                BLE_ERROR_NONE,
                (connection_handle_t)conn_index,
                connection_role,
                peer_address_type_t::ANONYMOUS,
                peerAddress,
                ble::address_t(),
                ble::address_t(),
                conn_interval_t::max(),
                /* dummy slave latency */ 0,
                supervision_timeout_t::max(),
                /* master clock accuracy */ 0
            )
        );
    }

    // legacy process event
    Role_t legacy_role;
    LegacyGap::ConnectionParams_t params = {10, 10, 10, 10};  // dummy
    LegacyGap::AddressType_t ownAddrType;
    LegacyGap::Address_t ownAddr;
    getAddress(&ownAddrType, ownAddr);

    if (role == INIT_CLIENT_ROLE) {
        legacy_role = LegacyGap::CENTRAL;
    } else {
        legacy_role = LegacyGap::PERIPHERAL;
    }

    processConnectionEvent(
        conn_index,
        legacy_role,
        PeerAddressType_t::ANONYMOUS,
        peerAddress.data(),
        ownAddrType,
        ownAddr,
        &params
    );
}

void Esp32AtGap::ble_disconn_cb(int conn_index)
{
    _connect = false;
    if (_eventHandler) {
        _eventHandler->onDisconnectionComplete(
            DisconnectionCompleteEvent(
                0,
                (disconnection_reason_t::type)REMOTE_USER_TERMINATED_CONNECTION
            )
        );
    }

    // legacy process event
    processDisconnectionEvent(
        0,
        (LegacyGap::DisconnectionReason_t)REMOTE_USER_TERMINATED_CONNECTION
    );
}

void Esp32AtGap::ble_scan_cb(ESP32::ble_scan_t * ble_scan)
{
    if (!_scan) {
        return;
    }

    if (_eventHandler) {
        uint8_t tmp_buf[6];

        tmp_buf[5] = ble_scan->addr[0];
        tmp_buf[4] = ble_scan->addr[1];
        tmp_buf[3] = ble_scan->addr[2];
        tmp_buf[2] = ble_scan->addr[3];
        tmp_buf[1] = ble_scan->addr[4];
        tmp_buf[0] = ble_scan->addr[5];

        peer_address_type_t peer_address_type =
            static_cast<peer_address_type_t::type>(ble_scan->addr_type);

        _eventHandler->onAdvertisingReport(
            AdvertisingReportEvent(
                advertising_event_t(0x1B),
                peer_address_type,
                tmp_buf,
                /* primary */ phy_t::LE_1M,
                /* secondary */ phy_t::NONE,
                /* SID - NO ADI FIELD IN THE PDU */ 0xFF,
                /* tx power information not available */ 127,
                ble_scan->rssi,
                /* NO PERIODIC ADVERTISING */ 0,
                peer_address_type_t::ANONYMOUS,
                ble::address_t (),
                mbed::Span<const uint8_t>(ble_scan->adv_data, ble_scan->adv_data_len)
            )
        );
    }
}

ble_error_t Esp32AtGap::connect_(
    peer_address_type_t peerAddressType,
    const ble::address_t &peerAddress,
    const ConnectionParameters &connectionParams
)
{
    uint8_t tmp_buf[6];

    tmp_buf[5] = peerAddress[0];
    tmp_buf[4] = peerAddress[1];
    tmp_buf[3] = peerAddress[2];
    tmp_buf[2] = peerAddress[3];
    tmp_buf[1] = peerAddress[4];
    tmp_buf[0] = peerAddress[5];

    if (!_esp->ble_connect(0, tmp_buf)) {
        return BLE_ERROR_INVALID_STATE;
    }
    return BLE_ERROR_NONE;
}

void Esp32AtGap::scanTimeoutCallback()
{
    if (_scan) {
        stopScan();
        if (_eventHandler) {
            _eventHandler->onScanTimeout(ScanTimeoutEvent());
        }
    }
}

void Esp32AtGap::advertisingTimeoutCallback()
{
    _esp->ble_stop_advertising();
    if (_eventHandler) {
        _eventHandler->onAdvertisingEnd(AdvertisingEndEvent(advertising_handle, 0, 0, _connect));
    }
}

void Esp32AtGap::doEvent(uint32_t id, void * arg)
{
    // do nothing
}

} // namespace atcmd
} // namespace ble

