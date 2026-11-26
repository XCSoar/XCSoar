#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project

"""Generate the spoken alert clips in Data/sound/.

The clips are spoken by the Piper voice "cori" (en_GB-cori-high,
trained on public-domain LibriVox recordings by Bryce Beattie).

Usage:
  pip install "piper-tts[alignment]>=1.8.0" numpy pyworld
  # en_GB-cori-high.onnx and .onnx.json from
  # https://huggingface.co/rhasspy/piper-voices/tree/main/en/en_GB/cori/high
  tools/GenerateVoiceClips.py en_GB-cori-high.onnx Data/sound

XCSoar plays a traffic alert as up to three clips in a row (urgency,
clock position, above/below). An airspace warning is one clip. Near
and above have a pause between the two words. The climb rate is
spoken word by word ("one", "point", "five"); its last word is cut
out of a sentence, so it falls like the end of one. Each clip ends
with a short pause.
"""

import io
import os
import sys
import wave

import numpy as np
import pyworld
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

# The words of a spoken number.  In the middle of a number they are
# said on their own, without a full stop.  The last word is cut out of
# a sentence so it keeps the falling pitch of a sentence end: said on
# its own, a word starts high and "one" or "six" even rises, so "zero
# point six" sounds like a question.  The comma makes a pause to cut
# in, so the first consonant of the digit is not lost.  "Hundred" is
# always the last word.
# file name -> (sentence, clip to match in length)
FINAL_CLIPS = {
    "hundred": ("three hundred.", None),
    "thousand_end": ("two thousand.", "thousand"),
}
for value, word in enumerate(["zero"] + CLOCK[:9]):
    CLIPS[f"number_{value}"] = (word, 0.75)
    FINAL_CLIPS[f"number_{value}_end"] = (f"point, {word}.", f"number_{value}")
for word in ("point", "minus", "thousand"):
    CLIPS[word] = (word, 0.75)
FINAL_LENGTH_SCALE = 0.75

# Where to look for the pause before the last word of a sentence
CUT_SEARCH_MS = 120
FADE_IN_MS = 5

# The voice says a word a little differently each time, so the words
# of a number are each picked from this many takes.  In the middle of
# a number, keep the take voiced as long as the median one.  As the
# last word, keep those voiced about as long as that median, so it
# sounds neither clipped nor rushed, and of these the one that falls
# most, so it sounds like the end of a sentence.  The voiced time
# counts, not the clip length: a long "s" or a gap inside "sev-en"
# would hide a rushed word.
TAKES = 12
FINAL_LENGTH_TOLERANCE = 0.1
# Quieter 10 ms frames are a pause, even with a pitch
VOICED_RMS = 0.01

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


def synthesize_last_word(voice, text, length_scale, rate):
    """The last word of a sentence, cut at the start of its first
    phoneme by the alignments of the voice."""
    chunks = list(
        voice.synthesize(
            text,
            syn_config=SynthesisConfig(length_scale=length_scale),
            include_alignments=True,
        )
    )
    if len(chunks) != 1 or chunks[0].phoneme_id_samples is None:
        sys.exit(f"No alignments for {text!r}")

    chunk = chunks[0]
    space_ids = set(voice.config.phoneme_id_map[" "])
    last_space = max(
        i for i, pid in enumerate(chunk.phoneme_ids) if pid in space_ids
    )
    boundary = int(np.sum(chunk.phoneme_id_samples[: last_space + 1]))
    samples = chunk.audio_float_array * 32767

    # The alignment is an estimate, and the first consonant of the
    # word ("z", "s", "n") often starts before it.  Cut in the
    # quietest 10 ms before the boundary instead, the pause between
    # the words, and fade in so the cut does not click.
    frame = rate // 100
    first = max(0, boundary - rate * CUT_SEARCH_MS // 1000)
    energy = [
        np.mean(samples[i : i + frame] ** 2)
        for i in range(first, boundary, frame // 2)
    ]
    start = first + int(np.argmin(energy)) * (frame // 2) if energy else 0
    samples = samples[start:].astype(np.float32)
    fade = rate * FADE_IN_MS // 1000
    samples[:fade] *= np.linspace(0, 1, fade)
    return trim_and_normalize(samples, rate)


def pitch_fall(samples, rate):
    """How far [Hz] the pitch falls from the first to the last quarter
    of the voiced part; negative if it rises."""
    f0, _ = pyworld.harvest(
        samples.astype(np.float64), rate, f0_floor=100, f0_ceil=450
    )
    voiced = f0[f0 > 0][2:-2]  # the edge frames carry octave errors
    if len(voiced) < 8:
        return 0
    quarter = len(voiced) // 4
    return float(np.median(voiced[:quarter]) - np.median(voiced[-quarter:]))


def voiced_length(samples, rate):
    """Number of 10 ms frames in which the voice sounds: with a pitch,
    and loud enough (the pitch tracker bridges short gaps)."""
    x = samples.astype(np.float64)
    f0, _ = pyworld.harvest(x, rate, f0_floor=100, f0_ceil=450,
                            frame_period=10)
    n = rate // 100
    return sum(
        1
        for i, pitch in enumerate(f0)
        if pitch > 0
        and np.sqrt(np.mean((x[i * n : (i + 1) * n] / 32768) ** 2))
        > VOICED_RMS
    )


def pick_final_take(takes, target, rate):
    """The take that falls most, of those within the length tolerance
    of #target voiced frames (or the closest, if none is)."""
    if target is not None:
        tolerance = target * FINAL_LENGTH_TOLERANCE
        voiced = [voiced_length(t, rate) for t in takes]
        fitting = [
            t for t, v in zip(takes, voiced) if abs(v - target) <= tolerance
        ]
        if not fitting:
            return takes[int(np.argmin([abs(v - target) for v in voiced]))]
        takes = fitting
    return max(takes, key=lambda t: pitch_fall(t, rate))


def write_clip(path, samples, rate):
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(samples.tobytes())


def main():
    if len(sys.argv) != 3:
        sys.exit(f"Usage: {sys.argv[0]} VOICE.onnx OUTPUT_DIR")

    voice = PiperVoice.load(sys.argv[1], include_alignments=True)
    rate = voice.config.sample_rate

    # the words of a number that a last word is matched to
    number_words = {match for _, match in FINAL_CLIPS.values() if match}

    clips = {}
    typical_voiced = {}
    for name, (text, length_scale) in CLIPS.items():
        if name not in number_words:
            clips[name] = synthesize_phrase(
                voice, text, length_scale, rate, WORD_GAP_MS.get(name, 0)
            )
            continue

        takes = [
            synthesize_phrase(voice, text, length_scale, rate, 0)
            for _ in range(TAKES)
        ]
        voiced = [voiced_length(t, rate) for t in takes]
        typical_voiced[name] = np.median(voiced)
        clips[name] = takes[
            int(np.argmin([abs(v - typical_voiced[name]) for v in voiced]))
        ]

    for name, (text, match) in FINAL_CLIPS.items():
        takes = [
            synthesize_last_word(voice, text, FINAL_LENGTH_SCALE, rate)
            for _ in range(TAKES)
        ]
        clips[name] = pick_final_take(
            takes, typical_voiced.get(match), rate
        )

    for name, samples in clips.items():
        write_clip(os.path.join(sys.argv[2], name + ".wav"), samples, rate)


if __name__ == "__main__":
    main()
