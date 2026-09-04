# Optimization Strategies

One loop, taken down a ladder, with one number followed all the way: cycles per
multiply accumulate. That is the only measurement that survives a change of
workload, because total cycles depend on how many samples there were and cycles
per operation does not.

Six of the apps here are rungs on that ladder. The seventh is not a rung, it is
stepping off the ladder, and it matters more than the other six together.

## Two Different Kinds of Faster

Everything from `-O2` through hand written SIMD rearranges the same arithmetic.
The number of multiply accumulates does not change; each one gets cheaper.

Changing the algorithm changes the count. A direct DFT is $N^2$ multiply
accumulates and an FFT is closer to $N \log N$:

| N | $N^2$ | $N \log_2 N$ | ratio |
| --- | --- | --- | --- |
| 128 | 16384 | 896 | 18 |
| 512 | 262144 | 4608 | 56 |
| 4096 | 16777216 | 49152 | 341 |

That ratio grows with the problem while nothing else in this chapter changes
with it at all. So the order to try things in is not a matter of taste: count
the operations first, and only then start making each one cheaper.

[`opt_algorithm`](app/Src/opt_algorithm.c) puts the two side by side so the
proportion is a measurement rather than an assertion.

## Everything Costs Something

The strategies are usually presented as free. They are not, and the price is
worth writing down beside each one:

| rung | costs |
| --- | --- |
| `-O2` instead of `-O0` | debuggability |
| unrolling | code size and readability |
| q15 instead of f32 | precision |
| `SMLALD` | portability to any core without it |

The first is nearly free and is the one most often skipped, because the build
that gets measured is usually the build that was already open in the debugger.

## `-O0` Is Not Neutral

`-O0` is not "C without optimisation". It is a specific and deliberately naive
code generator: every variable goes back to the stack after every statement so a
debugger can always find it, and the loop counter is loaded and stored on every
iteration.

The same loop compiled five ways, measured on the target:

```
  dot_o0     98 bytes
  dot_o1     44 bytes
  dot_o2     36 bytes
  dot_o3     36 bytes
  dot_os     32 bytes
```

Nearly three times the code for the same source, and the whole difference is
saving and reloading things that never needed to leave a register. Measuring a
DSP routine in a debug build is the most common way to reach a wrong conclusion
about it.

`-O3` is not automatically the winner either. It unrolls and inlines harder than
`-O2`, which makes the code bigger, and bigger code on a part with a small
instruction cache can lose more to fetching than it gains to executing.

All five levels live in one binary. GCC lets a function carry its own
optimisation level through `__attribute__((optimize("O2")))`, which turns five
builds compared from memory into one table.

## Unrolling Does Two Things

Every iteration pays for the loop as well as the work: increment, compare,
branch. On this core that is about three cycles of bookkeeping, and if the body
is one multiply accumulate then most of the loop is not arithmetic.

Unrolling by $n$ divides that by $n$. It also gives the compiler several
independent chains to interleave, which matters because the floating point unit
stalls a cycle when an instruction needs the result of the one before it. That
is why [`opt_unroll`](app/Src/opt_unroll.c) uses `s0..s3` rather than adding
everything into one variable: four accumulators give the unit something else to
do while it waits.

The returns stop, and the reason is on the same page:

```
  dot_1      36 bytes
  dot_8     264 bytes
```

Seven times the code. Past some point the overhead is already small, the
register file runs out, and the extra instructions cost more to fetch than the
saved branches were worth.

One arithmetic note: `s0 + s1 + s2 + s3` is not bit for bit what adding in order
gives, because float addition is not associative. The convolution chapter
measured that difference at 1.19e-07 and it is the same effect.

## A Promise the Compiler Cannot Make Itself

A function taking two pointers and writing through one has a problem the source
does not show. The compiler cannot know they do not overlap, so after every
store through the output it must assume the input may have changed, and reload
it.

`restrict` is the promise that they do not. It generates no code; it removes a
reload the compiler had no choice about.

[`opt_memory`](app/Src/opt_memory.c) measures it, and the honest result is worth
stating in advance: if `restrict` buys nothing on that loop, that is correct
rather than disappointing. The inner loop there has no store in it, the
accumulator is already a local, so there was no reload to remove. That is the
useful shape of the rule. `restrict` is worth reaching for when a function
writes through one pointer while reading another, and worth nothing when it does
not.

The advice to group loads and stores is the same idea from the instruction side.
A load or store costs two cycles alone and one when it follows another, so
consecutive accesses cost $n+1$ rather than $2n$, and unrolling groups them for
free.

## Fused, or Not

A multiply accumulate computes $ab + c$, and the floating point unit has one
instruction for it. The usual advice is the opposite of what you would guess:
prefer a separate multiply and add.

The reason is real. The addition needs the result of the multiplication, so the
fused instruction has a stall built into it, and splitting it lets two of them
interleave and cover each other. But that is not a claim about one instruction
against two. It is a claim about whether there is anything else to do during the
stall, and it stops being true the moment there is only one chain of work. Which
is why [`opt_fma`](app/Src/opt_fma.c) has four rows rather than two, read in
pairs.

