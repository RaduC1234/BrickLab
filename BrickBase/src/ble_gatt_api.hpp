#pragma once

#include <vector>
#include <array>
#include <string>
#include <functional>
#include <cstdint>
#include <algorithm>  // <- Required for std::any_of

#include <esp_gatts_api.h>
#include <esp_log_buffer.h>

// Macro wrapper
#define TO_UUID(str) parseUUID(str)

struct GattDescriptor {
    uint16_t uuid;
    std::vector<uint8_t> value;
    uint16_t permissions;
};

struct GattCharacteristic {
    std::array<uint8_t, 16> uuid;
    uint8_t properties;
    uint16_t permissions;
    std::vector<uint8_t> value;
    std::vector<GattDescriptor> descriptors;
    std::function<std::vector<uint8_t>()> onRead;
    std::function<void(const std::vector<uint8_t>&)> onWrite;
};

struct GattService {
    std::array<uint8_t, 16> uuid;
    std::vector<GattCharacteristic> characteristics;
};

struct GattDescriptorBuilder {
    GattDescriptor d{};
    GattDescriptorBuilder& uuid(uint16_t uuid) { d.uuid = uuid; return *this; }
    GattDescriptorBuilder& permissions(uint16_t perm) { d.permissions = perm; return *this; }
    GattDescriptorBuilder& value(const std::vector<uint8_t>& val) { d.value = val; return *this; }
    GattDescriptor build() const { return d; }
};

struct GattCharacteristicBuilder {
    GattCharacteristic ch{};
    GattCharacteristicBuilder& uuid(const std::array<uint8_t, 16>& uuid) { ch.uuid = uuid; return *this; }
    GattCharacteristicBuilder& properties(uint8_t props) { ch.properties = props; return *this; }
    GattCharacteristicBuilder& permissions(uint16_t perm) { ch.permissions = perm; return *this; }
    GattCharacteristicBuilder& value(const std::vector<uint8_t>& val) { ch.value = val; return *this; }
    GattCharacteristicBuilder& onRead(std::function<std::vector<uint8_t>()> cb) { ch.onRead = std::move(cb); return *this; }
    GattCharacteristicBuilder& onWrite(std::function<void(const std::vector<uint8_t>&)> cb) { ch.onWrite = std::move(cb); return *this; }
    GattCharacteristicBuilder& addDescriptor(const GattDescriptor& desc) { ch.descriptors.push_back(desc); return *this; }

    GattCharacteristic build() {
        ESP_LOG_BUFFER_HEX("UUID_build", ch.uuid.data(), 16);
        bool needs_cccd = ch.properties & (ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_INDICATE);
        bool has_cccd = std::any_of(ch.descriptors.begin(), ch.descriptors.end(),
            [](const GattDescriptor& d) { return d.uuid == 0x2902; }); // what is this magic number?

        if (needs_cccd && !has_cccd) {
            GattDescriptor cccd = GattDescriptorBuilder()
                .uuid(0x2902)
                .permissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE)
                .value({0x00, 0x00})
                .build();
            ch.descriptors.push_back(cccd);
        }

        return ch;
    }
};

struct GattServiceBuilder {
    GattService svc{};
    GattServiceBuilder& uuid(const std::array<uint8_t, 16>& uuid) { svc.uuid = uuid; return *this; }
    GattServiceBuilder& addCharacteristic(const GattCharacteristic& ch) { svc.characteristics.push_back(ch); return *this; }
    GattService build() const { return svc; }
};
