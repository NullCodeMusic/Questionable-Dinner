# QUESTIONABLE DINNER USER MANUAL
## QD001 A Saw B
A Saw B Is a multi module with a window comparator, difference rectifier, and simple subtraction. A and B ports are normalled to A and B inputs from above.
> **Comparator** ( **A>B**, **A<B**, **A=B**, **A!=B** )
> 
> These four outputs send gate signals based on their respective comparisons. The **window** knob controls a range in which A and B are considered equal. It's useful in situations where you want to ignore negligible voltage differences.
>
> **Diff-Rect** ( **Over**, **Under** )
> 
> Over outputs how much the voltage at A exceeds the voltage at B. `A - B if A is greater than B, 0 otherwise`
> 
> Under outputs how much the voltage at B exceeds the voltage at A. `B - A if A is less than than B, 0 otherwise`
>
> Both of these outputs are also affected by the **window** knob, which can be used for interesting wave morph effects.
>
> **Subtract** ( **A-B** )
> 
> Simple subtraction. The **window** know has no effect on this output!
## QD002 Pilfer
Pilfer is a filter-adjacent module that tries to replicate incoming signals using a moving particle. It takes a sample of the input signal as a coordinate, then accelerates the particle towards that position. 
> **Acceleration** and **friction** govern the motion of the particle. 
> 
> **Distance** determines how far the particle has to be from the input signal for it to start accelerating.
> 
> **Bounce** determines how much momentum is conserved when the particle hits against the the boundary (12 units from center) and bounces off.
> 
> **Drive** boosts input signals and then scales down output signals by the same amount.
>
> **Shake** directly adds to the current velocity of the particle.

Pilfer also has a number of discrete modes and switches.

> **Coordinate Systems**
> 
> Pilfer can use 1-dimensional, 2-dimensional polar, and 2-dimensional rectangular coordinate systems for its inputs. When only **X/R** is plugged in, pilfer automatically runs in 1-dimensional mode. When both **X/R** and **Y/θ** are plugged in, it runs in 2-dimensional mode. The switch in the center of the module (in a yellow box) switches between polar and rectangular coordinates.
> 
> In rectangular coordinates, **X/R** and **Y/θ** are used as X and Y grid coordinates. The boundary is a square with side length 12.
>
> In polar coordinates, **X/R** and **Y/θ** are used as radius and angle coordinates. The boundary is a circle with radius 12.
> 
> **Friction**
>
> The particle loses momentum every tick. This is by default an exponential decay, but can be switched to linear. When friction is linear, it corresponds to acceleration.
>
> **Frequency Range**
> 
> Pilfer can be switched between low frequency which is good for use on CV signals, and high frequency which is good for audio signals.

## QD003 Surgeon
Surgeon is a live-coded additive synth. It controls up to 64 sine wave partials using typed-in math expressions. 
### Expressions
There are 9 expressions that can be programmed into this oscillator. All of these are calculated on a per-partial basis (variables i and n are used to get the index of the current partial, and the number of total partials respectively). The header changes to tell you what expression you currently have hovered. From top to bottom, the expressions are:
> **Variable** `j` - A variable that can be referenced in expressions.
>
> **Variable** `k` - A variable that can be referenced in expressions.
>
> **Variable** `l` - A variable that can be referenced in expressions.
>
> **Time Warp** - An offset to the envelope, phase, and the time variable, `t`. 
>
>**Env Attack** - The number of seconds it takes for the envelope to rise.
> 
>**Env Decay** - The number of seconds it takes for the envelope to fall.
>
> **Pitch** - The pitch distribution of partials.
>
> **Amp** - The amplitude distribution of partials.
>
> **Phase** - The phase distribution of partials. When the expression is set to 0, phase is not reset on trig.
>
Note: These expressions are calculated in order from top to bottom, per-partial. If you reference them out of order, calculations will no longer line up with the correct current partial index, `i`. Sometimes this can be fixed by substituting `i` with (`i-1`) to compensate for the shift, but this might not always work.
### Variables & Constants
> `i` - The index of the current partial, starting at 1
>
> `n` - The total number of partials
>
> `r` - Sample and hold pseudo random value per partial. The **seed** knob determines the seed for the random number generator.
>
> `t` - Time since last trig
>
> `f` - Base frequency in hz, defined by v/Oct input
>
> `e` - Envelope
>
> `out` - Previous oscillator output
>
> `x`, `y`, and `z` - Param/CV in 
>
> `j`, `k`, and `l` - User defined variables
>
>`pi`
## QD004 Elastic Twin
Elastic twin is a delay/looper plugin that records to and swaps between two buffers (per polyphony channel) to cut apart and rearrange audio. With no set buffer size, no set delay time, this module is great at making garbled, beautiful messes out of input audio.
> When writing to a buffer, values are added to the top of the buffer. 
>
> When reading a buffer, values are either taken from the top (reverse) or bottom (forward) of the buffer. 
>
> Whenever a value is read, it’s re-recorded into the other buffer, and removed from its original place.

To manage the movement of audio between buffers, there are 4 buttons that can be driven by internal clocks or by external gates and triggers, and 2 knobs.

