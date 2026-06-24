#include "core/j2534/J2534Error.h"
#include "core/j2534/J2534Types.h"

namespace core::j2534 {

DiagErr mapJ2534Error(long status) {
    switch (status) {
        case STATUS_NOERROR:
            return DiagErr::Ok;
        case ERR_TIMEOUT:
            return DiagErr::Timeout;
        case ERR_INVALID_CHANNEL_ID:
            return DiagErr::InvalidChannel;
        case ERR_INVALID_DEVICE_ID:
        case ERR_DEVICE_NOT_CONNECTED:
            return DiagErr::DeviceNotOpen;
        case ERR_NOT_SUPPORTED:
            return DiagErr::Unsupported;
        default:
            return DiagErr::DriverError;
    }
}

}  // namespace core::j2534
