# PROFET PWM & DimmableChannel

BTS5008-1EKB t_ON / t_OFF are up to **250 µs**. A 5 kHz period (200 µs) never saturates the FET.

Use **200 Hz** (100–400 Hz allowed). `DimmableChannel` default: `frequency = 200`, `resolution = 8` (`ledcAttach` / `ledcWrite` on the pin).

## Ramp FSM (keep-alives)

Remote `ButtonHandler` sends `StartHold` at 400 ms, then every 150 ms.

```
StartHold:
  if (!_isRamping) { _isRamping = true; _rampDirection = -_rampDirection; }
  _lastHoldMsgTime = now;
Release or 300 ms silence:
  stop ramp, persist brightness
```

Do not flip direction on keep-alives. Clamp ramp at 5%–100% (do not turn the lamp fully off while holding). Encoder steps are ±5% per detent on the **focused** zone.

## NVS

Persist brightness on `Release` or after encoder settle (1 s), not on every ramp tick. Pump chirp `digitalWrite` pulses must not each trigger a flash write.
