# Clock Tree and Flash Latency

Everything in the previous chapters was arithmetic on numbers already in
memory, and the only thing the hardware contributed was somewhere to run. This
one is about the machine, starting with how fast it goes.

The chip wakes up at 16 MHz on an internal RC oscillator. It is rated for 100.
Taking the other 84 is a page of register writes, and doing it in the wrong
order does not give a slow chip, it gives one executing whatever happened to be
on the bus.

The single most useful idea here is that two different things are being bought
and they must not be confused:

```
raising the clock   divides the milliseconds and leaves the cycles alone
fixing the memory   divides the cycles and leaves the clock alone
```

A cycle is a unit of work. A frequency only says how long a cycle lasts. So
going from 16 to 100 MHz does not make the code more efficient and never will;
it makes the same inefficiency take less time. Every measurement in this
repository quoted in cycles survives the change and every one quoted in
milliseconds does not, which is why the convolution and DFT chapters report
both.

## What the Board Actually Has

A clock configuration is not portable. It is a property of the crystal soldered
to the board, and the numbers below are for a WeAct BlackPill with a **25 MHz**
part. A board fed 8 MHz from a debugger's MCO pin needs entirely different ones,
and it also needs `HSEBYP` set, because that is an external clock signal rather
than a crystal for the oscillator to drive.

The PLL divides its input by $M$, multiplies by $N$, and divides by $P$:

$ f_{sys} = \frac{f_{in}}{M} \times N \div P $

which for this board is

$ \frac{25\ \mathrm{MHz}}{25} \times 200 \div 2 = 100\ \mathrm{MHz} $

That looks like a lot of freedom and it is not. The divided input has to land
between 1 and 2 MHz and the multiplied result has to land between 100 and 432
MHz, so most triples that reach the right answer are not allowed to.
[`clock_pll`](app/Src/clock_pll.c) runs candidates through both windows:

```
       in    M     N   P     vco in     vco out     sysclk
 25000000   25   200   2    1000000   200000000  100000000 ok
 25000000   13   104   2    1923076   200000000  100000000 ok
 25000000   12    96   2    2083333   200000000  100000000 NO
 25000000   50   400   2     500000   200000000  100000000 NO
 25000000    4   200   4    6250000  1250000000  312500000 NO
  8000000    4   200   4    2000000   400000000  100000000 ok
```

The last two rows are the same three numbers on two different boards. On an
8 MHz part they give exactly 100 MHz; on this one they ask for 312.5 MHz with a
1.25 GHz VCO. The second row is worth noticing too: it reaches the same 100 MHz
with the VCO input nearer 2 MHz, which is where ST wants it for jitter, so there
are two exact answers and the difference between them is phase noise.

USB explains why so much code for this board runs at 96 rather than 100. The
peripheral needs exactly 48 MHz from $f_{vco}/Q$ with $Q$ a whole number, and
200 divides into no such thing while 192 does, at $Q = 4$.

## Order Is Not Optional

Three things have to be told before the frequency goes up, and the same three in
reverse before it comes down.

The **regulator** first: 100 MHz needs voltage scale 1, and reset leaves the part
in scale 2, whose ceiling is 84 MHz. The **flash** next, because it cannot answer
at 100 MHz and has to be given its wait states before anything asks it to. And
**APB1**, because it stops at 50 MHz, so it must already be divided when the
core arrives at 100.

Only then may the PLL become the system clock. Getting this backwards is not a
performance bug.

[`clock_config_100mhz`](drivers/Src/driver_clock.c) also refuses to wait
forever. If the crystal never starts it gives up, leaves the chip on the
internal oscillator and returns 0, because a cracked crystal should look like a
diagnosable failure rather than a boot that hangs with nothing on the terminal.

## Asking Rather Than Remembering

`clock_get` works the frequency out from RCC every time it is called. It stores
nothing, so it cannot be out of date.

That is the whole reason the rest of the system survives a clock change. The
SysTick reload and the UART baud divider both call it, so when the PLL comes on
they follow, and there is no list of constants to remember to edit. The usual
alternative is a `#define SYS_FREQ` that has to be kept in step by hand, and the
usual outcome is a terminal full of garbage the first time it is forgotten.

One detail matters for the baud rate: `clock_get` returns SYSCLK, and UART2 does
not run on SYSCLK. It hangs off APB1, which is divided by two at this frequency,
so the divider has to come from `clock_pclk1`. UART1 and UART6 are on APB2 and
need `clock_pclk2`. Getting it from the wrong one puts the baud rate out by
exactly a factor of two.

That this page of text arrives unmangled is therefore a real check on the whole
tree, and the only number on [`clock_tree`](app/Src/clock_tree.c)'s output that
verifies itself.

## Believing It

Everything `clock_tree` prints was decoded from the registers that were written
a moment earlier, so it is consistent with itself and proves nothing about the
crystal. If this board had an 8 MHz part fitted instead, every line would still
read 100 MHz and every one would be wrong by a factor of three.

[`clock_verify`](app/Src/clock_verify.c) is the two checks that come from
outside the chip. The LED toggles every 500 SysTick ticks, so twenty toggles is
ten seconds against a wristwatch if the tree is honest. And MCO1 puts a chosen
clock straight onto PA8 with a divider of up to five, which is the only way to
read the oscillator itself: HSE over 5 is 5 MHz for a 25 MHz crystal and 1.6 MHz
for an 8 MHz one, and those are hard to confuse on a logic analyser.

## The Memory in Front of the Core

