#include "core/diag/NrcDecoder.h"

namespace core::diag {

std::string decodeNrc(uint8_t nrc) {
    switch (nrc) {
        case 0x10: return "GeneralReject";
        case 0x11: return "ServiceNotSupported";
        case 0x13: return "IncorrectMessageLengthOrInvalidFormat";
        case 0x22: return "ConditionsNotCorrect";
        case 0x31: return "RequestOutOfRange";
        case 0x33: return "SecurityAccessDenied";
        case 0x35: return "InvalidKey";
        case 0x36: return "ExceedNumberOfAttempts";
        case 0x37: return "RequiredTimeDelayNotExpired";
        case 0x78: return "ResponsePending";
        default: return "UnknownNRC";
    }
}

}  // namespace core::diag
