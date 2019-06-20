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

#ifndef _ESP32AT_GAP_H_
#define _ESP32AT_GAP_H_

#include "mbed.h"
#include "ble/blecommon.h"
#include "ble/GapAdvertisingParams.h"
#include "ble/GapAdvertisingData.h"
#include "ble/Gap.h"
#include "ble/GapScanningParams.h"

#include "ESP32.h"

namespace ble {
namespace atcmd {

class Esp32AtGap : public interface::LegacyGap<Esp32AtGap>
{
public:
    static Esp32AtGap &getInstance();

    /**
     * @see Gap::getAddress
     */
    ble_error_t getAddress_(
        BLEProtocol::AddressType_t *typeP,
        BLEProtocol::AddressBytes_t address
    );

    /** @copydoc Gap::setAdvertisingData
     */
    ble_error_t setAdvertisingData_(const GapAdvertisingData &, const GapAdvertisingData &);

    /**
     * @see Gap::getMinAdvertisingInterval
     */
    uint16_t getMinAdvertisingInterval_(void) const;

    /**
     * @see Gap::getMinNonConnectableAdvertisingInterval
     */
    uint16_t getMinNonConnectableAdvertisingInterval_(void) const;

    /**
     * @see Gap::getMaxAdvertisingInterval
     */
    uint16_t getMaxAdvertisingInterval_(void) const;

    /**
     * @see Gap::startAdvertising
     */
    ble_error_t startAdvertising_(const GapAdvertisingParams &);

    /**
     * @see Gap::setDeviceName
     */
    ble_error_t setDeviceName_(const uint8_t *deviceName);

    /**
     * @see Gap::getDeviceName
     */
    ble_error_t getDeviceName_(uint8_t *deviceName, unsigned *lengthP);

    /** @copydoc Gap::setScanParameters
     */
    ble_error_t setScanParameters_(const ScanParameters &params);

    /** @copydoc Gap::startScan
     */
    ble_error_t startScan_(
        scan_duration_t duration = scan_duration_t::forever(),
        duplicates_filter_t filtering = duplicates_filter_t::DISABLE,
        scan_period_t period = scan_period_t(0)
    );

    /**
     * @see Gap::stopScan
     */
    ble_error_t stopScan_();

    /** @copydoc Gap::setAdvertisingParams
     */
    ble_error_t setAdvertisingParameters_(
        advertising_handle_t handle,
        const AdvertisingParameters &params
    );

    /** @copydoc Gap::setAdvertisingPayload
     */
    ble_error_t setAdvertisingPayload_(
        advertising_handle_t handle,
        mbed::Span<const uint8_t> payload
    );

    /** @copydoc Gap::setAdvertisingScanResponse
     */
    ble_error_t setAdvertisingScanResponse_(
        advertising_handle_t handle,
        mbed::Span<const uint8_t> response
    );

    /** @copydoc Gap::startAdvertising
     */
    ble_error_t startAdvertising_(
        advertising_handle_t handle,
        adv_duration_t maxDuration = adv_duration_t::forever(),
        uint8_t maxEvents = 0
    );

    /**
     * @see Gap::connect
     */
    ble_error_t connect_(
        peer_address_type_t peerAddressType,
        const ble::address_t &peerAddress,
        const ConnectionParameters &connectionParams
    );

    /* event process */
    void doEvent(uint32_t id, void * arg);

protected:
    // import from Gap
    friend interface::Gap<Esp32AtGap>;

    using interface::Gap<Esp32AtGap>::startAdvertising_;
    using interface::Gap<Esp32AtGap>::stopAdvertising_;
    using interface::Gap<Esp32AtGap>::connect_;
    using interface::Gap<Esp32AtGap>::disconnect_;

    // import from LegacyGap
    friend interface::LegacyGap<Esp32AtGap>;

    using interface::LegacyGap<Esp32AtGap>::startAdvertising_;
    using interface::LegacyGap<Esp32AtGap>::stopAdvertising_;
    using interface::LegacyGap<Esp32AtGap>::connect_;
    using interface::LegacyGap<Esp32AtGap>::disconnect_;

private:
    ESP32 *_esp;
    BLEProtocol::AddressType_t _address_type;
    address_t _address;
    bool _scan;
    Timeout scanTimeout;
    Timeout advertisingTimeout;
    advertising_handle_t advertising_handle;
    bool _connect;
    uint8_t randam_addr[6];
    ESP32::advertising_param_t advertising_param;

    Esp32AtGap();
    Esp32AtGap(Esp32AtGap const &);
    void operator=(Esp32AtGap const &);

    void ble_sigio_cb(void);
    void ble_conn_cb(int conn_index, uint8_t * remote_addr);
    void ble_disconn_cb(int conn_index);
    void ble_scan_cb(ESP32::ble_scan_t * ble_scan);

    void scanTimeoutCallback();
    void advertisingTimeoutCallback();
};

} // namespace atcmd
} // namespace ble

#endif /* _ESP32AT_GAP_H_ */