> **Record to Buffer** (rec) Toggles whether or not the module is recording
>
> **Clear Buffers** (clr) Removes all data from both buffers.
>
> **Swap Buffer Data** (swp) Swaps the  data between the two buffers.
>
> **Select Current Buffer** (sel) Toggles which of the two buffers is being read from, and which one is being written to. By default Elastic Twin reads from buf2, and writes to buf1.
>
> **Feedback** controls the amplitude of sound passed between the two buffers.
>
> **Smooth** adds an amplitude window to remove clicking when switching buffers.
>

## QD005 Loam
Loam is an unstable and dirty feedback loop based lowpass with built in non-linearity. It can be used as a filter or pinged for drum sounds. *I actually use this as a drum far more often that I do as a filter. I heavily reccommend it.*

As a filter:
> Cutoff, Feedback, and Radioactivity all influence the filter's frequency. 
>
> **Cutoff** sets a base frequency.
>
> **Feedback** causes the frequency to dip, and the timbre to change.
>
> **Radioactivity** causes frequency to fluctuate erratically and harshly.

As a drum:
> Play a click into loam to make analog drum sounds.
>
> **Cutoff** is the base pitch of the drum sound.
>
> **Resonance** controls the drum's decay. Generally, this should be between 0.85 and 1.
>
> **Feedback** causes a downward sweep from the base pitch, adding a "punchy" characteristic.
>
> **Gain** adds distortion to the drum.
>
> **Radioactivity** makes the drum have a bit of a "crunch."

## QD006 Flick
Flick is a dual module for making impulses for resonators. I made it to be compact and straightforward to use. Flick EXT adds CV inputs for pitch (V/Oct) and level.

## QD007 Graze
Graze is a distortion module that follows input peaks to preserve original amplitude. Graze works by first normalizing input audio, adding noise, clipping, then rescaling audio.
> **Drive** increases amplitude of the audio before clipping and after normalization. Higher drive = fuller distortion.
>
> **Bite** controls the decay of the envelope follower and the overall color of the sound. Higher bite means a brighter sound.
> 
> **Rasp** controls how much noise is added to the signal. This can be tweaked with the rasp mode switch and rasp input. In rasp mode A, noise is added to the envelope, whereas in rasp mode B, noise is added to the sound.
>
> Sending polyphonic signals into the rasp input creates polyphonic outputs.
>
> **Knee** controls the curve of the clipper. Lower knee is softer clipping.

## QD008 Moxie
Moxie is an extremely flexible power starved double VCO/accumulator. It splits signals from the **power** input into positive and negative, then uses them to drive 2 oscillators. 
> **Bias** adds a consistent voltage to the power input.
>
> **Input Linearity** determines the curve of the clipping applied to the power input. Moxie is tuned to be pitch accurate at maximum and minimum input linearity.
>
> **Frequency** determines the base frequency of the oscillator.
>
> **Bite** is a general harshness parameter. In the base model, it's envelope decay. In waveshaping models, it's the amount of shaping applied. Does nothing for clock model.
>
> **Morph** controls the the amplitude of both oscillators through the main output.
>

### Models, Modes and States
This module has 5 different models, 2 modes, and 2 states that shape how it functions, for a total of 20 different configurations.
>**Models**: 
>
>**The base model** (no light) works by accumulating voltage and then releasing the voltage as an envelope pulse. The outputs are the accumulated voltage plus the envelope pulse creating a tri-saw wave by default.
>
>**The waveshaping models** (green, blue, yellow) work by using accumulated voltage as phase for a sine oscillator, and waveshaping it. Blue is a smoother waveshaper, green is a harsher waveshaper, and yellow is a combination of both.
>
>**The clock model** (red) emits a pulse when the accumulator fires. It's tuned to a lower frequency for ease of use.
>
>**Mode**:
>
> In **continuous** mode, static voltages at the power input get repeatedly added to the accumulators. A consistent voltage will produce a tone.
>
> In **slope** mode, *changes* in voltage at the power input are added to the accumulators. A consistent voltage will produce no sound.
>
>**State**:
>
> When moxie is **alive**, both oscillators both fire freely.
>
> When moxie is **dead**, each oscillator waits for the other to fire before it's able to fire again.

## QD009 The Funnel
The Funnel is a note sample & hold/poly utility module. It takes a V/Oct signal through **pitch** input, transposes it by octaves until it's between **min** and **max**, filters out repeated notes, assigns that new note to a polyphony channel. The **output polyphony channels** knob determines how many channels it will output.

With polyphonic inputs, it reduces the notes down to a monophonic sequence, then re-assigns them to fill however many channels specified.

## QD010 Yare
Yare is a simple peak normalizing module. Incoming audio is scaled to an amplitude of +-5v. The **CV** input scales output audio like a VCA.
> **Mode** changes how amplitude is determined. For most purposes, release env should be use. Sample & hold is intended to replicate the sound of certain flawed normalization tools that I found texturally interesting.
>
> **Shape** changes the dynamic character of the module. Negative results in clickier sounds and positive results in smoother sounds. The shape knob can lead to extremely loud outputs if set far from center.
>
> **Low Threshold** sets a threshold below which sound won't be scaled up to 5v.
>
> **Mix** Is the overall dry/wet mix for the effect.
## QD011 Organism


## QD012 Simmer

## QD013 Dead Meat

## QD014 WTHR
