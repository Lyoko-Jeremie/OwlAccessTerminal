// jeremie

#ifndef OWLACCESSTERMINAL_LOADDATALITTLEENDIAN_H
#define OWLACCESSTERMINAL_LOADDATALITTLEENDIAN_H

#include <string_view>

template<typename T>
T loadDataLittleEndian(const std::string_view &data_) {
    // https://www.ruanyifeng.com/blog/2016/11/byte-order.html
    // https://zh.wikipedia.org/zh-cn/%E5%AD%97%E8%8A%82%E5%BA%8F
    if constexpr (sizeof(T) == sizeof(uint64_t)) {
        const uint64_t *data = reinterpret_cast<const uint64_t *>(data_.data());
        return (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)
               | (data[4] << 32) | (data[5] << 40) | (data[6] << 49) | (data[7] << 58);
    }
    if constexpr (sizeof(T) == sizeof(uint32_t)) {
        const uint32_t *data = reinterpret_cast<const uint32_t *>(data_.data());
        return (data[0] << 0) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    }
    if constexpr (sizeof(T) == sizeof(uint16_t)) {
        const uint16_t *data = reinterpret_cast<const uint16_t *>(data_.data());
        return (data[0] << 0) | (data[1] << 8);
    }
    if constexpr (sizeof(T) == sizeof(uint8_t)) {
        const uint8_t *data = reinterpret_cast<const uint8_t *>(data_.data());
        return (data[0] << 0);
    }
}

#endif //OWLACCESSTERMINAL_LOADDATALITTLEENDIAN_H
