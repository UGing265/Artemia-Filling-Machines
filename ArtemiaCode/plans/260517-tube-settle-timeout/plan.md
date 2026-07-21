---
title: Extend PUMP_WAIT Timeout to 9s
description: Extend the tube stability timeout during PUMP_WAIT from 6s to 9s, giving the tube more time to settle during the 1s delay period before pump activates.
status: completed
priority: medium
effort: low
branch: main
tags: [timeout, pump, bug-fix]
created: 2026-05-17
updated: 2026-05-17
---

# Plan: Extend PUMP_WAIT Timeout to 9s

## Summary

Changed timeout from 6s to 9s (waitElapsed >= 9000).

## Files to Modify

| File | Change |
|------|--------|
| `ArtemiaCode.ino` | Line 267: change `waitElapsed >= 6000` to `waitElapsed >= 9000` |

## Current Behavior (Bug)

Line 267:
```cpp
} else if (waitElapsed >= 6000) {
```

- PUMP_WAIT has 1s delay (`PUMP_WAIT_DELAY = 1000`)
- 6s timeout gives only 5 extra seconds after the 1s wait = tube must stabilize within 5s after initial detection
- If user drops tube and it takes >5s to settle, machine aborts and rotates back

## Fix

Change timeout from 6s to 9s:
```cpp
} else if (waitElapsed >= 9000) {
```

- 9s timeout = 8 extra seconds after the 1s wait = tube has 8s to settle
- More forgiving for slower tube placement

## Implementation Steps

1. Edit `ArtemiaCode.ino` line 267: `waitElapsed >= 6000` → `waitElapsed >= 9000`
2. Update comment to reflect new timeout

## Success Criteria

1. Machine waits 1s then starts pump if tube stays in position
2. Machine aborts to ROTATING only after tube has 8s (not 5s) to stabilize
3. No other behavior changes