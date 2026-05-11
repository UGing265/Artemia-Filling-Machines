---
title: "Artemia Handling Sequence with Sensor-Triggered Pump"
description: "Photoelectric sensor detects tube → wait 3s → pump 5ml in ~5s → wait 5s → rotate. Processing time adjustable 2-20s via pot."
status: completed
priority: P1
effort: 2h
branch: main
tags: [feature, motor-control, arduino, pump-control]
created: 2026-05-03
---

# Artemia Handling Sequence with Sensor-Triggered Pump

## Overview

Replace the current hardcoded rotation+pump cycle with a sensor-driven sequence:
- Photoelectric sensor detects tube → wait 3s → pump fills 5ml (~5s) → wait 5s → rotate
- If no tube detected after rotation, wait 10s then continue searching

## LCD Display

```
Line 0: "MAY CHIET ARTERMIA"
Line 1: "Speed:XXX"
Line 2: "Tubes:X |....|"
Line 3: "Time:Xs"
```
- Speed: pump PWM (0-255)
- Tubes: filled tube count
- Time: adjustable processing time (2s-20s via pot)
- Bar: `|` = tube detected, `-` = no tube