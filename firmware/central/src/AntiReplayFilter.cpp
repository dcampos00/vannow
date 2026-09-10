#include "AntiReplayFilter.h"
#include <string.h>

AntiReplayFilter::AntiReplayFilter() {
    resetAll();
}

void AntiReplayFilter::resetAll() {
    for (size_t i = 0; i < MAX_REMOTES; i++) {
        _remotes[i].maxSeq = 0;
        _remotes[i].windowBitmap = 0;
        _remotes[i].initialized = false;
    }
}

void AntiReplayFilter::resetRemote(uint8_t remoteId) {
    if (remoteId == 0 || remoteId > MAX_REMOTES) return;
    size_t idx = remoteId - 1;
    _remotes[idx].maxSeq = 0;
    _remotes[idx].windowBitmap = 0;
    _remotes[idx].initialized = false;
}

bool AntiReplayFilter::validateAndAdvance(uint8_t remoteId, uint32_t seq) {
    if (remoteId == 0 || remoteId > MAX_REMOTES) {
        return false;
    }
    if (seq == 0) {
        // Sequence number 0 is reserved and invalid
        return false;
    }

    size_t idx = remoteId - 1;
    RemoteState& state = _remotes[idx];

    // First packet received from this remote
    if (!state.initialized) {
        state.initialized = true;
        state.maxSeq = seq;
        state.windowBitmap = 1ULL; // Mark maxSeq bit (bit 0) as seen
        return true;
    }

    // Packet arrives ahead of window (new highest sequence)
    if (seq > state.maxSeq) {
        uint32_t diff = seq - state.maxSeq;
        if (diff < WINDOW_SIZE) {
            state.windowBitmap = (state.windowBitmap << diff) | 1ULL;
        } else {
            // Jumped beyond window; reset bitmap with only the current packet
            state.windowBitmap = 1ULL;
        }
        state.maxSeq = seq;
        return true;
    }

    // Packet arrives behind or at maxSeq
    uint32_t diff = state.maxSeq - seq;
    if (diff >= WINDOW_SIZE) {
        // Packet is too old (outside sliding window)
        return false;
    }

    uint64_t mask = (1ULL << diff);
    if ((state.windowBitmap & mask) != 0) {
        // Already seen this sequence number (replay attack or duplicate frame)
        return false;
    }

    // Valid out-of-order packet within window
    state.windowBitmap |= mask;
    return true;
}

uint32_t AntiReplayFilter::getMaxSeq(uint8_t remoteId) const {
    if (remoteId == 0 || remoteId > MAX_REMOTES) return 0;
    return _remotes[remoteId - 1].maxSeq;
}

bool AntiReplayFilter::isInitialized(uint8_t remoteId) const {
    if (remoteId == 0 || remoteId > MAX_REMOTES) return false;
    return _remotes[remoteId - 1].initialized;
}
