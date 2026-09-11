# ESP-NOW Protocol & Anti-Replay (as implemented)

Do not invent a second packet format. Production code uses `firmware/lib/protocol/protocol.h`.

## Packet

```cpp
enum class ActionType : uint8_t {
    Click = 0, StartHold = 1, Release = 2, EncoderTurn = 3, DoubleClick = 4
};

struct __attribute__((packed)) SwitchMessage {
    uint8_t  remote_id;
    uint8_t  button_index;
    uint8_t  action;
    int8_t   rotation_steps;
    float    battery_voltage;
    uint32_t seq;
};
```

There is no magic byte, CRC-16, or `channel_id` on the air. Routing is `(remote_id, button_index)` → `RemoteMapping` on the central. Encoder uses `button_index = 7`.

Channel is fixed to **Wi-Fi channel 1**. Encryption uses PMK + per-peer LMK (`encrypt = true`). Keys in-tree are placeholders — provision before a van install. Check `esp_wifi_set_mac` return value.

Remote IDs: 1 entrance, 2 bed (table only), 3 cockpit.

## Anti-replay

API: `AntiReplayFilter::validateAndAdvance(remoteId, seq)`.

- `seq == 0` is invalid.
- First packet after an **empty** in-memory window is accepted (then persisted).
- 64-bit sliding window; duplicates and seq older than 64 are rejected.
- After each accepted packet, persist `{init, maxSeq, bitmap}` under NVS namespace `vannow_replay`.
- Remote stores `seq` in RTC (survives deep sleep) and NVS `vannow_seq` (survives battery removal).

Never reintroduce `if (seq <= last && seq != 1)`.

## Threading

```
OnDataRecv (wifi_task)
  MAC allow-list
  memcpy into PacketQueueItem
  xQueueSend(..., 0)          // no channel mutation, no NVS

loop()
  xQueueReceive
  SystemController::dispatchMessage  // anti-replay + routing + FSMs
  SystemController::update
```

`dispatchMessage` may write NVS (channel state, anti-replay). That is only legal on `loopTask`.