Flash does not answer at 100 MHz. It answers at something nearer 25, so the core
is told to wait three cycles before believing what is on the bus. The boundaries
are 30, 64, 90 and 100 MHz on a 3.3 V part, and below the required number the
chip is not faster, it is wrong.

[`clock_latency`](app/Src/clock_latency.c) measures the other direction, where
each extra wait state is a real tax, and it does the measuring with the caches
switched off. Left on they hide almost all of it and the table comes out flat,
which is a true result and belongs to the next app.

Because the ART accelerator is the part of the speedup nobody mentions. Three
wait states means the flash keeps up with a quarter of the core, so a core
fetching every instruction from flash would run at a quarter speed no matter
what the PLL says. A prefetch queue, an instruction cache and a data cache are
what stop that, and [`clock_cache`](app/Src/clock_cache.c) runs the same
arithmetic at the same 100 MHz through all four combinations.

Worth holding these two in the right proportion. The PLL is the famous half of
the speedup and it is bounded: 16 to 100 MHz is 6.25 times and no more exists.
The caches are the unglamorous half, they are worth a similar factor on a
flash bound loop, and they cost three bits in a register.

## The Multiplier Hiding Among the Prescalers

Timers on an APB bus do not run at that bus's clock. When the prescaler is 1
they do, and the moment it is anything else the timer clock is doubled back up.

| APB1 pre | PCLK1 | TIM clock | bus legal |
| --- | --- | --- | --- |
| /1 | 100 MHz | 100 MHz | over 50 MHz |
| /2 | 50 MHz | 100 MHz | ok |
| /4 | 25 MHz | 50 MHz | ok |
| /8 | 12.5 MHz | 25 MHz | ok |

The `/1` row is the only one where the timer sees what the bus sees, and at this
frequency it is also the only one that is not allowed. So on this configuration
PCLK1 is 50 MHz and TIM2 counts at 100.

It is a real multiplier drawn into the clock tree, not a rounding, and it is
there so that slowing a bus down for its peripherals does not also halve the
resolution of its timers. Every timer period has to be computed from the doubled
number. Missing it gives a timer running at exactly twice the rate that was
asked for, which is the easiest mistake in the next chapter to make and the
hardest to see, because everything about it looks right.

[`clock_buses`](app/Src/clock_buses.c) works that table out rather than
programming it. Putting APB1 at `/1` while the core is at 100 MHz would run the
bus at twice its rating, and UART2 is on that bus.

## What It Was All For

[`clock_speed`](app/Src/clock_speed.c) runs one fixed workload at both clocks in
a single session and reports cycles and microseconds side by side. The cycle
column should be flat and the microsecond column should fall by 6.25. If the
cycle column is not flat, something other than the frequency changed with it,
and on this part that means the flash wait states that had to go up too.

One consequence is easy to get backwards. The cycle counter is 24 bits and
counts at HCLK, so its ceiling is 1.048 s at 16 MHz and 0.168 s at 100. The
faster the core, the shorter the interval it can measure in one go.

## Watching the Signals in Ozone

This chapter measures the machine rather than a signal, so the traces carry
numbers about the machine:

| probe | carries |
| --- | --- |
| `g_mhz` | whatever clock the app is talking about, in MHz |
| `g_cycles` | what the fixed workload cost, in cycles |
| `g_ms` | what those cycles came to in time |
| `g_gain` | the ratio against the slowest configuration |

Keeping `g_cycles` and `g_ms` on separate traces is the chapter in one picture:
in [`clock_speed`](app/Src/clock_speed.c) one of them is a flat line and the
other is a step.

They are declared in [`probe.c`](app/Src/probe.c) and wired into the Data
Sampling window by [`app.jdebug`](workspaces/app.jdebug). Every app calls
`probe_reset` first, because the linker runs with `--gc-sections` and a probe no
app in the build mentions is dropped from the image. `STEP_MS` is 100 rather
than a few milliseconds, because the sampler misses points at the faster rate
and a trace with holes in it reads as a measurement that went wrong.

## Apps

Each app is a self-contained `main` that prints its numbers once and then
streams the probes, so `make monitor` catches the output and the Timeline shows
the traces. Three of them measure cycles and only mean anything on the board.

0. [The tree read back](app/Src/clock_tree.c): every clock in the chip decoded
   from RCC and FLASH at the moment it is printed, with each bus checked against
   its own limit, and the regulator, wait states and cache bits reported
   alongside.
1. [Choosing M, N and P](app/Src/clock_pll.c): candidate PLL settings for this
   board run through the VCO's two windows, including the ones that reach the
   right frequency illegally and the course's own values, which are correct on
   an 8 MHz board and ask for 312.5 MHz on this one.
2. [Checking from outside](app/Src/clock_verify.c): the LED against a
   wristwatch and the oscillator itself on PA8 through MCO1, because everything
   the registers say is only consistent with the registers.
3. [What wait states cost](app/Src/clock_latency.c): the same workload at three
   through seven wait states with the caches off, where the number that moves is
   cycles and the clock never changes.
4. [What the caches give back](app/Src/clock_cache.c): the same workload at the
   same clock through all four combinations of prefetch and the two caches,
   which is the largest single speedup on the part and costs three bits.
5. [Prescalers and the doubler](app/Src/clock_buses.c): what each APB1 divisor
   gives the bus and what it gives the timers on that bus, worked out rather
   than programmed, ending in the count a 1 kHz timer period actually needs.
6. [Cycles against time](app/Src/clock_speed.c): one workload at 16 MHz and at
   100 in a single run, with the cycle count staying still and the wall clock
   time falling, and the cycle counter's own window shrinking as the core speeds
   up.

