#include "s3paper/status.h"

namespace s3paper {

const char *StatusCodeName(StatusCode code) {
    switch (code) {
        case StatusCode::Ok: return "Ok";
        case StatusCode::InvalidArgument: return "InvalidArgument";
        case StatusCode::CapacityExceeded: return "CapacityExceeded";
        case StatusCode::Busy: return "Busy";
        case StatusCode::Timeout: return "Timeout";
        case StatusCode::CorruptData: return "CorruptData";
        case StatusCode::OutOfMemory: return "OutOfMemory";
        case StatusCode::Unimplemented: return "Unimplemented";
    }
    return "Unknown";
}

}  // namespace s3paper
