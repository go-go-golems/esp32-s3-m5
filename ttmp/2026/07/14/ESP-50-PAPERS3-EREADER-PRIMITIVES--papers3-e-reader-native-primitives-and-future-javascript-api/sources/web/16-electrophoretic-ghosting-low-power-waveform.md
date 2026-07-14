. 2024 Aug 26;15(9):1076. doi: [10.3390/mi15091076](https://doi.org/10.3390/mi15091076)

[Shanling Lin](https://pubmed.ncbi.nlm.nih.gov/?term=%22Lin%20S%22[Author])
<sup>1,</sup><sup>2</sup>, [Jianhao
Zhang](https://pubmed.ncbi.nlm.nih.gov/?term=%22Zhang%20J%22[Author]) <sup>1,</sup><sup>2</sup>,
[Jia Wei](https://pubmed.ncbi.nlm.nih.gov/?term=%22Wei%20J%22[Author]) <sup>1,</sup><sup>2</sup>,
[Xinxin Xie](https://pubmed.ncbi.nlm.nih.gov/?term=%22Xie%20X%22[Author])
<sup>1,</sup><sup>2</sup>, [Shanhong
Lv](https://pubmed.ncbi.nlm.nih.gov/?term=%22Lv%20S%22[Author]) <sup>1,</sup><sup>2</sup>, [Ting
Mei](https://pubmed.ncbi.nlm.nih.gov/?term=%22Mei%20T%22[Author]) <sup>2,</sup><sup>3</sup>,
[Tingyu Wang](https://pubmed.ncbi.nlm.nih.gov/?term=%22Wang%20T%22[Author])
<sup>1,</sup><sup>2</sup>, [Bipeng
Cai](https://pubmed.ncbi.nlm.nih.gov/?term=%22Cai%20B%22[Author]) <sup>2,</sup><sup>3</sup>,
[Wenjie Mao](https://pubmed.ncbi.nlm.nih.gov/?term=%22Mao%20W%22[Author])
<sup>1,</sup><sup>2</sup>, [Tailiang
Guo](https://pubmed.ncbi.nlm.nih.gov/?term=%22Guo%20T%22[Author])
<sup>1,</sup><sup>2,</sup><sup>3</sup>, [Jianpu
Lin](https://pubmed.ncbi.nlm.nih.gov/?term=%22Lin%20J%22[Author])
<sup>1,</sup><sup>2,</sup><sup>*</sup>, [Zhixian
Lin](https://pubmed.ncbi.nlm.nih.gov/?term=%22Lin%20Z%22[Author])
<sup>1,</sup><sup>2,</sup><sup>3,</sup><sup>*</sup>

Editor: Antonio Ramos

PMCID: PMC11433740 PMID: [39337736](https://pubmed.ncbi.nlm.nih.gov/39337736/)

## Abstract

To address the high power consumption associated with image refresh operations in EPDs, this paper
proposes a low-power driving waveform that reduces the refresh power of EPDs by lowering the
system’s peak power. Compared to traditional waveforms, this waveform first activates the
particles before erasing them, thus reducing voltage polarity changes. Additionally, it introduces
a specific duration of 0 V voltage during the activation phase based on the physical
characteristics of the electrophoretic particles to reduce the voltage span. Finally, a particular
duration of 0 V voltage is introduced during the erasure phase to minimize the voltage span while
ensuring the stability and consistency of the reference gray scale. The experimental results
demonstrate that, in standard power tests, the new driving waveform reduces the power fluctuation
value by 1.33% and the energy fluctuation value by 37.24% compared to the traditional driving
waveform. This reduction in refresh power also mitigates screen flicker and ghosting phenomena.

**Keywords:** electrophoretic electronic paper, refresh power consumption, driving waveforms, low
power consumption, flicker, ghosting

## 1\. Introduction

Electrophoretic electronic paper (EPD) is a display technology that simulates the appearance of
paper. It has advantages such as low power consumption, reflectivity, biostability, etc. \[[^1]\].
EPDs are widely used in various fields, including e-books, e-labels, electronic billboards, and
electronic license plates, providing convenience and comfort in daily life and work. With
continuous technological advancements, EPDs are also evolving towards color displays, video
capabilities, and flexibility \[[^2]\].

The display of EPDs relies on the movement of particles driven by different timing voltages. The
magnitude of these voltages and the duration of the driving time determine the position of the
particles, thus the gray scale of the electronic paper (e-paper) display \[[^3]\]. This timing
voltage composition is known as the driving waveform of EPDs. During the driving process, the size
and duration of the applied voltage in the driving waveform affect the position of the
electrophoretic particles in the microcapsules. The power consumed to drive these particles varies
depending on their positions and the driving waveforms used. Additionally, due to the viscous
nature of electrophoretic particles, considerable time is often required to align the particles to
achieve the desired gray scale, sometimes taking hundreds of milliseconds or even a full second.
The traditional driving waveform is typically divided into three stages: the elimination of the
original image, particle activation, and writing a new image. The elimination stage stabilizes the
EPD screen, usually turning it white or black to reduce residual shadows. The activation phase
increases particle activity by repeatedly driving them between the optical extremes (black and
white), reducing the sticking effect and making it easier to write new data. Finally, the writing
stage drives the e-paper to display new shades of gray.

Due to its inherent bistable characteristics similar to memory behavior, EPDs maintain the image
display for an extended period without power and consume power only when the display needs to be
refreshed or updated. In practice, the power consumption of EPDs arises from the protocols during
data transmission, the switching or refreshing of the TFT source lines, and the driving waveforms
\[[^4]\]. Therefore, optimizing power consumption mainly focuses on these aspects. Li W et al.
\[[^5]\] addressed the high power consumption of EWD displays by analyzing the influence of the
driving waveform and designing a waveform with a rising gradient and a sawtooth pattern to reduce
the power consumption. Qingyun Luo et al. \[[^6]\] tackled the issue of increased power consumption
in e-paper displays due to halftone technology by proposing a multi-objective optimization
technique to simultaneously optimize the image quality and power consumption in color
electrophoretic displays, achieving a Pareto optimum. Pitt et al. \[[^4]\] analyzed the power
consumption caused by capacitive losses due to switching the source lines of TFTs and proposed
reducing the switching voltage to a lower power dissipation, although this approach increased the
response time. JY Kim et al. \[[^7]\] developed a new driving scheme to reduce power consumption by
minimizing the number of driving data lines of TFTs and designing a new TFT panel structure. Cheng
Wei et al. \[[^8]\] proposed a voltage-driven waveform debugging method to reduce e-paper power
consumption by staggering the positive and negative voltage segments of each color-developing
particle at different times, thereby reducing the IC load and average power consumption during
screen refreshes.

In summary, the power consumption of the driving waveform mainly arises from increasing the number
of gray levels or the accuracy, or from particles flipping back and forth between two optical limit
states. To further optimize the driving waveform and reduce the refresh power, this paper
investigates the factors contributing to EPD power consumption, incorporates the characteristics of
traditional driving waveforms, and proposes an optimized waveform that reduces both the refresh
power and the phenomena of flickering and ghosting.

## 2\. Principle

### 2.1. Principle of EPDs

Inside the EPDs, positively and negatively charged particles, along with colorless transparent
organic solvents, are encapsulated in tiny capsules fixed in a transparent adhesive \[[^9]\]. EPDs
display an image by applying an electric field to the e-ink, causing the charged pigment particles
to move through a nonpolar solvent under the influence of the electric field \[[^10]\], as
illustrated in [Figure 1](#micromachines-15-01076-f001). Under the control of an external electric
field, the particles move according to the principle of “homopolar repulsion and anisotropic
attraction” to achieve the image display effect \[[^11]\]. The microcapsules are embedded in a
binder layer (binder 1), with the microcapsule walls separating the electronic ink. Pixel
electrodes adhere to the microcapsule layer through another binder layer (binder 2). These pixel
electrodes are connected to the thin-film transistors of active matrix EPDs, generating an electric
field to drive the particles \[[^12]\].

![Figure
1](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/5e2fe47a821f/micromachines-15-01076-g001.jpg
)

Schematic diagram of the microcapsule EPD structure. At the top is the common electrode, followed
by the microcapsule layer. The microcapsules are suspended in binder 1. The microcapsules contain a
nonpolar solvent in which black and white particles are dispersed. At the bottom is the pixel
electrode, which adheres to the microcapsule layer of binder 2.

### 2.2. Factors That Generate Power Consumption

The power consumption of EPDs primarily originates from two aspects. The first is the refresh
operation, which involves displaying a new image or text on the screen. This process requires the
rearrangement of the electronic ink particles inside the EPD to form a new image or text, thereby
consuming power. The second is the control circuit that drives the EPD display. This circuit is
responsible for generating the electric field, processing image data, performing the refresh
operation, and ensuring the stability of the display content, all of which contribute significantly
to power consumption.

E-paper is bistable, meaning that the particles remain in their current position even without an
electric field, allowing it to consume almost no power when the display is stationary. The refresh
operation of EPDs involves moving the electronic ink particles to display a new image or text,
which consumes electrical energy. Each pixel of an EPD consists of tiny capsules containing charged
particles of various colors, and the speed and path of these particles are influenced by the
strength and direction of the electric field. Therefore, the precise control of the driving
waveform is crucial for the refresh operation.

Different driving waveforms need to be designed according to the photoelectric properties of the
particles and the target gray scale, with each driving waveform representing a specific voltage
timing. Different voltage timings result in varying power consumption, making the design of driving
waveforms a critical factor in the refresh power of EPDs. Poorly designed driving waveforms can
lead to excessive transient currents and increased power consumption.

During the driving waveform design process, a sudden change in voltage can cause rapid charging and
discharging of the capacitor, generating transient current and leading to peak power.

The peak power satisfies the following relationship \[[^13]\]:

| $$ Ppeak=12CΔV2·f·Vsource $$ | (1) |
| --- | --- |

where *C* is the capacitance value, $Δ$ *V* is the magnitude of the voltage variation, *f* is the
switching frequency, and *V <sub>source</sub>* is the supply voltage.

From Equation (1), it can be seen that the magnitude of the voltage change will cause larger peak
power when the other conditions remain unchanged, Therefore, in the design of the driving waveform,
reducing the number of voltage polarity changes and the voltage amplitude can effectively lower the
transient power. The reduction in peak power will reduce the overall refresh power of EPDs.

## 3\. Experiment and Discussion

### 3.1. Experiment Platform

#### 3.1.1. Optoelectronic Performance Test Platform

In this study, a microcapsule e-paper (10.3 inches, with a screen resolution of 1680 × 2240, a
driving voltage of ±15 V, and a driving frequency of 66.67 Hz) manufactured by BOE (Beijing,
China) was used for the experiments. To avoid interference from external environmental factors on
the measurement of EPD reflectivity, a darkroom test system was designed, as shown in [Figure
2](#micromachines-15-01076-f002). A gray scale response time meter, FS-GRT, was placed directly in
front of the EPD under testing. This setup accurately collected the photoelectric change data and
calculated the gray scale response time and flicker data.

![Figure
2](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/30b096117d72/micromachines-15-01076-g002.jpg
)

The EPD optoelectronic performance test platform, measuring the relationship between the EPD’s
reflectivity and response time. ( a ) Physical figure of the optoelectronic performance test
platform; and ( b ) model figure of the optoelectronic performance test platform.

Based on the selection of the EPD’s driving chip, a drive board based on an FPGA was designed and
developed for the EPD display. Additionally, the UNI-T UTP1310 switching regulator power supply was
used to power the light source, providing a stable output voltage and current. This stable power
supply effectively reduces any interference with the test results caused by power supply
fluctuations.

#### 3.1.2. Power Test Platform

In this study, a power test system device was designed, as shown in [Figure
3](#micromachines-15-01076-f003). A power analyzer, EKA1080M (Emkia, Shenzhen, China), was used to
measure the total current of the drive board and EPD over time. These data were then used to
calculate the refresh power of the EPD.

![Figure
3](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/c016eb091fb8/micromachines-15-01076-g003.jpg
)

The EPD’s power consumption test bench for the measurement of the EPD’s current versus time:
(1) computer; (2) EKA1080M; (3) drive board; and (4) EPD.

### 3.2. Optoelectronic Performance Test

The conventional EPD driving process is divided into three stages: erasure, activation, and
display. To determine the appropriate driving time for each stage of the selected EPD, the
photoelectric characteristics were studied. First, the EPD underwent multiple rounds of positive
voltage refresh operations. The effect of different voltage durations on screen reflectivity at +15
V was measured using a gray scale response time meter, with the results shown in [Figure
4](#micromachines-15-01076-f004) a. This step aimed to determine the time required for all black
particles to be effectively driven to the common electrical extremity under different voltage
durations. The initial reflectance was approximately 0.75, decreasing nonlinearly with an increased
positive voltage application time, stabilizing around 0.1 after 90 ms.

![Figure
4](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/bd7e34cca872/micromachines-15-01076-g004.jpg
)

Relationship between voltage and e-paper reflectivity for different driving durations: ( a ) change
in e-paper reflectivity at +15 V for the varying driving duration; and ( b ) change in e-paper
reflectivity at −15 V for the varying driving duration.

Next, the EPD was subjected to multiple rounds of negative voltage refresh operations. A gray scale
response time meter measured the effects of different voltage durations on screen reflectivity at
−15 V, with the results shown in [Figure 4](#micromachines-15-01076-f004) b. This step aimed to
determine the time required for all the white particles to be effectively driven to the common
electrical pole. The initial reflectivity was around 0.1, increasing nonlinearly with an increased
negative voltage application time, stabilizing around 0.7 after 90 ms, though not reaching the
maximum reflectivity of 0.75. Therefore, the drive-to-black and drive-to-white times in the
activation stage of the driving waveform were set to 90 ms, sufficient to shift the EPD’s screen
reflectivity between 0.1 and 0.7.

During the design of the EPD’s driving waveform, single-voltage driving can lead to significant
gray scale loss. To avoid this, a multi-stage voltage combination strategy was used, with
subframe-based driving adopted. Each subframe was 15 ms. When selecting the reference gray scale, a
state with high stability must be chosen, typically white or black \[[^14]\]. Generally, the white
gray scale is used as the reference, and other gray scales are derived from it \[[^15]\]. The
experimental results showed that using white as the reference gray scale yielded a better display
quality than using black. Thus, a negative voltage was used to drive the EPD to display the white
reference gray scale, and a positive voltage was used for the target gray scale adjustment.

In this paper, four gray scales were used to verify the waveform’s validity: 0.7 (white), 0.5
(light gray), 0.3 (dark gray), and 0.1 (black).

### 3.3. Low-Power Driving Waveform Design

A traditional driving waveform structure is shown in [Figure 5](#micromachines-15-01076-f005) a,
where the driving waveform of conventional EPDs generally includes an erase phase, an activation
phase, and a write phase. According to the peak power formula *p* = *C* /2 × *f* × (Δ *V*)
<sup>2</sup> × *V <sub>source</sub>*, it can be found that traditional waveforms have more voltage
reversals, so the peak power consumption is larger. TA1 = TD1 and TB1 = TC1 in [Figure
5](#micromachines-15-01076-f005) a to satisfy the DC balance principle.

![Figure
5](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/ef58196601b8/micromachines-15-01076-g005.jpg
)

Comparison of driving waveform structures: ( a ) conventional driving waveform structure; and ( b )
the low-power driving waveform structure of this paper.

For the low-power driving waveform, it is necessary to reduce the voltage span while ensuring the
DC balance and display quality to avoid damage to the e-paper screen. The low-power driving
waveform structure proposed in this paper is shown in [Figure 5](#micromachines-15-01076-f005) b.
In this structure, the activation voltage is applied first, followed by the erasure voltage. This
approach reduces the peak power by decreasing the number of voltage polarity changes compared to
the traditional waveform. During the activation stage, the black and white particles move at
different speeds under the same voltage due to their distinct characteristics, resulting in
different times to reach their extreme optical states. Therefore, it is unnecessary to keep the
driving times to black and white the same during the activation stage; it is sufficient to drive
both particles to their extreme optical states. The difference between the two is TX, to which is
applied a 0 V voltage, reducing the voltage span and lowering the peak power.

The erasure stage then follows, applying a voltage of TA1 duration to cycle the DC balance and
erase the original image. A 0 V voltage voltage of TY - is then applied to allow the particles to
reach a steady state on their own, forming a stable and consistent reference gray scale. This also
reduces the voltage span and lowers the peak power.

In this paper, four gray levels are used for the waveform verification experiments. The waveforms
for B-B, B-DG, B-LG, B-W, DG-W, LG-W, and W-W of the conventional driving waveforms are shown in
[Figure 6](#micromachines-15-01076-f006) a. The distance between the two dotted lines in the figure
represents a minimum time unit of 15 ms. Based on the test results of the optoelectronic
characteristics, to ensure that the particles are fully activated while avoiding excessively long
response times, each stage’s waveform length is designed to be longer than 75 ms. Thus, the first
stage is a single-DC balancing stage with a length of 90 ms, the second stage is the activation
stage with a length of 180 ms, and the third stage is the new image writing stage with a length of
90 ms.

![Figure
6](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/f03ae0a2a176/micromachines-15-01076-g006.jpg
)

Comparison of driving waveform examples: ( a ) conventional driving waveform example; and ( b )
low-power driving waveform example.

A set of low-power driving waveforms for four gray levels (B-B, B-DG, B-LG, B-W, DG-W, LG-W, and
W-W) is shown in [Figure 6](#micromachines-15-01076-f006) b. The activation stage is performed
first, with a zero-setting voltage duration TX, which is optimized to 15 ms according to the
response time and the movement speed of the black and white particles. Next, the erase stage is
carried out, and the length of the new image writing stage is set to 180 ms. Depending on the
previous gray state and the principle of cyclic DC balance, different lengths of erasing voltage
are applied, with the zeroing voltage duration, TY, optimized to 15 ms to balance the response time
and display effect.

### 3.4. Gray Scale Display Effect Comparison

In this paper, four gray scale driving waveforms are used to compare the effects of a gray scale
display, as shown in [Figure 7](#micromachines-15-01076-f007). [Figure
7](#micromachines-15-01076-f007) a shows the brightness values of four gray scales in cd/m
<sup>2</sup> under traditional waveform driving, while [Figure 7](#micromachines-15-01076-f007) b
shows the brightness values under low-power waveform driving. From the brightness comparison, it
can be seen that under the low-power driving waveform, the EPD display has a higher contrast, and
the luminance differences between the gray scales are more uniform and reasonable. This results in
a better display effect compared to the traditional driving waveform.

![Figure
7](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/40d46371fc92/micromachines-15-01076-g007.jpg
)

Comparison of four gray scale brightnesses: ( a ) four gray scale brightnesses with conventional
driving waveforms; and ( b ) four gray scale brightnesses with low-power driving waveforms.

### 3.5. Power Test

[Figure 8](#micromachines-15-01076-f008) shows the standard images used for e-paper refresh power
tests according to the IEC 62679-3-2:2013 \[[^16]\] and GB/T 43789.32-2024 \[[^17]\] standards.
[Figure 8](#micromachines-15-01076-f008) a displays a checkerboard pattern with 50% coverage, while
[Figure 8](#micromachines-15-01076-f008) b shows its inverted checkerboard pattern. The power
measured during the refresh process of EPDs from pattern a to pattern b is considered the refresh
power of the EPDs; and patterns a and b should have the same contrast.

![Figure
8](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/8d190c0ea3b9/micromachines-15-01076-g008.jpg
)

Standard refresh power test plot: ( a ) checkerboard pattern with 50% coverage; and ( b )
checkerboard pattern inverted from pattern a.

In this paper, we use the EKA1080M power analyzer to measure and calculate the refresh power of the
EPD display, and the relationship between the current and time in the process of refreshing the
image using the driving waveform is obtained by the power analyzer as shown in [Figure
9](#micromachines-15-01076-f009):

![Figure
9](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/5ed044a59a6e/micromachines-15-01076-g009.jpg
)

Current time variation graph during image refresh.

According to the IEC 62679-3-2:2013 and GB/T 43789.32-2024 standards, the e-paper refresh power can
be calculated by Formula (2):

| $$ W=∫0tVIdt $$ | (2) |
| --- | --- |

where *V* represents the voltage; *I* represents the current; and *W* represents the electrical
energy.

Because the driving circuit of the e-paper is continuously providing power during the image refresh
process, in order to better compare the change of power during the image refresh process, according
to the definition of the power fluctuation value of a flat-panel TV in GB 24850-2020 “Flat-panel
TVs and Set-top Boxes Energy Efficiency Limit Values and Energy Efficiency Levels” \[[^18]\], the
power fluctuation value of e-paper is determined by the absolute value of the difference between
the static display power and the refresh power, divided by the static display power, which is *U
<sub>wt</sub>*. The absolute value of the difference between the static display power and refresh
power and the ratio of the static display power is defined as the e-paper power fluctuation value,
symbolized as *U <sub>pt</sub>*, corresponding to the e-paper power fluctuation value of *U
<sub>wt</sub>*. The power fluctuation value can be calculated by Formulas (3) and (4).

The power fluctuation value of the conventional driving waveform is as follows:

| $$ Upt=Pt−PsPs $$ | (3) |
| --- | --- |

The power fluctuation value of the low-power driving waveform is as follows:

| $$ Upl=Pl−PsPs $$ | (4) |
| --- | --- |

The energy fluctuation value of the conventional driving waveform is as follows:

| $$ Uwt=Pt·t1−Ps·t1Ps·t1=Upt $$ | (5) |
| --- | --- |

The energy fluctuation value of the low-power driving waveform is as follows:

| $$ Uwl=Pl·t2−Ps·t1Ps·t1 $$ | (6) |
| --- | --- |

The power fluctuation value reduction rate *R <sub>p</sub>* of the low-power waveform compared to
the conventional waveform can be calculated by Equation (7):

| $$ Rp=Upt−Upl $$ | (7) |
| --- | --- |

The energy fluctuation value reduction rate *R <sub>w</sub>* of the low-power waveforms compared to
the conventional waveforms can be calculated by using Equation (8):

| $$ Rw=Uwt−Uwl $$ | (8) |
| --- | --- |

where *U <sub>pt</sub>* represents the power fluctuation value of the traditional waveform; *U
<sub>pl</sub>* represents the power fluctuation value of the low-power waveform; *U <sub>wt</sub>*
represents the energy fluctuation value of the traditional waveform; *U <sub>wl</sub>* represents
the energy fluctuation value of the low-power waveform; *P <sub>t</sub>* represents the refresh
power of the traditional waveform; *P <sub>l</sub>* represents the refresh power of the low-power
waveform; *P <sub>s</sub>* represents the static display power; *R <sub>p</sub>* represents the
power fluctuation value reduction rate of the low-power waveform compared with the traditional
waveform; *R <sub>w</sub>* represents the energy fluctuation value reduction rate of the low-power
waveform compared with that of the traditional waveform; *t <sub>1</sub>* represents the
traditional driving waveform refresh cycle; and *t <sub>2</sub>* represents the low-power driving
waveform refresh cycle.

In order to avoid experimental chance, this paper performed 10 independent repetitive experiments,
and the experimental measured data after the calculations are as shown in [Table
1](#micromachines-15-01076-t001).

#### Table 1.

Refresh power and static display power values of EPDs with different driving waveforms.

<table><thead><tr><th align="center" rowspan="1" colspan="1"></th><th align="center" rowspan="1"
colspan="1">Refresh Power (W)</th><th align="center" rowspan="1" colspan="1">Static Display Power
(W)</th></tr></thead><tbody><tr><td align="center" rowspan="1" colspan="1">Conventional Driving
Waveforms</td><td align="center" rowspan="1" colspan="1">0.4852</td><td rowspan="2" align="center"
colspan="1">0.4055</td></tr><tr><td align="center" rowspan="1" colspan="1">Low-power Driving
Waveforms</td><td align="center" rowspan="1" colspan="1">0.4798</td></tr></tbody></table>

[Open in a new
tab](https://pmc.ncbi.nlm.nih.gov/articles/PMC11433740/table/micromachines-15-01076-t001/)

[Table 1](#micromachines-15-01076-t001) demonstrates that the refresh power of the EPD is
significantly higher than its static display power. Under the low-power driving waveform, the
refresh power of the EPD is reduced compared to the traditional driving waveform, verifying the
feasibility of this low-power driving approach. After the testing and calculations, we found that
the power fluctuation value of the EPD with the low-power driving waveform is 1.33% lower than that
with the traditional driving waveform, and the overall power consumption is reduced by 37.24%.

Additionally, this paper randomly selected nine pictures with different scene styles to perform the
driving waveform power test, as shown in [Figure 10](#micromachines-15-01076-f010). The refresh
method involved transitioning from a white screen to each picture.

![Figure
10](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/ef51cf801483/micromachines-15-01076-g010.jp
g)

Power consumption test with random black and white pictures.

According to the above standard power test method, in order to avoid chance, 10 independent
repetitions of the experiment were performed for each picture, and the average refresh power data
of each picture was obtained as shown in [Table 2](#micromachines-15-01076-t002):

#### Table 2.

Average power value of 10 refreshes of nine images under different driving waveforms.

<table><thead><tr><th align="center" rowspan="1" colspan="1">Experimental Scenario</th><th
align="center" rowspan="1" colspan="1"><em>P <sub>t</sub></em> (<em>W</em>)</th><th align="center"
rowspan="1" colspan="1"><em>P <sub>l</sub></em> (<em>W</em>)</th><th align="center" rowspan="1"
colspan="1">Optimal Value Δ <em>P</em> (<em>W</em>)</th><th align="center" rowspan="1"
colspan="1"><em>U <sub>pt</sub></em> / <em>U <sub>wt</sub></em> (%)</th><th align="center"
rowspan="1" colspan="1"><em>U <sub>pl</sub></em> (%)</th><th align="center" rowspan="1"
colspan="1"><em>U <sub>wl</sub></em> (%)</th></tr></thead><tbody><tr><td align="center" rowspan="1"
colspan="1">1</td><td align="center" rowspan="1" colspan="1">0.5652</td><td align="center"
rowspan="1" colspan="1">0.5256</td><td align="center" rowspan="1" colspan="1">0.0396</td><td
align="center" rowspan="1" colspan="1">37.38</td><td align="center" rowspan="1"
colspan="1">27.76</td><td align="center" rowspan="1" colspan="1">6.47</td></tr><tr><td
align="center" rowspan="1" colspan="1">2</td><td align="center" rowspan="1"
colspan="1">0.5323</td><td align="center" rowspan="1" colspan="1">0.5065</td><td align="center"
rowspan="1" colspan="1">0.0258</td><td align="center" rowspan="1" colspan="1">29.39</td><td
align="center" rowspan="1" colspan="1">23.12</td><td align="center" rowspan="1"
colspan="1">2.60</td></tr><tr><td align="center" rowspan="1" colspan="1">3</td><td align="center"
rowspan="1" colspan="1">0.4964</td><td align="center" rowspan="1" colspan="1">0.4858</td><td
align="center" rowspan="1" colspan="1">0.0106</td><td align="center" rowspan="1"
colspan="1">20.66</td><td align="center" rowspan="1" colspan="1">18.08</td><td align="center"
rowspan="1" colspan="1">−1.6</td></tr><tr><td align="center" rowspan="1" colspan="1">4</td><td
align="center" rowspan="1" colspan="1">0.5797</td><td align="center" rowspan="1"
colspan="1">0.5333</td><td align="center" rowspan="1" colspan="1">0.0464</td><td align="center"
rowspan="1" colspan="1">40.91</td><td align="center" rowspan="1" colspan="1">29.63</td><td
align="center" rowspan="1" colspan="1">8.03</td></tr><tr><td align="center" rowspan="1"
colspan="1">5</td><td align="center" rowspan="1" colspan="1">0.5308</td><td align="center"
rowspan="1" colspan="1">0.5053</td><td align="center" rowspan="1" colspan="1">0.0255</td><td
align="center" rowspan="1" colspan="1">29.02</td><td align="center" rowspan="1"
colspan="1">22.82</td><td align="center" rowspan="1" colspan="1">2.35</td></tr><tr><td
align="center" rowspan="1" colspan="1">6</td><td align="center" rowspan="1"
colspan="1">0.5068</td><td align="center" rowspan="1" colspan="1">0.4916</td><td align="center"
rowspan="1" colspan="1">0.0152</td><td align="center" rowspan="1" colspan="1">23.19</td><td
align="center" rowspan="1" colspan="1">19.49</td><td align="center" rowspan="1"
colspan="1">−0.42</td></tr><tr><td align="center" rowspan="1" colspan="1">7</td><td
align="center" rowspan="1" colspan="1">0.5689</td><td align="center" rowspan="1"
colspan="1">0.5272</td><td align="center" rowspan="1" colspan="1">0.0417</td><td align="center"
rowspan="1" colspan="1">38.28</td><td align="center" rowspan="1" colspan="1">28.15</td><td
align="center" rowspan="1" colspan="1">6.79</td></tr><tr><td align="center" rowspan="1"
colspan="1">8</td><td align="center" rowspan="1" colspan="1">0.5349</td><td align="center"
rowspan="1" colspan="1">0.5077</td><td align="center" rowspan="1" colspan="1">0.0272</td><td
align="center" rowspan="1" colspan="1">30.02</td><td align="center" rowspan="1"
colspan="1">23.41</td><td align="center" rowspan="1" colspan="1">2.84</td></tr><tr><td
align="center" rowspan="1" colspan="1">9</td><td align="center" rowspan="1"
colspan="1">0.5142</td><td align="center" rowspan="1" colspan="1">0.4963</td><td align="center"
rowspan="1" colspan="1">0.0179</td><td align="center" rowspan="1" colspan="1">24.99</td><td
align="center" rowspan="1" colspan="1">20.64</td><td align="center" rowspan="1"
colspan="1">0.53</td></tr><tr><td align="center" rowspan="1" colspan="1">average</td><td
align="center" rowspan="1" colspan="1">0.5366</td><td align="center" rowspan="1"
colspan="1">0.5088</td><td align="center" rowspan="1" colspan="1">0.0278</td><td align="center"
rowspan="1" colspan="1">30.43</td><td align="center" rowspan="1" colspan="1">23.68</td><td
align="center" rowspan="1" colspan="1">3.06</td></tr></tbody></table>

[Open in a new
tab](https://pmc.ncbi.nlm.nih.gov/articles/PMC11433740/table/micromachines-15-01076-t002/)

The optimized value Δ represents the reduction in refresh power of the EPD under the low-power
driving waveform compared to the traditional driving waveform. As shown in [Table
2](#micromachines-15-01076-t002), while the low-power driving waveform optimizes the refresh power
for all images, the optimization values vary across different images. Therefore, the average
refresh power of the nine images is used for the final comparison in this paper. The negative power
fluctuation value of the low-power driving waveform when refreshing, as seen in [Figure
3](#micromachines-15-01076-f003) and [Figure 6](#micromachines-15-01076-f006), indicates that the
power consumed by refreshing these two images under the low-power driving waveform cycle is less
than the power consumed by statically displaying the images under the traditional driving waveform
cycle. The experimentally measured static display power of white images is 0.4114 W. According to
Equations (3)–(8), the average power fluctuation value of the low-power driving waveform for the
nine images under the low-power driving waveform is 6.75% lower, and the power fluctuation value is
reduced by an average of 27.37% compared to the traditional driving waveform.

### 3.6. Display Effect Test

#### 3.6.1. Flicker Test

[Figure 11](#micromachines-15-01076-f011) and [Figure 12](#micromachines-15-01076-f012) show the
process of image switching of EPDs under conventional waveform driving and low-power driving
waveforms, respectively. Under conventional driving waveforms, the screen flickers four times, and,
under low-power driving waveforms, the flickering of the EPD’s once-refreshed image is reduced to
three times.

![Figure
11](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/e1c4cf0e4524/micromachines-15-01076-g011.jp
g)

Switching process of electrophoretic display under conventional driving waveform.

![Figure
12](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/d27da449eb0d/micromachines-15-01076-g012.jp
g)

Electrophoretic display switching process under the low-power driving waveform of this paper.

#### 3.6.2. Ghosting Test

[Figure 13](#micromachines-15-01076-f013) shows the phenomenon of ghosting in EPDs under the
traditional driving waveform and the low-power driving waveform of this paper. Under the
traditional driving waveform, the ghosting of the screen is seriously retained, which seriously
interferes with the clear display of the subsequent images. Under the low-power driving waveform
proposed in this paper, the ghosting is basically eliminated, which significantly improves the
clarity of the screen display and brings a more comfortable reading experience to users.

![Figure
13](https://cdn.ncbi.nlm.nih.gov/pmc/blobs/a89e/11433740/4692c01f1652/micromachines-15-01076-g013.jp
g)

Ghosting phenomenon: ( a ) refresh from image of a girl to text under conventional driving
waveform; ( b ) refresh from image of a girl to text under low-power driving waveform; ( c )
refresh from checkerboard to white gray scale under conventional driving waveform; and ( d )
refresh from checkerboard to white gray scale under low-power driving waveform.

## 4\. Conclusions

To address the high power consumption issue of electrophoretic electronic paper during an image
refresh, a low-power driving waveform that reduces the refresh power of EPDs by lowering the peak
power of the system was proposed by analyzing the characteristics of EPDs. The final experimental
results show that the low-power driving waveform reduces the power fluctuation value by 1.33% and
the energy fluctuation value by 37.24% compared to the traditional driving waveform. This reduces
the refresh power of the electrophoretic e-paper while also decreasing the screen flicker and
ghosting images.

## Author Contributions

S.L. (Shanling Lin) and J.Z. designed this project. J.Z., X.X. and J.W. wrote the initial draft of
the manuscript. J.Z. and X.X. carried out most of the experiments and data analysis. T.M. and J.W.
performed part of the experiments. S.L. (Shanling Lin) and J.L. revised the paper. T.G., Z.L. and
S.L. (Shanhong Lv) helped with discussions during the manuscript preparation. T.W., B.C. and W.M.
gave suggestions on project management. All authors have read and agreed to the published version
of the manuscript.

## Data Availability Statement

Data are contained within the article.

## Conflicts of Interest

The authors declare no conflicts of interest.

## Funding Statement

This research was funded by the National Key R&D Program of China (No. 2023YFB3609400) and the
National Key R&D Program of China (No. 2022YFB3603705).

## Footnotes

**Disclaimer/Publisher’s Note:** The statements, opinions and data contained in all publications
are solely those of the individual author(s) and contributor(s) and not of MDPI and/or the
editor(s). MDPI and/or the editor(s) disclaim responsibility for any injury to people or property
resulting from any ideas, methods, instructions or products referred to in the content.

## References

## Associated Data

*This section collects any data citations, data availability statements, or supplementary materials
included in this article.*

### Data Availability Statement

Data are contained within the article.

[^1]: 1.Liu X.Q. Research on scientific evaluation system of electronic paper-like paper display.
Inf. Technol. Stand. 2024:84–87+92. doi: 10.3969/j.issn.1671-539X.2024.03.027. (In Chinese)
\[[DOI](https://doi.org/10.3969/j.issn.1671-539X.2024.03.027)\] \[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=Inf.%20Technol.%20Stand.&title=Research%2
0on%20scientific%20evaluation%20system%20of%20electronic%20paper-like%20paper%20display&author=X.Q.%
20Liu&publication_year=2024&pages=84-92&doi=10.3969/j.issn.1671-539X.2024.03.027&)\]

[^2]: 2.Yuan D., Yang T.H., Tang B., Zhou G. Development status and prospect of flexible e-paper
display technology. China Basic Sci. 2023;25:15–25. (In Chinese) \[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=China%20Basic%20Sci.&title=Development%20
status%20and%20prospect%20of%20flexible%20e-paper%20display%20technology&author=D.%20Yuan&author=T.H
.%20Yang&author=B.%20Tang&author=G.%20Zhou&volume=25&publication_year=2023&pages=15-25&)\]

[^3]: 3.Bai P.F., Hayes R.A., Jin M., Shui L., Yi Z., Li W., Zhang X., Zhou G. Review of paper-like
display technologies (invited review) Prog. Electromagn. Res. 2014;147:95–116. doi:
10.2528/PIER13120405. \[[DOI](https://doi.org/10.2528/PIER13120405)\] \[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=Prog.%20Electromagn.%20Res.&title=Review%
20of%20paper-like%20display%20technologies%20\(invited%20review\)&author=P.F.%20Bai&author=R.A.%20Ha
yes&author=M.%20Jin&author=L.%20Shui&author=Z.%20Yi&volume=147&publication_year=2014&pages=95-116&do
i=10.2528/PIER13120405&)\]

[^4]: 4.Pitt M.G., Zehner R.W., Amudson K.R., Gates H. Power consumption of micro-encapsulated
electrophoretic display for smart handheld applications. SID Symp. Dig. Tech. Pap.
2012;33:1378–1381. doi: 10.1889/1.1830204. \[[DOI](https://doi.org/10.1889/1.1830204)\] \[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=SID%20Symp.%20Dig.%20Tech.%20Pap.&title=P
ower%20consumption%20of%20micro-encapsulated%20electrophoretic%20display%20for%20smart%20handheld%20
applications&author=M.G.%20Pitt&author=R.W.%20Zehner&author=K.R.%20Amudson&author=H.%20Gates&volume=
33&publication_year=2012&pages=1378-1381&doi=10.1889/1.1830204&)\]

[^5]: 5.Li W., Wang L., Zhang T., Lai S., Liu L., He W., Zhou G., Yi Z. Driving waveform design
with rising gradient and sawtoothwave of electrowetting displays for ultra-low-power
consumption(Article) Micromachines. 2020;11:145. doi: 10.3390/mi11020145.
\[[DOI](https://doi.org/10.3390/mi11020145)\] \[[PMC free
article](https://pmc.ncbi.nlm.nih.gov/articles/PMC7074629/)\]
\[[PubMed](https://pubmed.ncbi.nlm.nih.gov/32012871/)\] \[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=Micromachines&title=Driving%20waveform%20
design%20with%20rising%20gradient%20and%20sawtoothwave%20of%20electrowetting%20displays%20for%20ultr
a-low-power%20consumption\(Article\)&author=W.%20Li&author=L.%20Wang&author=T.%20Zhang&author=S.%20L
ai&author=L.%20Liu&volume=11&publication_year=2020&pages=145&pmid=32012871&doi=10.3390/mi11020145&)\
]

[^6]: 6.Luo Q.Y. Master’s Thesis. Zhongshan University; Zhongshan, China: 2020. Optimization of
Image and Power Consumption of Color e-Paper Based on Multi-Objective Optimization and Deep
Learning. (In Chinese) \[[Google
Scholar](https://scholar.google.com/scholar_lookup?title=Master%E2%80%99s%20Thesis&author=Q.Y.%20Luo
&publication_year=2020&)\]

[^7]: 7.Kim J.Y., Hwang H.S., Jung H.Y., Park C.W., Souk J.H. Power consumption reductive driving
method for the electrophoretic display. Sid Symp. Dig. Tech. Pap. 2012;39:1843–1845. doi:
10.1889/1.3069541. \[[DOI](https://doi.org/10.1889/1.3069541)\] \[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=Sid%20Symp.%20Dig.%20Tech.%20Pap.&title=P
ower%20consumption%20reductive%20driving%20method%20for%20the%20electrophoretic%20display&author=J.Y
.%20Kim&author=H.S.%20Hwang&author=H.Y.%20Jung&author=C.W.%20Park&author=J.H.%20Souk&volume=39&publi
cation_year=2012&pages=1843-1845&doi=10.1889/1.3069541&)\]

[^8]: 8.Cheng W., Hu Z.P., Xiao X.M. A Voltage-Driven Waveform Debugging Method for Reducing Power
Consumption of Electronic Paper. CN113539191A. Chinese Patent. 2021 October 22; (In Chinese)

[^9]: 9.Yang Y., Wang T.J., Jin Y. Application of polymer materials in microcapsule electrophoretic
display technology. Polym. Mater. Sci. Eng. 2005 doi: 10.3321/j.issn:1000-7555.2005.05.001. (In
Chinese) \[[DOI](https://doi.org/10.3321/j.issn:1000-7555.2005.05.001)\] \[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=Polym.%20Mater.%20Sci.%20Eng.&title=Appli
cation%20of%20polymer%20materials%20in%20microcapsule%20electrophoretic%20display%20technology&autho
r=Y.%20Yang&author=T.J.%20Wang&author=Y.%20Jin&publication_year=2005&doi=10.3321/j.issn:1000-7555.20
05.05.001&)\]

[^10]: 10.Yang B.R., Hu W.J., Zeng Z., Wu Z.Y., Gu Y.F., Xu J.Z., Cao J.X., Zhang Y.D., Chen P.
Understanding the mechanisms of electronic ink operation. J. Soc. Inf. Disp. 2020;29:38–46. doi:
10.1002/jsid.960. \[[DOI](https://doi.org/10.1002/jsid.960)\] \[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=J.%20Soc.%20Inf.%20Disp.&title=Understand
ing%20the%20mechanisms%20of%20electronic%20ink%20operation&author=B.R.%20Yang&author=W.J.%20Hu&autho
r=Z.%20Zeng&author=Z.Y.%20Wu&author=Y.F.%20Gu&volume=29&publication_year=2020&pages=38-46&doi=10.100
2/jsid.960&)\]

[^11]: 11.Wang J., Feng Y.Q., Li X.G., Meng S.X., Xie J.Y., Li G. Microencapsulated electrophoretic
display technology. Chem. Bull. 2005:432–437. (In Chinese) \[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=Chem.%20Bull.&title=Microencapsulated%20e
lectrophoretic%20display%20technology&author=J.%20Wang&author=Y.Q.%20Feng&author=X.G.%20Li&author=S.
X.%20Meng&author=J.Y.%20Xie&publication_year=2005&pages=432-437&)\]

[^12]: 12.Zeng Z., Liu G., Yang M., Yang J., Liu Y., Zou G., Qin Z., Wang X., Deng S., Yang B., et
al. Simulation and analysis of edge ghosting for microcapsule E-Paper based on particles dynamics.
SID Symp. Dig. Tech. Pap. 2022;53:29–32. doi: 10.1002/sdtp.15828.
\[[DOI](https://doi.org/10.1002/sdtp.15828)\] \[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=SID%20Symp.%20Dig.%20Tech.%20Pap.&title=S
imulation%20and%20analysis%20of%20edge%20ghosting%20for%20microcapsule%20E-Paper%20based%20on%20part
icles%20dynamics&author=Z.%20Zeng&author=G.%20Liu&author=M.%20Yang&author=J.%20Yang&author=Y.%20Liu&
volume=53&publication_year=2022&pages=29-32&doi=10.1002/sdtp.15828&)\]

[^13]: 13.Royal Philips Electronics plc. Driving Method for Electrophoretic Display with High Frame
Rate and Low Peak Power Consumption. CN200480025683.0. Chinese Patent. 2006 October 18; (In Chinese)

[^14]: 14.Duan F.B., Bai P.F., Alex H., Zhou G.F. Optimization study of electrophoretic e-paper
driving waveform based on DC balance. Liq. Cryst. Disp. 2016;31:943–948. doi:
10.3788/YJYXS20163110.0943. (In Chinese) \[[DOI](https://doi.org/10.3788/YJYXS20163110.0943)\]
\[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=Liq.%20Cryst.%20Disp.&title=Optimization%
20study%20of%20electrophoretic%20e-paper%20driving%20waveform%20based%20on%20DC%20balance&author=F.B
.%20Duan&author=P.F.%20Bai&author=H.%20Alex&author=G.F.%20Zhou&volume=31&publication_year=2016&pages
=943-948&doi=10.3788/YJYXS20163110.0943&)\]

[^15]: 15.Zhou G.F., Yi Z.C., Wang L., Lu W.X. Current status and prospect of electrophoretic
e-paper driving waveform research. J. South China Norm. Univ. (Nat. Sci. Ed.) 2013;45:56–61. (In
Chinese) \[[Google
Scholar](https://scholar.google.com/scholar_lookup?journal=J.%20South%20China%20Norm.%20Univ.%20\(Na
t.%20Sci.%20Ed.\)&title=Current%20status%20and%20prospect%20of%20electrophoretic%20e-paper%20driving
%20waveform%20research&author=G.F.%20Zhou&author=Z.C.%20Yi&author=L.%20Wang&author=W.X.%20Lu&volume=
45&publication_year=2013&pages=56-61&)\]

[^16]: 16.Electronic Paper Display Part 3-2: Measuring Method Electro-Optical. International
organization-International Electrotechnical Commission; Geneva, Switzerland: 2024. \[[Google
Scholar](https://scholar.google.com/scholar_lookup?title=Electronic%20Paper%20Display%20Part%203-2:%
20Measuring%20Method%20Electro-Optical&publication_year=2024&)\]

[^17]: 17.GB/T 43789.32-2024 \[(accessed on 15 March 2024)\];Electronic Paper Display Devices Part
3-2: Optical Performance Test Methods. State Administration for Market Supervision and Regulation,
National Technical Committee for the Standardization of Electronic Display Devices. 2024 Available
online:
[https://std.samr.gov.cn/gb/search/gbDetailed?id=14156507D2860337E06397BE0A0AE656](https://std.samr.
gov.cn/gb/search/gbDetailed?id=14156507D2860337E06397BE0A0AE656). (In Chinese)

[^18]: 18.GB 24850-2020State Administration of Market Supervision and Administration, National
Standardization Administration: 2020. \[(accessed on 1 August 2021)\];Energy Efficiency Limit
Values and Energy Efficiency Ratings for Flat-panel Televisions and Set-Top Boxes. Available
online:
[https://std.samr.gov.cn/gb/search/gbDetailed?id=AB2CA7A65EFC3FD1E05397BE0A0A98CA](https://std.samr.
gov.cn/gb/search/gbDetailed?id=AB2CA7A65EFC3FD1E05397BE0A0A98CA). (In Chinese)
