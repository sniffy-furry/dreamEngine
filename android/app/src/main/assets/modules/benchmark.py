"""
DreamEngine Python Benchmark
============================

Hot-swappable Python benchmark for DreamEngine.

Place this file in:
    Download/DreamEngine/modules/benchmark.py

The benchmark intentionally avoids external Python packages so it can run
inside the embedded CPython runtime.

It measures:
  - Python callback/update overhead
  - arithmetic workload
  - object allocation / garbage collection pressure
  - rolling frame/update time
  - periodic benchmark summaries

The engine should call:
    on_update(dt)

If the engine exposes an optional logging callback, this script will use it.
Otherwise it prints to stdout.
"""

from __future__ import annotations

import gc
import math
import time
from collections import deque


# ---------------- Configuration ----------------

WARMUP_SECONDS = 2.0
REPORT_EVERY_SECONDS = 2.0

# Work per update. Increase carefully on mobile.
ARITHMETIC_ITERATIONS = 2500
OBJECT_ITERATIONS = 150

# Keep enough samples for useful percentiles without growing forever.
MAX_SAMPLES = 4096


# ---------------- State ----------------

_started = False
_start_time = 0.0
_last_report = 0.0
_updates = 0

_frame_samples = deque(maxlen=MAX_SAMPLES)
_work_samples = deque(maxlen=MAX_SAMPLES)

_accumulator = 0.0
_sink = 0.0


def _now() -> float:
    return time.perf_counter()


def _log(message: str) -> None:
    # Keep this compatible with the simple embedded runtime.
    print("[PythonBenchmark] " + message, flush=True)


def _percentile(samples, p: float) -> float:
    if not samples:
        return 0.0

    ordered = sorted(samples)
    index = (len(ordered) - 1) * p
    lo = int(index)
    hi = min(lo + 1, len(ordered) - 1)
    frac = index - lo
    return ordered[lo] * (1.0 - frac) + ordered[hi] * frac


def _run_workload() -> float:
    """
    Deterministic-ish Python workload intended to exercise:
      - arithmetic
      - math functions
      - local variables
      - short-lived object allocation
    """
    global _accumulator

    x = _accumulator

    for i in range(ARITHMETIC_ITERATIONS):
        x += math.sin(i * 0.001 + x * 0.00001)
        x *= 0.999999

    # Deliberate temporary allocations.
    tmp = []
    for i in range(OBJECT_ITERATIONS):
        tmp.append({
            "i": i,
            "v": (x + i) * 0.0001,
            "ok": (i & 1) == 0,
        })

    # Consume the objects so the workload is not trivially optimized away.
    x += sum(item["v"] for item in tmp)

    _accumulator = x
    return x


def _report(force: bool = False) -> None:
    global _last_report

    now = _now()

    if not force and now - _last_report < REPORT_EVERY_SECONDS:
        return

    _last_report = now

    elapsed = max(now - _start_time, 1e-9)
    fps = _updates / elapsed

    p50 = _percentile(_frame_samples, 0.50) * 1000.0
    p95 = _percentile(_frame_samples, 0.95) * 1000.0
    p99 = _percentile(_frame_samples, 0.99) * 1000.0

    work_p50 = _percentile(_work_samples, 0.50) * 1000.0
    work_p95 = _percentile(_work_samples, 0.95) * 1000.0

    gc_counts = gc.get_count()

    _log(
        "updates=%d | avg=%.1f updates/s | "
        "frame p50=%.3f ms p95=%.3f ms p99=%.3f ms | "
        "work p50=%.3f ms p95=%.3f ms | "
        "gc=(%d,%d,%d)"
        % (
            _updates,
            fps,
            p50,
            p95,
            p99,
            work_p50,
            work_p95,
            gc_counts[0],
            gc_counts[1],
            gc_counts[2],
        )
    )


def on_start() -> None:
    global _started, _start_time, _last_report, _updates
    global _frame_samples, _work_samples

    _started = True
    _start_time = _now()
    _last_report = _start_time
    _updates = 0
    _frame_samples.clear()
    _work_samples.clear()

    gc.collect()

    _log("starting")
    _log(
        "config: arithmetic=%d object=%d warmup=%.1fs"
        % (ARITHMETIC_ITERATIONS, OBJECT_ITERATIONS, WARMUP_SECONDS)
    )


def on_update(dt: float) -> None:
    global _updates

    if not _started:
        on_start()

    frame_begin = _now()

    work_begin = _now()
    _run_workload()
    work_end = _now()

    frame_end = _now()

    _work_samples.append(work_end - work_begin)
    _frame_samples.append(frame_end - frame_begin)
    _updates += 1

    _report()


def on_stop() -> None:
    if not _started:
        return

    _report(force=True)

    elapsed = max(_now() - _start_time, 1e-9)
    avg_updates = _updates / elapsed

    _log("finished")
    _log(
        "final: %.1f updates/s | frame p95=%.3f ms | "
        "work p95=%.3f ms | samples=%d"
        % (
            avg_updates,
            _percentile(_frame_samples, 0.95) * 1000.0,
            _percentile(_work_samples, 0.95) * 1000.0,
            len(_frame_samples),
        )
    )


# Optional aliases for engines that use init/update/shutdown naming.
init = on_start
update = on_update
shutdown = on_stop
