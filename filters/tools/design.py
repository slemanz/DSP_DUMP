"""Designs every kernel and every test signal this module uses, and writes them
out as C. None of the generated numbers is ever typed into the firmware by hand.

    python3 tools/design.py            print the design summary
    python3 tools/design.py --write    also write the four generated files

Run it from the module root, so the paths below land in app/.

scipy's firwin takes the cutoff in Hz once fs is given, so there is no
normalised frequency to get wrong. MATLAB's fir1 takes it as a fraction of the
Nyquist rate, which is fs/2 and not fs.
"""

import sys
import numpy as np
from scipy.signal import firwin, freqz

FS = 2000.0             # sampling rate of everything in this module
SIG_LEN = 200           # 1 period of 10 Hz, 10 of 100 Hz, 50 of 500 Hz
NOISE_LEN = 600         # longer, because a standard deviation needs samples
NOISE_SIGMA = 0.30
NOISE_SEED = 7

if __name__ == "__main__":
    summary()
    if "--write" in sys.argv:
        print()
#       write_all()