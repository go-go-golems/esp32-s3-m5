#include "s3paper/region.h"

#include <cstring>

namespace s3paper {

void RegionTable::Clear() {
    count_ = 0;
    std::memset(invalid_, 0, sizeof(invalid_));
}

Status RegionTable::Add(const RegionSpec &spec) {
    for (uint32_t i = 0; i < count_; ++i) {
        if (specs_[i].id == spec.id) {
            specs_[i] = spec;
            return OkStatus();
        }
    }
    if (count_ >= kCapacity) {
        return ErrStatus(StatusCode::CapacityExceeded);
    }
    specs_[count_] = spec;
    invalid_[count_] = false;
    count_++;
    return OkStatus();
}

const RegionSpec *RegionTable::At(uint32_t i) const {
    return i < count_ ? &specs_[i] : nullptr;
}

const RegionSpec *RegionTable::Find(uint32_t region_id) const {
    for (uint32_t i = 0; i < count_; ++i) {
        if (specs_[i].id == region_id) {
            return &specs_[i];
        }
    }
    return nullptr;
}

void RegionTable::Invalidate(DependencyId changed) {
    if (changed == 0) {
        return;
    }
    for (uint32_t i = 0; i < count_; ++i) {
        if (specs_[i].dependency == changed) {
            invalid_[i] = true;
        }
    }
}

Status RegionTable::InvalidateRegion(uint32_t region_id) {
    for (uint32_t i = 0; i < count_; ++i) {
        if (specs_[i].id == region_id) {
            invalid_[i] = true;
            return OkStatus();
        }
    }
    return ErrStatus(StatusCode::InvalidArgument);
}

uint32_t RegionTable::TakeInvalid(uint32_t *region_ids, uint32_t cap) {
    uint32_t written = 0;
    for (uint32_t i = 0; i < count_; ++i) {
        if (!invalid_[i]) {
            continue;
        }
        if (written < cap && region_ids != nullptr) {
            region_ids[written] = specs_[i].id;
        }
        invalid_[i] = false;
        written++;
    }
    return written > cap ? cap : written;
}

}  // namespace s3paper
