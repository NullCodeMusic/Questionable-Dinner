# QUESTIONABLE DINNER USER MANUAL
This manual is a quick reference/summary of each of my modules. I prioritized brevity when writing it, so parameters, inputs, and outputs that I think are explained by their names are omitted.
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

*Note: the expander goes on the left of the main module.*

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
>
> **Bass Split** sends low frequencies through without getting distorted.

## QD008 Moxie
Moxie is an extremely flexible power starved double VCO/accumulator. It splits signals from the **power** input into positive and negative, then uses them to drive 2 oscillators. 
> **Bias** adds a consistent voltage to the power input.
>
> **Input Linearity** determines the curve of the clipping applied to the power input. Moxie is tuned to be pitch accurate at maximum and minimum input linearity.
>
> **Frequency** determines the base frequency of the oscillator.
>
> **Bite** is a general shape parameter. To the right is generally brighter or harsher shapes.
>
> **Morph** controls the the amplitude of both oscillators through the main output.
>

### Models, Modes and States
This module has 5 different models, 2 modes, and 2 states that shape how it functions, for a total of 20 different configurations.
>**Models**:
>
>*Current model is indicated by light color*
>
>**Light Off: The base model** works by accumulating voltage and then releasing the voltage as an envelope pulse. The outputs are the accumulated voltage plus the envelope pulse creating a tri-saw wave by default. The **bite** parameter controls the envelope's decay.
>
>**Blue Light: The square model** creates a square wave based on the current state of the accumulators. The **bite** parameter controls the pulse width.
>
>**Green Light: The sine warp model** creates a warped sine wave based on the current state of the accumulators. The **bite** parameter works similar to pulse width, and bends the sine into a tighter shape.
>
>>**Yello Light: The sine pulse model** creates a short percussive sine wave pulse when the accumulators fire. The **bite** parameter controls the speed of the pulse.
>
>**Red Light: The clock model** (red) emits a trigger when the accumulators fire. It's tuned to a lower frequency for ease of use. The **bite** parameter does nothing.
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
Organism is a additive synth macro oscillator/synth voice. The **gate** input accepts variable voltages, so it can also function like a velocity control. Modulator inputs are normalled. **FM** is normalled to an internal sine voice, other mod inputs are normalled to the envelope.

I recommend just playing by ear for sound designing with this module.

*Note: Recommended use with a tuner module!*
> **Partials** controls the maximum number of partials used by the synth voice.
> 
> **Specimen** is the current additive oscillator algorithm in use.
>
> **Structure** and **Morph** are the current specimen's parameters.
>
> **Attack**, **Decay**, and **Sustain** shape the internal envelope. **LPG Curve** applies a tighter envelope curve to higher numbered (usually meaning higher pitched) partials.
### Specimens
Basic [`Base`]:
> A basic saw-square patch. 
> 
> **Structure** controls the distribution of partials.
>
> **Morph** fades amplitude between even and odd numbered partials.

Organ [`Orgn`]:
> Exponential-series stack of partials.
> 
> **Structure** scales the overall pitch distribution.
>
> **Morph** repitches even numbered partials.

Low Quality [`LowQ`]:
> Saw with phase and pitch distortion.
> 
> **Structure** quantizes pitches of partials to increasingly large intervals.
>
> **Morph** applies bitcrush-adjacent phase artifacting.

Fish [`Fish`]:
> Packing partials within a narrow pitch range.
> 
> **Structure** controls the range.
>
> **Morph** disperses pitches somewhat.

Combed Saw [`Comb`]:
> A saw with applied comb filtering.
> 
> **Structure** comb width.
>
> **Morph** positive/negative comb amount.

Combed Saw [`Metal`]:
> A bunch of partials everywhere. Vaguely based on cymbal sample frequencies. Vaguely.
> 
> **Structure** controls distribution of partials.
>
> **Morph** puts an emphasis on certain partials.

Combed Saw [`Metal`]:
> A bunch of partials everywhere. Vaguely based on cymbal sample frequencies. Vaguely.
> 
> **Structure** controls distribution of partials.
>
> **Morph** amp emphasis on certain partials.

Keys [`Keys`]:
> Satisfying lookup-tables for an electric piano sound.
> 
> **Structure** blends partial pitches between 2 preset tables.
>
> **Morph** repitches even numbered partials.

