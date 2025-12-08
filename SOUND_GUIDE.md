# PC Speaker Audio Guide

AltoniumOS includes a PC speaker driver that provides simple musical capabilities through the system speaker.

## Overview

The PC speaker driver programs the Programmable Interval Timer (PIT) Channel 2 to generate square waves at specific frequencies, producing audible tones. The speaker is gated on/off via port 0x61 to control sound output.

## Note Naming Convention

Notes follow standard musical notation with octave numbers:

- **Format**: `[Letter][Accidental][Octave]`
- **Letter**: A through G (case-insensitive when scripting)
- **Accidental**: `#` for sharp (e.g., C#), no character for natural
- **Octave**: Number from 3 to 7

### Supported Octave Range

The PC speaker driver supports notes from **C3** (131 Hz) to **B7** (3951 Hz), covering 5 octaves:

- **Octave 3**: C3 (131 Hz) through B3 (247 Hz) - Low notes
- **Octave 4**: C4 (262 Hz) through B4 (494 Hz) - Middle C and surrounding notes
- **Octave 5**: C5 (523 Hz) through B5 (988 Hz) - Higher range
- **Octave 6**: C6 (1047 Hz) through B6 (1976 Hz) - High range
- **Octave 7**: C7 (2093 Hz) through B7 (3951 Hz) - Very high

### Note to Frequency Mapping

Here are the available notes and their frequencies:

**Octave 3:**
- C3: 131 Hz, C#3: 139 Hz, D3: 147 Hz, D#3: 156 Hz
- E3: 165 Hz, F3: 175 Hz, F#3: 185 Hz, G3: 196 Hz
- G#3: 208 Hz, A3: 220 Hz, A#3: 233 Hz, B3: 247 Hz

**Octave 4:**
- C4: 262 Hz, C#4: 277 Hz, D4: 294 Hz, D#4: 311 Hz
- E4: 330 Hz, F4: 349 Hz, F#4: 370 Hz, G4: 392 Hz
- G#4: 415 Hz, A4: 440 Hz, A#4: 466 Hz, B4: 494 Hz

**Octave 5:**
- C5: 523 Hz, C#5: 554 Hz, D5: 587 Hz, D#5: 622 Hz
- E5: 659 Hz, F5: 698 Hz, F#5: 740 Hz, G5: 784 Hz
- G#5: 831 Hz, A5: 880 Hz, A#5: 932 Hz, B5: 988 Hz

**Octave 6:**
- C6: 1047 Hz, C#6: 1109 Hz, D6: 1175 Hz, D#6: 1245 Hz
- E6: 1319 Hz, F6: 1397 Hz, F#6: 1480 Hz, G6: 1568 Hz
- G#6: 1661 Hz, A6: 1760 Hz, A#6: 1865 Hz, B6: 1976 Hz

**Octave 7:**
- C7: 2093 Hz, C#7: 2217 Hz, D7: 2349 Hz, D#7: 2489 Hz
- E7: 2637 Hz, F7: 2794 Hz, F#7: 2960 Hz, G7: 3136 Hz
- G#7: 3322 Hz, A7: 3520 Hz, A#7: 3729 Hz, B7: 3951 Hz

## Commands

### Single Tone

Play a single frequency for a specified duration:

```bash
beep <frequency> <duration_ms>
```

**Examples:**
```bash
beep 440 200    # Play A4 (440 Hz) for 200ms
beep 880 500    # Play A5 (880 Hz) for 500ms
beep 261 1000   # Play C4 (261 Hz) for 1 second
```

### Melody Mode

Play sequences of notes with timing:

```bash
beep melody NOTE:MS,NOTE:MS,...
```

Each note is specified as `NOTE:DURATION` where:
- **NOTE** is a note name (e.g., C4, D#5, A6)
- **DURATION** is in milliseconds
- Multiple notes are separated by commas

**Examples:**
```bash
# Play a simple C major arpeggio
beep melody C4:250,E4:250,G4:500

# Play a scale
beep melody C4:200,D4:200,E4:200,F4:200,G4:200,A4:200,B4:200,C5:400

# Play a melody with different note lengths
beep melody E4:250,E4:250,E4:500,F4:250,G4:750
```

### Piano Mode

Interactive piano using the keyboard:

```bash
beep piano
```

**Controls:**
- Letter keys map to white and black piano keys
- Press ESC to exit piano mode
- Keys are arranged in a piano-like layout

**Keyboard to Notes Layout:**
```
White keys:  A S D F G H J K L ; P
Black keys:  W E T Y U O

Mapping:
a = C4    w = C#4   s = D4    e = D#4   d = E4
f = F4    t = F#4   g = G4    y = G#4   h = A4
u = A#4   j = B4    k = C5    o = C#5   l = D5
p = E5
```

## Scripting Simple Songs

You can create simple musical sequences by chaining notes with appropriate timings. Here are some examples:

### Example 1: "Happy Birthday" (First Line)

Notes: C4 C4 D4 C4 F4 E4

```bash
beep melody C4:250,C4:250,D4:500,C4:500,F4:250,E4:750
```

### Example 2: "Twinkle Twinkle Little Star"

Notes: C4 C4 G4 G4 A4 A4 G4 F4 F4 E4 E4 D4 D4 C4

```bash
beep melody C4:250,C4:250,G4:250,G4:250,A4:250,A4:250,G4:500,F4:250,F4:250,E4:250,E4:250,D4:250,D4:250,C4:500
```

### Example 3: Major Scale

C4 D4 E4 F4 G4 A4 B4 C5

```bash
beep melody C4:200,D4:200,E4:200,F4:200,G4:200,A4:200,B4:200,C5:400
```

### Example 4: "Mary Had a Little Lamb"

Notes: E4 D4 C4 D4 E4 E4 E4

```bash
beep melody E4:300,D4:300,C4:300,D4:300,E4:300,E4:300,E4:600
```

## Notes on Timing

- Timings are approximate due to busy-wait implementation
- The `sleep_ms()` function uses a simple CPU loop
- For more complex music, create sequences of shorter notes
- Rests (silence) can be created with `beep 0 <duration>`

## Technical Details

### Hardware Interface

The PC speaker driver uses these hardware ports:

- **PIT Channel 2 Data**: 0x42
- **PIT Command Register**: 0x43
- **Speaker Control**: 0x61 (bits 0-1 enable/disable speaker)

### PIT Programming

The PIT is programmed with:
- Mode 3 (square wave generator)
- Divisor calculated as: `1193182 / frequency`
- 16-bit divisor loaded low-byte/high-byte

The maximum divisor (0 = 65536) gives the minimum frequency of approximately 18.2 Hz.

## Limitations

- Only square wave tones (no waveform synthesis)
- No volume control (speaker is either on or off)
- No support for multiple simultaneous notes
- Timing accuracy limited by busy-wait implementation
- Some very low frequencies may be inaudible

## Troubleshooting

**No sound heard:**
- Check that QEMU audio is enabled (-soundhw pcspk)
- Some systems may not have a PC speaker
- Frequency may be too low or high to hear

**Sound is distorted:**
- Some frequencies may not generate clean square waves
- Try different frequencies within the audible range (200-2000 Hz)