There is also an arithmetic difference with nothing to do with speed. The fused
instruction keeps the full precision of the product and rounds once; the split
version rounds twice. The fused answer is the more accurate one, so this rung
can cost precision as well as buy cycles.

## Two Multiplies in One Instruction

This is the rung the fixed point chapter was pointing at. Two q15 values fit in
one register, and the instruction set has multiply accumulates that take both
halves at once:

```
SMLAD    Rd = Rn.bottom * Rm.bottom + Rn.top * Rm.top + Ra
SMLALD   the same, accumulating into 64 bits
```

One instruction, one cycle, two multiplies and three additions. Nothing the
float unit has comes close, and it exists only because the data is sixteen bits
wide, which is the whole argument for fixed point on this core.

**The compiler will not find this from the plain loop, and it cannot.** Reading
two q15 values as one word is a promise about alignment that C does not let it
assume, so this is one of the few places where writing the instruction by hand
is the only route. The arrays carry `__attribute__((aligned(4)))` and the loop
starts on an even index for exactly that reason.

The three versions in [`opt_simd`](app/Src/opt_simd.c) give identical answers,
which they can because integer addition does not care about order. For floats
they would not, and that is one more reason this rung belongs to fixed point.

## Rungs Do Not Multiply

Each app above reports its own ratio against its own baseline: `-O2` against
`-O0`, unrolling against no unrolling, a fused multiply-add against a split
one, `SMLALD` against one multiply at a time. It is tempting to multiply those
ratios together into a single compounded expectation, and doing so overstates
the truth.

The reason is that a later rung's baseline is not independent of the earlier
ones. Once `-O2` has already stopped reloading the loop counter from the stack,
unrolling only has the branches left to remove. Once unrolling has already put
several accumulators in flight, splitting the fused multiply-add is covering a
stall that the accumulators had already covered part of. Each rung's measured
ratio quietly includes some of the gain a rung before it was also claiming.

Run the same loop through `opt_levels` to `opt_simd` in one sitting, multiply
their four ratios, and compare that number against the target running all four
changes at once. The gap between the two is not a mistake in either
measurement — it is the whole content of this section.

## A Trap Found Twice

Writing [`opt_algorithm`](app/Src/opt_algorithm.c) reproduced a finding from the
DFT chapter without meaning to. Built with the generic `arm_rfft_fast_init_f32`
the image came to 108180 bytes of text; with `arm_rfft_fast_init_128_f32`, which
brings only the twiddle table that was asked for, it came to 29740.

Seventy eight kilobytes, and the DFT chapter measured seventy seven for the same
swap at a different length. The trap is consistent, and it is worth checking
every time a transform is used at a fixed size.

## Reading What the Compiler Did

One of the strategies is to look at the generated assembly, which is advice
without a tool unless there is a way to run it. There is a make target:

```
make asm APP=opt_unroll
arm-none-eabi-objdump -d Build/flash.elf | less
```

What to look for is not the whole listing. It is whether the multiply
accumulates are there at all, whether they are surrounded by loads and stores
that should have been hoisted, and whether the instruction you asked for by name
actually appeared. `smlald` shows up three times in
[`opt_simd`](app/Src/opt_simd.c)'s image and `vfma.f32` ten times in
[`opt_fma`](app/Src/opt_fma.c)'s, which is how those two apps were confirmed to
be measuring what they claim.

## Watching the Signals in Ozone

Nothing here is a signal. The chapter measures the same arithmetic arranged
differently, so the traces carry the measurement:

| probe | carries |
| --- | --- |
| `g_cycles` | what the variant cost |
| `g_per_mac` | the same, per multiply accumulate |
| `g_speedup` | how much better than the starting point |
| `g_bytes` | whatever the app has left to report |

`g_per_mac` is the one to follow across apps, because it is the only number that
survives a change of workload.

`STEP_MS` is 100 rather than a few milliseconds, because the sampler misses
points at the faster rate and the steps come out ragged.

## Apps

Each app is a self-contained `main` that prints its table once and then streams
the probes, so `make monitor` catches the output and the Timeline shows the
steps. Every measurement here is in cycles, so unlike the earlier chapters none
of it can be checked on a host.

0. [What the flag is worth](app/Src/opt_levels.c): the same loop five times,
   differing only in the optimisation level each function carries, with `-O0`
   shown as a specific code generator rather than the absence of one.
1. [Unrolling](app/Src/opt_unroll.c): the same loop by 1, 2, 4 and 8, separating
   the branches saved from the stalls covered, and showing the code size that
   pays for both.
2. [Aliasing](app/Src/opt_memory.c): the same filter with and without
   `restrict`, and hoisted by hand, with a checksum column so a faster wrong
   answer cannot pass.
3. [Fused or split](app/Src/opt_fma.c): the multiply accumulate as one
   instruction and as two, at one chain and at two, which is the condition the
   usual advice leaves out, plus the rounding difference between them.
4. [SIMD](app/Src/opt_simd.c): `SMLALD` doing two q15 multiplies per
   instruction, the alignment it requires, and confirmation that the compiler
   cannot reach it from the plain loop.
5. [The rung that is not one](app/Src/opt_algorithm.c): a direct transform
   against an FFT, set against everything above it, and extended out to the
   lengths where the gap stops being a factor and starts being a decision.

