#ifndef ANTI_REPLAY_FILTER_H
#define ANTI_REPLAY_FILTER_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief RFC 6479 Anti-Replay Sliding Window Filter.
 * 
 * Protects ESP-NOW communications against packet replay attacks.
 * Tracks incoming 32-bit sequence numbers per remote panel across a
 * 64-packet bitmap window. Replays or obsolete packets outside the window
 * are rejected, while valid out-of-order frames within the window are accepted.
 */
class AntiReplayFilter {
public:
    static constexpr size_t MAX_REMOTES = 8;
    static constexpr uint32_t WINDOW_SIZE = 64;

    AntiReplayFilter();

    /**
     * @brief Validate sequence number and advance window if valid.
     * @param remoteId 1-based remote ID (1 .. MAX_REMOTES).
     * @param seq 32-bit sequence counter from packet.
     * @return true if packet is accepted, false if rejected (replay or obsolete).
     */
    bool validateAndAdvance(uint8_t remoteId, uint32_t seq);

    /**
     * @brief Reset sequence state for a single remote (e.g. after authorized re-pairing).
     */
    void resetRemote(uint8_t remoteId);

    /**
     * @brief Reset sequence state for all remotes.
     */
    void resetAll();

    /**
     * @brief Get highest sequence number seen so far for a remote.
     */
    uint32_t getMaxSeq(uint8_t remoteId) const;

    /**
     * @brief Check whether at least one valid packet has been accepted for this remote.
     */
    bool isInitialized(uint8_t remoteId) const;

private:
    struct RemoteState {
        uint32_t maxSeq;
        uint64_t windowBitmap;
        bool initialized;
    };

    RemoteState _remotes[MAX_REMOTES];
};

#endif // ANTI_REPLAY_FILTER_H
