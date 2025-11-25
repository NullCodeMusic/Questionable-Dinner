# Questionable Dinner Manual
This is a shared manual for every questionable dinner module. 
## QD001 A Saw B
A Saw B Is a utility module that lets you Compare, split, subtract and mangle two inputs. The panel is divided into 3 semi-independent units and and has 1 parameter. From top-to-bottom, the 3 units are Logic, Split, and Minus.
### Inputs
There are 3 sets of A and B inputs. For the Split and Minus sections, inputs are normalled to the A and B inputs above.
### Bias
The bias parameter offsets the value of B for all of this module’s calculations. The polarity (positive or negative) of the offset is contextual and changes to push values away from center.
### The Logic Unit
The logic unit outputs 0v or 10v based on four comparative statements. For this unit, bias widens the window in which inputs are considered equal.
### The Split Unit
The split unit uses B to split A into an over and an under value. 
The over value is how much the A value exceeds B. If A is less than B, the value is 0.
The under value is how much the A value falls below B. If A is greater than B, the value is 0.
Both values are positive.
### The Minus Unit
The minus unit simply subtracts B from A
### Wave Morphing
By patching A Saw B into itself, you can morph audio inputs.
You can get a simple wave morphing algorithm by 
[1] patching one oscillator or audio source into Split B and (for a cleaner sound, after a one sample delay) Minus A 
[2] patching a second oscillator or audio source into Split A 
[3] patching Over into Minus A and Undr into Minus B
## QD002 Pilfer
Pilfer is a module that uses a moving particle to try and approximate input audio. It takes inputs (x/r, y/θ)as coordinates and moves a particle towards those coordinates. It sounds like a messier, less stable resonant lowpass filter.
### 1D vs 2D mode
Pilfer has 2 algorithms, one that works in one dimensional space, and another that works in two dimensions. Depending on which of the two inputs are connected, the module selects one. You can see which algorithm is enabled by looking at the blue (1D) and yellow (2D) lights at the bottom.
### Parameters
*Acceleration* (a) is how fast the particle picks up speed and switches directions. This is similar to a cutoff frequency, but not the same.
*Friction* (μ) is how quickly the particle loses speed. Friction is similar to resonance, the lower the friction, the more resonant the sound. When friction is set to 1, the module acts like a slew limiter.
*Trigger Distance* (dist) is how far the particle must be from the input position to start moving. 
*Drive* (drive) increases input gain and decreases output gain.
*Bounce* (bounce) is how much the particle bounces & conserves its speed when it hits against the bounds (±10). At higher bounce values, the system becomes very unstable.
*Coordinates* (xy | rθ) switches between using polar or rectangular coordinates when pilfer is in 2D. 
*Friction* (lin | exp) changes whether friction acts linearly (subtracts) or exponentially (divides) on the particle’s current velocity.
*Range* (hi | lo) changes the range for the acceleration parameter. Lo is better suited for processing cv, and Hi is better for processing audio.
*Oversampling* (*) controls the number of times this module iterates per sample. At 16x, the sound is significantly less harsh and less prone to instability. At 1x, the sound is very volatile.
### Shake & Velocity
The shake input directly adds its value to the velocity of the particle.
The v output is the absolute value of the particle’s current velocity (after applying friction)
### The Red Lights
There are two red lights at the top of the module. The top light’s brightness is determined by a particle’s velocity and the bottom light pulses when the particle bounces. Generally, the brighter both lights are, the less stable the sound.
## QD003 Surgeon
Surgeon is a text based and fully programmable additive oscillator. It controls up to 64 sine wave partials using typed-in math expressions.
### Expressions
There are 9 total expressions that can be programmed into this oscillator. All of these are calculated on a per-partial basis (variables i and n are used to get the index of the current partial, and the number of total partials). The header changes to tell you what expression you currently have hovered. From top to bottom, the expressions are:
*Variable* `j` - A variable that can be referenced in expressions.
*Variable* `k` - A variable that can be referenced in expressions.
*Variable* `l` - A variable that can be referenced in expressions.
*Time Warp* - An offset to the envelope, phase, and the time variable, `t`. 
*Env Attack* - The number of seconds it takes for the envelope to rise.
*Env Decay* - The number of seconds it takes for the envelope to fall.
*Pitch* - The pitch distribution of partials.
*Amp* - The amplitude distribution of partials.
*Phase* - The phase distribution of partials. When the expression is set to 0, phase is not reset on trig.
Note: These expressions are calculated in order from top to bottom, per-partial. If you reference them out of order, calculations will no longer line up with the correct current partial index, `i`. Sometimes this can be fixed by substituting `i` with (`i-1`) or something else to compensate for the shift but this might not always work.
### Variables & Constants
`i` - The index of the current partial, starting at 1
`n` - The total number of partials
`r` - Sample and hold random per partial. 
`t` - Time since last trig
`f` - Base frequency in hz, defined by v/Oct input
`e` - Envelope
`out` - Previous oscillator output
`x`, `y`, and `z` - Param/CV in 
`j`, `k`, and `l` - User defined variables
`pi`
### Trig
The trig input serves both as a signal to tell the oscillator to compile the typed math expressions and the start of a note.
### Freeze
The freeze button stops all changes to pitch, amp and phase.
### Rand Seed
This control changes the seed for the per partial random value, r. When set to anything other than 0, the random values are deterministic and will be the same on every trig. You can get new random numbers by changing the seed. When set to 0, the random values will be different every trig.
### X,Y,Z
While these inputs are unplugged, the knobs function as parameters. While plugged in, they function as attenuators.
### Quality
By default, parials are calculated once every few samples instead of at audio rate and then interpolated to create smooth changes. The quality option in the right click menu determines how fast partials are calculated. Higher quality greatly impacts CPU usage (64 partials on ultra uses 75% of my CPU) and is usually unnecessary, but there are niche uses (see phase mod preset).
*Low* recaclculates every 128 samples.
*Standard* recalculates every 64 samples.
*High* recalculates every 24 samples.
*Very High* recalculates every 8 samples.
*Ultra* recaclculates every other sample.
## QD004 Elastic Twin
Elastic twin is a delay/looper plugin that records to and swaps between two buffers to cut apart and rearrange audio. With no set buffer size, no set delay time, this module is great at making garbled, beautiful messes out of input audio.
## The Buffers
The plugin’s 2 buffers (more specifically deques) work as following:
- When writing to a buffer, values are added to the top of the buffer. 
- When reading a buffer, values are either taken from the top (reverse) or bottom (forward) of the buffer. 
- Whenever a value is read, it’s re-recorded into the other buffer, and removed from its original place.
## Buffer Controls
Record to Buffer (rec) Toggles whether or not the module is recording
Clear Buffers (clr) Removes all data from both buffers.
Swap Buffer Data (swp) Swaps the  data between the two buffers.
Select Current Buffer (sel) Toggles which of the two buffers is being read from, and which one is being written to. By default Elastic Twin reads from buf2, and writes to buf1.
## Clocks and Extermal Overrides
The module has 2 clocks that can be synced to eachother and can be used to control each of the 4 buffer controls. Each control also has an override input that overrides any internal clock with an external clock, gate, or trig. For toggles any clock or gate input will invert the toggle.
## Other Params
*Feedback* (feed) controls the volume of the audio being re-recorded into the other buffer.
*Smooth* (smth) Makes audio less harsh by tapering out the volume of recordings before and after a buffer control is triggered or toggled. The control value is how many milliseconds the transition takes.