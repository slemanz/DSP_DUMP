# Streaming and Block Processing

The sensor hands over one sample. Convolution wants the whole array. The DFT
wants a window. Every function in CMSIS-DSP takes a pointer and a length, and
none of them takes a number.

That mismatch is this chapter, and there are exactly two ways across it:
keep the history inside the filter and run it once per sample, or collect
samples until there are enough and run it once per block.

The useful thing to know first is that these are not two techniques. They are
one parameter. Streaming is a block of length 1, the arithmetic is identical,
and the answers agree to the last bit. What changes is *when* the work happens,
and that shows up in two places that cannot both be small.

## The Slot That Is Not There

The ring buffer both arrangements are built on has a property worth meeting
before it bites. Two pointers walk round an array, and the buffer is empty when
they are equal. That rule is what lets the structure work without a separate
counter, and it costs one slot: the state where the pointers are equal has
already been spent meaning empty, so it cannot also mean full.

An array of 64 holds 63. [`blk_fifo`](app/Src/blk_fifo.c) fills one until it
refuses and counts what went in.

Losing one slot in 64 does not matter. Not knowing about it does, because the
collecting loop fills until `put` fails and the reading loop asks for the array
length, and the last `get` then finds the ring empty, returns a failure nobody
reads, and writes nothing. The destination keeps whatever was there.

That is the dip at the end of every block in the source material, which the
narration explains as a property of block processing and offers to filter out.
[`blk_dip`](app/Src/blk_dip.c) reproduces it and prints the last four samples of
the block three ways:

```
                                n-4      n-3      n-2      n-1
  asking for 64, lesson way   160.0    161.0    162.0      0.0
  asking for 64, checking     160.0    161.0    162.0      0.0
  asking for 63               159.0    160.0    161.0    162.0
```

The middle row is the interesting one. Reading the return value and stopping
early produces an array identical to the first. **Checking the status bought
information, not correctness.** The data is wrong either way; the difference is
whether the program knows. The fix is asking for the number the ring actually
holds.

## A Filter With No Array

[`blk_stream`](app/Src/blk_stream.c) is the first bridge. The filter keeps the
last `numTaps` inputs in a circular buffer inside its own instance, so every
call already has the window it needs and nobody has to assemble one.

The result is not approximately the block answer. It is the block answer, to
1.8e-07, which is float rounding. Same multiply accumulate, same numbers,
different bookkeeping. The only thing streaming gives up is the tail: the
`numTaps - 1` outputs where the filter is still ringing after the input stopped,
which a stream that has not stopped does not have yet.

What it costs is that the whole filter runs inside whatever context the sample
arrives in, once per sample, at the sampling rate. Seventeen taps at 2 kHz is
fine. Two hundred taps at 48 kHz is not, and that is the argument for the other
bridge.

## The Seam

Collecting a block and convolving it is the obvious second bridge and it is
wrong in a way worth seeing. [`blk_block`](app/Src/blk_block.c) does exactly
that and compares against streaming:

```
  worst gap against streaming      0.960512
  samples that disagree                 112 of 256
  blocks with a damaged start             7
  damaged samples in each                16
  and taps minus one is                  16
```

The damage is not noise and it is not spread around. It is exactly the first
$M-1$ samples of every block after the first, because each block was convolved
as if the signal began there, so the filter started from silence eight times
instead of once. Block zero is the exception, and not because it is fine: it is
the one place where starting from silence is correct, which is what streaming
does too.

[`blk_seam`](app/Src/blk_seam.c) closes it two ways that turn out to be the same
way.

**Overlap and save** is the explicit one. Hand the filter the last $M-1$ samples
of the previous block along with the new ones, convolve the longer thing, and
discard the outputs the overlap already covered.

**A state buffer** is the tidy one. `arm_fir_f32` keeps that overlap inside its
instance, so the caller passes only new samples and gets back exactly as many
outputs, with the join handled where it cannot be forgotten. The state buffer it
asks for is `numTaps + blockSize - 1` floats, and that is what those floats are.

Both land on the streaming answer, and on each other exactly:

```
  overlap and save                    0.000000179
  arm_fir_f32 state buffer            0.000000179
  the two against each other          0.000000000
```

That last zero is the point. They are the same idea written twice: something has
to remember the last $M-1$ samples across the boundary, and the only question is
whether it is you or the library. It also answers what the filter design chapter
left hanging, where one call of 200 samples matched two calls of 100. This is
why that function wants an instance rather than a pair of arrays.

