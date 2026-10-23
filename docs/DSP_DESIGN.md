# ToyTune — DSP Architecture & Mathematical Design
**Developer:** Producer Toy  
**Product:** ToyTune Real-Time Vocal Pitch Correction  

---

## 1. Fundamental Frequency Detection (F0)

ToyTune implements a real-time vocal pitch estimation pipeline combining Cumulative Mean Normalized Difference (CMNDF / YIN-style) with sub-sample parabolic interpolation, octave error rejection, and voiced/unvoiced confidence gating.

### Mathematical Formulation
1. **Difference Function:**
   $$d_t(\tau) = \sum_{j=0}^{W-1} \left( x_{t+j} - x_{t+j+\tau} \right)^2$$
   where $W$ is the integration window and $\tau$ is the candidate lag in samples.

2. **Cumulative Mean Normalized Difference Function (CMNDF):**
   $$d'_t(\tau) = \begin{cases} 1 & \text{if } \tau = 0 \\ \frac{d_t(\tau)}{\frac{1}{\tau} \sum_{j=1}^\tau d_t(j)} & \text{for } \tau > 0 \end{cases}$$

3. **Sub-Sample Parabolic Interpolation:**
   Around the local dip minimum $\tau^*$, a parabola is fitted to $\tau^*-1$, $\tau^*$, and $\tau^*+1$:
   $$\Delta \tau = \frac{d'(\tau^*-1) - d'(\tau^*+1)}{2 \left( d'(\tau^*-1) - 2d'(\tau^*) + d'(\tau^*+1) \right)}$$
   $$\tau_{\text{exact}} = \tau^* + \Delta \tau$$
   $$F_0 = \frac{f_s}{\tau_{\text{exact}}}$$

4. **Confidence & Gating:**
   $$\text{Confidence} = 1 - d'_t(\tau^*)$$
   If $\text{Confidence} < 0.55$ or RMS level $< -52\text{ dBFS}$, the frame is marked as unvoiced/silence.

---

## 2. Musical Scale Engine

1. **Fractional MIDI Conversion:**
   $$m = 69 + 12 \log_2\left( \frac{F_0}{A_{\text{ref}}} \right)$$
   where $A_{\text{ref}}$ is adjustable from 415 Hz to 465 Hz (default 440 Hz).

2. **Target Matching:**
   A 12-semitone boolean scale mask $M[k] \in \{0, 1\}$ ($k \in [0, 11]$) defines active scale degrees. The engine evaluates candidate integers $m' \in [m-6, m+6]$ and selects the minimum distance:
   $$m_{\text{target}} = \arg\min_{m' \text{ s.t. } M[m' \bmod 12] = 1} |m' - m|$$

3. **Deviation in Cents:**
   $$\Delta c = 100 \times (m_{\text{target}} - m)$$

---

## 3. Retune Dynamics & Smoothing

1. **Retune Speed Filter:**
   Exponential smoothing time constant $\tau_{\text{retune}}$:
   $$\alpha = 1 - \exp\left( -\frac{\Delta t}{\tau_{\text{retune}}} \right)$$
   $$c_{\text{smooth}}[n] = c_{\text{smooth}}[n-1] + \alpha \left( \Delta c[n] - c_{\text{smooth}}[n-1] \right)$$
   At $\tau_{\text{retune}} = 0\text{ ms}$, $\alpha = 1$, resulting in instantaneous pitch quantization (Hard Tune mode).

2. **Humanize Soft-Knee:**
   A quadratic deadband window preserves micro-vibrato within $\pm 15\text{ cents}$ of the target pitch.

3. **Pitch Transposition Ratio:**
   $$R = 2^{c_{\text{smooth}} \times \text{Strength} / 1200}$$

---

## 4. Real-Time Pitch Shifter

- High-precision dual synchronized read heads scanning a circular delay buffer with 4-point, 3rd-order Hermite fractional sample interpolation:
  $$y[n] = c_3 x^3 + c_2 x^2 + c_1 x + c_0$$
- Hann window crossfading envelopes eliminate discontinuity clicks at pointer wrap-around.
- Stereo phase linking ensures identical pitch modulation on Left and Right channels, preventing stereo image collapse or phase smearing.
