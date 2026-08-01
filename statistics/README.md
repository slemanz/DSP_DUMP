# Signal Statistics and Noise

Before a signal can be filtered or transformed it has to be described, and the
description starts with two numbers: where the signal sits, and how much it
moves. Those are the mean and the standard deviation, and almost everything
that follows in DSP leans on them. They are also the cheapest measurements you
can make on a microcontroller, which makes them the natural place to start. The
runnable examples live under [`app/Src/`](app/Src/), one file per concept, and
are built on the project skeleton described under
[Template](../template/README.md).
