"""Looks at one kernel: the taps, the response in dB, and the step response.

    python3 tools/plot.py lp_hamming
    python3 tools/plot.py ma_11 lp_hamming lp_blackman

This is what MATLAB's fdatool is for. The tool has a dropdown where this has a
function name: firwin is the windowed design, firls is Least-squares, and remez
is Equiripple. Naming them in a script instead of clicking them in a dialog is
the difference between a filter you can regenerate and one you cannot.
"""

import sys
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import freqz

from design import FS, KERNELS

names = sys.argv[1:] or ["lp_hamming"]

fig, axes = plt.subplots(3, 1, figsize=(8, 9))

for name in names:
    h = KERNELS[name]
    freq, values = freqz(h, worN=8192, fs=FS)

    axes[0].plot(h, ".-", label=f"{name} ({len(h)} taps)")
    axes[1].plot(freq, 20 * np.log10(np.maximum(np.abs(values), 1e-8)), label=name)
    axes[2].plot(np.cumsum(h), label=name)

axes[0].set_title("kernel")
axes[0].set_xlabel("tap")

axes[1].set_title("frequency response")
axes[1].set_xlabel("Hz")
axes[1].set_ylabel("dB")
axes[1].set_ylim(-100, 10)
axes[1].grid(True, alpha=0.3)

axes[2].set_title("step response, the running sum of the kernel")
axes[2].set_xlabel("sample")

for ax in axes:
    ax.legend(fontsize=8)

fig.tight_layout()
plt.show()