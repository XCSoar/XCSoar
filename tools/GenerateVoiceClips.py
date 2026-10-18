#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project

"""Generate the spoken alert clips in Data/sound/.

The clips are spoken by the Piper voice "cori" (en_GB-cori-high,
trained on public-domain LibriVox recordings by Bryce Beattie).

Usage:
  pip install piper-tts numpy
  # en_GB-cori-high.onnx and .onnx.json from
  # https://huggingface.co/rhasspy/piper-voices/tree/main/en/en_GB/cori/high
  tools/GenerateVoiceClips.py en_GB-cori-high.onnx Data/sound

XCSoar plays a traffic alert as up to three clips in a row (urgency,
clock position, above/below). An airspace warning is one clip. Near
and above have a pause between the two words. Each clip ends with a
short pause.
"""

import io
import os
import sys
import wave

import numpy as np
from piper import PiperVoice, SynthesisConfig

# file name -> (text, length scale: smaller is faster)
CLIPS = {
    "traffic_low": ("Traffic.", 0.8),
    "traffic_important": ("Traffic important.", 0.75),
    "traffic_urgent": ("Traffic urgent!", 0.7),
    "above": ("Above.", 0.75),
    "below": ("Below.", 0.75),
    "level": ("Level.", 0.75),
    "airspace_near": ("Airspace near.", 0.75),
    "airspace_above": ("Airspace above.", 0.75),
    "airspace_inside": ("Airspace inside.", 0.7),
}

# Silence after the first word of these clips.
WORD_GAP_MS = {
    "airspace_near": 300,
    "airspace_above": 300,
}

CLOCK = [
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine",
    "ten",
    "eleven",
    "twelve",
]
for hour, word in enumerate(CLOCK, 1):
    CLIPS[f"oclock_{hour}"] = (f"{word.capitalize()} o'clock.", 0.75)

LEAD_MS = 40
TAIL_MS = 70
THRESHOLD = 0.02
PEAK_DBFS = -1.0


def synthesize(voice, text, length_scale):
    buffer = io.BytesIO()
    with wave.open(buffer, "wb") as w:
        voice.synthesize_wav(
            text, w, syn_config=SynthesisConfig(length_scale=length_scale)
        )
    buffer.seek(0)
    with wave.open(buffer, "rb") as r:
        samples = np.frombuffer(r.readframes(r.getnframes()), np.int16)
    return samples.astype(np.float32)


def trim_and_normalize(samples, rate, tail_ms=TAIL_MS - LEAD_MS):
    """Cut the silence around the speech and normalize the peak."""
    loud = np.where(np.abs(samples) / 32768 > THRESHOLD)[0]
    start = max(0, loud[0] - rate * LEAD_MS // 1000)
    end = min(len(samples), loud[-1] + rate * LEAD_MS // 1000)
    samples = samples[start:end]
    samples *= 32767 * 10 ** (PEAK_DBFS / 20) / np.max(np.abs(samples))
    pause = np.zeros(rate * tail_ms // 1000, np.float32)
    return np.concatenate([samples, pause]).astype(np.int16)


def synthesize_phrase(voice, text, length_scale, rate, gap_ms):
    """One utterance, or two words with gap_ms of silence between them."""
    if gap_ms <= 0 or " " not in text:
        return trim_and_normalize(synthesize(voice, text, length_scale), rate)

    first, rest = text.split(" ", 1)
    head = trim_and_normalize(
        synthesize(voice, first, length_scale), rate, tail_ms=gap_ms
    )
    tail = trim_and_normalize(synthesize(voice, rest, length_scale), rate)
    return np.concatenate([head, tail])


def main():
    if len(sys.argv) != 3:
        sys.exit(f"Usage: {sys.argv[0]} VOICE.onnx OUTPUT_DIR")

    voice = PiperVoice.load(sys.argv[1])
    rate = voice.config.sample_rate

    for name, (text, length_scale) in CLIPS.items():
        samples = synthesize_phrase(
            voice, text, length_scale, rate, WORD_GAP_MS.get(name, 0)
        )
        with wave.open(os.path.join(sys.argv[2], name + ".wav"), "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(rate)
            w.writeframes(samples.tobytes())


if __name__ == "__main__":
    main()