Fractal-ish [`Frct`]:
> Groups partials into 4, then scales pitches. Baseline of saw.
> 
> **Structure** determines distances between groups.
>
> **Morph** determines pitches within each group.

Particles [`;*.:`]:
> Saw but with a cascading repeating envelope/lfo amp window on each partial.
> 
> **Structure** determines envelope speed and direction.
>
> **Morph** adds a pitch effect to the envelopes.

Prism [`Prsm`]:
> Power series stack of partials.
> 
> **Structure** controls power scaling of partials.
>
> **Morph** controls linear scaling of partials.

Chaotic [`Dice`]:
> Randomly detuned partials.
> 
> **Structure** determines the random generation seed.
>
> **Morph** controls how much the random detuning is applied.

Noise [`&#$%`]:
> Constantly changind random detuned partials. Wider random pitch range than Chaotic.
> 
> **Structure** determines the speed of pitch re-randomization.
>
> **Morph** controls how much the random detuning is applied.

Phase Mod [`PMod`]:
> Phase modulation.
> 
> **Structure** controls the intensity of phase modulation.
>
> **Morph** disperses base phases of partials.

Per-Partial AM [`PMod`]:
> Applies amplitude modulation between 2 additive oscillators per-partial.
> 
> **Structure** controls oscillator 1 pitch.
>
> **Morph** controls oscillator 2 pitch.

### Domesticate
Domesticate is an expander that applies a simple filter/blend functions to organism. It also includes an envelope out from organism's internal envelope.
> **Filter Type** (Lowpass, Highpass, Bandpass, Notch) remove partials from organism's base specimen, and replace them with the distribution specified by **Wave**.
>
> **Lock Fundamental** makes the first harmonic always align with the pitch determined by V/Oct, exempting it from the current specimen.
>
> **Center** controls which partial the filter is centered on.
>
> **Width** controls the width of the filter slope.
>
> **Base** applies a static blend to all harmonics.
>
> **Amount** scales the "amplitude" of the filter.
>
> **Dither** filters every #th partial. Negative values invert it.
## QD012 Simmer
Simmer is a weird arp/gate-to-melody module. It has a number of gate inputs separated by pitch intervals. The length of each pitch interval is determined by the **Scale** param. This works microtonally, so maybe use a quantizer. **Range** determines the maximum range in +-volts from 0 before pitch is either wrapped, or the module's state is reset (**Edge Behavior**).

## QD013 Dead Meat
Dead meat is a phase distortion/harshness module. It rapidly flips between a dry signal and an allpassed signal by comparing the two.
> **Freq** is the frequency of the allpass filter.
>
> **Bite** determines the harshness of the flip between allpassed and dry signals.
>
> **Bite x Cutoff** makes bite follow cutoff frequency, creating a more consistent filter-like character.
> 
> **Resonance** applies a feedback loop to the module.
>
> **Bias** applies a voltage bias to separate the two signals, creating sparser flips (different texture).

## QD014 WTHR
WTHR is a VCO/LFO meant for creating textured noise of various harshnesses and pitches. It applies randomization to a phasor, then applies a function based on the phasor. It has 4 different outputs for 4 different functions.
> **Randomness** determines overall variation around the base pitch. At 0%, **Cloud** is silent, and the other 3 outputs are a consistent oscillator.
>
> **Length** scales the length of each pulse for **Rain**, **Clock**, and **Bubble** outs.
>
> **Unison**, **Spread**, and **Outputs (Mono/Poly)** layer each noise to create a deeper texture or to output multiple different noise oscillators without having to place more copies of WTHR.

## QD015 Boxes
Boxes is a delay/reverb based on distances between points and the walls of a box. This is mainly a method of dispersing delay times and not meant for realism. 

You can **Send** and **Recieve** to add effects to its feedback chain. 

> **The Screen** lets you drag around points for the input and two outputs.
>
> **Delay** is a multiplier for the calculated delay times, and is not directly implemented as a delay time.
> 
> **Width** and **Height** determine the size of the box.
>
> **Feedback** takes a second reflection off of the walls, and feeds it back into the input. **Highpass**, **Lowpass** and **Diffuse** are all applied to the feedback chain.
>
> **Pull** makes it so that direct audio to A and B outs happens immediately at the closest output, and scales down the delay at the other by that same amount, separating them from the rest of the delay signal.