## Choosing the Block Length

[`blk_latency`](app/Src/blk_latency.c) puts numbers on the trade at a fixed
sampling rate:

| block | latency ms | macs/sample | macs/burst | bursts/s |
| --- | --- | --- | --- | --- |
| 1 | 0.50 | 17 | 17 | 2000 |
| 8 | 4.00 | 17 | 136 | 250 |
| 32 | 16.00 | 17 | 544 | 62 |
| 128 | 64.00 | 17 | 2176 | 15 |

The third column never moves. The work per sample is the filter, and rearranging
when it happens does not change how much of it there is. Block of 1 is in the
table on purpose, because block of 1 is streaming.

Latency is the block length over the sampling rate, and no processor makes it
smaller: the first sample of a block cannot be filtered until the last one has
arrived. The burst column is what has to fit between two samples if the work is
not allowed to overrun, and it grows with the block while the time available
does not.

So the block length is chosen from whichever end is binding. A control loop
picks it from the latency column. A path with a hard deadline picks it from the
burst column. Nothing picks it from the third one.

## Does It Fit

The acquisition chapter ended by measuring how much of a sample period the
handler spent just storing a number. [`blk_budget`](app/Src/blk_budget.c) is the
same question with the filter included, and it has a definite answer:

$ \text{cycles per sample} = \frac{f_{HCLK}}{f_s} $

That is the budget, and everything else the chip has to do comes out of the same
place. Where the measured cost approaches it, the answer is not to optimise the
loop. It is that the design is wrong at that rate, and the things that move it
are fewer taps, a cheaper number format, or instructions that do more than one
multiply at a time. Those are the next two chapters.

## Watching the Signals in Ozone

The chapter is one signal through two arrangements, so the graph carries the
input once and both answers side by side:

| probe | carries |
| --- | --- |
| `g_in` | the signal going in |
| `g_stream` | the answer computed one sample at a time |
| `g_block` | the answer computed a block at a time |
| `g_gap` | the difference between them |

`g_gap` is the trace that carries this chapter. When the arrangement is right it
is a flat line at zero, and when it is not, the shape of what is left says which
seam leaked: eight evenly spaced teeth in [`blk_block`](app/Src/blk_block.c),
nothing at all in [`blk_seam`](app/Src/blk_seam.c).

They are declared in [`probe.c`](app/Src/probe.c) and wired into the Data
Sampling window by [`app.jdebug`](workspaces/app.jdebug). `STEP_MS` is 100
rather than a few milliseconds, because the sampler misses points at the faster
rate and a trace with holes in it reads as a seam that is not there.

## Apps

Each app is a self-contained `main` that prints its numbers once and then
streams the probes, so `make monitor` catches the output and the Timeline shows
the traces. This chapter is plain C apart from the last app, so everything
except the timing can be checked without the board.

0. [The ring and its capacity](app/Src/blk_fifo.c): a 64 entry buffer filled
   until it refuses, drained back in order, and then run round the end
   repeatedly to show that the wrap is the ordinary case and the missing slot is
   the special one.
1. [The dip explained](app/Src/blk_dip.c): the last four samples of a block
   collected three ways, where asking for one more than the ring holds leaves a
   zero, checking the return value leaves the same zero, and asking for the
   right number leaves a block.
2. [Filtering one sample at a time](app/Src/blk_stream.c): a circular buffer
   inside the filter instance, giving the same answer as `arm_conv_f32` on the
   whole array and giving up only the tail.
3. [Where blocks go wrong](app/Src/blk_block.c): each block convolved on its
   own, with the damage located exactly at the first `numTaps - 1` samples of
   every block after the first and counted rather than described.
4. [Two ways to close it](app/Src/blk_seam.c): overlap and save written out, and
   `arm_fir_f32` doing the same thing inside its state buffer, both landing on
   the streaming answer and on each other exactly.
5. [Latency against burst](app/Src/blk_latency.c): block lengths from 1 to 256
   with the latency they impose and the work they concentrate, and the column
   that stays still through all of it.
6. [The budget](app/Src/blk_budget.c): the filter timed per sample both ways
   against the cycles a sampling rate leaves for it, from 1 kHz up to 96, with
   the rates where it stops fitting marked.

