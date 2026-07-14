## BRIEF RESEARCH REPORT article

Sec. Optics and Photonics

Volume 9 - 2021 |
[https://doi.org/10.3389/fphy.2021.723106](https://doi.org/10.3389/fphy.2021.723106)

## Driving Waveform Design Based on Driving Process Fusion and Black Reference Gray Scale for
Electrophoretic Displays

- School of Information Engineering, Zhongshan Polytechnic, Zhongshan, China

## Abstract

An electrophoretic display (EPD) is a kind of paper display technology, which has the advantages of
ultra-low power consumption and readability under strong light. However, in an EPD-driving process,
four stages are needed to finish the driving of a pixel erase original images, reset to black
state, clear-to-white state, and write a new image. A white reference gray scale can be obtained
before writing a new image, and this driving process may take too long for the comfort of reading.
In this article, an EPD-driving waveform, which takes the black state as the reference gray, is
proposed to reduce the driving time. In addition, the rules of direct current (DC) balance are also
followed to prevent the charge from getting trapped in the driving backplane. The driving process
is fused and there are two stages in the driving waveform: reset to black state and write the next
image. First, the EPD is written to a stable black state according to the original gray scale
driving waveform and the black state is used as the reference gray for the next image. Second, the
new image is written by the second stage of the new driving waveform. The experimental results show
that the proposed driving waveform has a better performance. Compared with the traditional driving
waveform which has four stages, the driving time of the new driving waveform is reduced by nearly
50%.

## Introduction

EPD has become the most widely used paper-like display due to its good market potential. As a
reflective display technology, the light reflection characteristic provides it with a paper-like
comfortable reading experience \[–\]. In addition, EPDs also have the advantages of ultra-low
power consumption and a good bistable characteristic \[, \]. Therefore, EPDs have been widely used
in electronic labels, e-books, etc. But video playback cannot be achieved due to the long response
time of EPDs \[\]. Particles in microcapsules of EPDs are driven by voltages, and pixels can
display different gray scales by adjusting the position of particles \[–\]. The voltage sequence
which can drive particles to display gray scales is called the driving waveform. Hence, it is of
great significance to reduce the response time and improve the display effect by optimizing the
driving waveform design \[, \].

A traditional driving waveform can be divided into four stages: erasing the original image,
resetting to the black state, clearing to the white state, and writing a new image \[–\]. When a
particle is in a same position for a long time, its mobility is much lower than that of other
particles. The particle activity can be improved by resetting to the black state and clearing to
the white state \[–\]. This driving process may be continued from hundreds of milliseconds to 1 s
\[, \]. Therefore, there are some scholars who have conducted a series of research on the driving
waveform, and some principles and methods of driving waveform editor are put forward to shorten the
driving time or reduce flickers. Kao et al. \[\] studied the property of the suspension viscosity,
characterized the response latency of the EPD, and put forward a new driving waveform, which set
the white gray scale as a reference gray scale, to shorten the driving time and reduce the number
of flickers. However, the proposed driving method did not take into account an important factor-DC
balance. Moreover, the reference gray scale is unstable. Wang et al. \[\] used four kinds of screen
update modes to update the EPD according to different images, and it could improve the update
speed. However, the form of driving waveforms in four modes was not changed, and it could not
improve the refresh speed fundamentally. In a traditional driving waveform, stages of resetting to
the black state and clearing to the white state are longer than the other two stages \[, \], so
some scholars have studied new driving schemes to reduce the driving time of these two stages. Yi
et al. \[\] found that high-frequency voltage can be used to activate particles, and the new
driving waveform could effectively reduce the ghost image and flicker. But it is difficult to get a
stable gray scale display. He et al. \[\] proposed a driving waveform based on the optimization of
particle activation by analyzing the electrophoresis performance of particles. It can effectively
reduce the driving time. However, the DC balance is not considered in the design of the driving
waveform, which may cause damage to the EPD \[\].

In this article, a new driving waveform, which is based on the black reference gray, is proposed to
reduce the driving time. The new driving waveform obeys the rule of DC balance and is divided into
two stages: reset to black state and write the new image. The first stage is used to get an
accurate reference gray, and some principles are proposed to get a uniform reflectance value for
black state at the same time. The second stage is used to write a new gray value by comparing it
with the black reference gray.

## Microencapsulated EPD System

The introduction of microcapsule technology is an important breakthrough in EPD technology. There
are two kinds of particles in a microcapsule \[[^1]\]. Here, the microcapsule is a pixel in EPD. It
solves the long-term display instability problem in EPD. The movement and aggregation of particles
are limited in microcapsules, and the different gray scales could be realized by controlling the
voltage which is applied on electrodes. In the display process, the white SiO <sub>2</sub>
particles of the EPD are charged with a negative charge and black carbon particles are charged with
a positive charge. The structure of an EPD is shown in. In, the electrode of a pixel in the upper
left part is provided with a negative voltage and the positively charged white particles move
upward, the EPD shows white, the lower right part is provided with a positive voltage and the
negatively charged black particles move upward, and the EPD shows black.

FIGURE 1

In the interior of a microcapsule, as is shown in, in order to ensure the good electrophoretic
performance of electrophoretic particles, a charged control agent is injected into the display
capsule, which can effectively prevent the accumulation of charged particles and deposit on the
capsule wall. The display particles in the capsule are mainly affected by gravity, buoyancy,
electric power, and the viscosity of nonpolar solvent. In order to avoid the settlement of display
particles, gravity is equal to buoyancy, as shown in.Here, is the mass of a particle, is the radius
of a particle, is the density of nonpolar solvents, and is the gravity constant. The electric force
can be expressed by.Here, is the electric force, is the charge of the particle, is electric field,
and is the total charge of particles. The first term of is the Coulomb interaction between
particles and electric field, and the other term is the interaction of dielectric polarization
components induced by particles. The viscosity can be expressed by \[[^2], [^3]\].Here, is the
viscosity, is the viscosity coefficient of the nonpolar solvent, and is the velocity of the
particle. can be obtained by solving, and it is shown in \[[^4]\].In general, the charged particle
zeta potential could reach a level which could make a particle whose radius is 1 with 50–100
charges by using the charge control agent, at the same time, the electrophoretic mobility reaches.
The expression of response time is shown as.where is the response time, is the distance between the
common electrode and the pixel electrode, is the voltage applied between two kinds of electrodes,
is the particle zeta potential, and is the dielectric constant of suspension liquid. Generally
speaking, the voltage of the driving waveform could be set as 15, 0, or −15 V, and the EPD may
spend 1 s or so on updating the image.

## Results and Discussion

### Particle Performance Analysis

In the process of driving particles, the EPD may not display a desired gray scale if we drive
particles to realize gray scales directly. In the traditional driving waveform, a four-stage
driving waveform is used to drive particles to realize gray scales accurately \[\]: erase the
original image, reset to black state, clear to white state, and write a new image. In a driving
waveform, a unit of time is 1/50 = 20 if the frame rate is 50 frames/s, so the time of four stages
must be an integer multiple of 20 ms, the second stage and the third stage form a process of
activating particles. Then, an accurate white reference gray scale can be obtained at the end point
of the third stage. The new image can be displayed in the fourth stage by using the reference gray
scale.

In the design process of a driving waveform, the DC balance is a factor which must be considered.
In the traditional driving waveform, the time of positive voltage and negative voltage must equal
in the one gray scale change path to reach the DC balance.

The process of activating particles is very important in the driving waveform. The particles in
microcapsules may be affected by various forces, and the driving force of electric potential must
overcome other external forces to drive the particles. However, the resistance increases with the
increase of speed. The forces that hinder the movement of particles are particle gravity, viscous
resistance of electrophoretic solution, collision between particles, wall collision of
microcapsules, Coulomb force between particles, and so on. When the resistance of particles is
greater than the electric field force, the particle velocity will decrease. Only after a long
enough residence time, the particles can reach a stable state, and the force acting on the
particles can reach equilibrium. This reaction time has a direct impact on the refresh speed of the
display screen. At this time, the motion ability of the moving particles is obviously lower than
that of the activated particles \[\]. Therefore, it is necessary to design a longer voltage
duration to obtain the same gray level as the driving waveform with the active particle stage.
Therefore, at the beginning of the driving waveform, we always use the limited driving time to
activate the particles as much as possible. There are two driving waveforms, which are designed to
drive the EPDs from a black state to a white state, in. The final reflectivity difference of an EPD
which is driven by two driving waveforms are shown in.

FIGURE 2

### Driving Process Fusion in the Driving Waveform

In this article, a new driving waveform, which is based on the black reference gray, is proposed.
In this waveform, the driving process is fused, and there are two stages which include a resetting
to black state stage and writing the new image stage. In the first stage, the particles are
activated as far as possible, and a stable reflectivity black state is obtained at the end of this
stage; then, the black state is taken as the reference gray. In the second stage, the positive
voltage and negative voltage can cooperate with each other to realize the display of multilevel
gray scales. An example of the new driving waveform is shown in.

FIGURE 3

The second stage of the driving waveform, which could drive the EPD to realize the original image,
must be considered in the design of the new driving waveform. For example, is the negative voltage
driving time and is the positive voltage driving time of second stage which could drive the EPD to
display the original image. And is the negative voltage driving time and is the positive voltage
driving time of the first stage which could drive the EPD to display a new image. The relationship
among four driving times is shown in.

The driving waveform could reach the DC balance if is obeyed. In the first stage of the driving
waveform, there is not enough time for refreshing between the white state and the black state, so
the EPD is written to a middle gray scale, and this method could reduce the activation time greatly
\[\]. The detail design of the driving waveform is shown in. For the sake of convenience, B
represents black gray, DG represents dark gray, LG represents light gray, and W represents white
gray, respectively. In, the original gray scale is LG, both and are square waves whose duty ratio
is 50%, so it has no effect on DC balance. is used to realize the DC balance and write to black
state. At the end point of the first stage, a black state which has a stable reflectance value is
obtained. The first stage of driving waveforms should be filled with a positive voltage when the
original gray scale is white, but it cannot get a black gray as the black reference gray. This is
because the black reference gray is obtained in the case of activating particles, but there is no
space for the activated particles stage when this stage is full of positive voltage. So, the first
stage must be prolonged to form a longer time of positive voltage to get the black reference gray.
The prolonged part could occupy the driving time of the second stage, or else, the driving waveform
must be prolonged a little to meet the requirement.

FIGURE 4

### Display Effect of the Proposed Driving Waveform

We carried out a driving experiment on an EPD device according to the proposed driving waveform,
and the result is shown in. The experimental results showed that the driving waveform could realize
multilevel gray scales. Among them, four testing areas are marked, the reflectivity of the first
testing area is 3%, the second testing area is 12.1%, the third testing area is 22.4%, and the
fourth testing area is 36.3%. According to the principle of gamma correction, the human eye
sensitive to brightness transformation when the brightness is low. So, the change of the
reflectivity value is nonlinear.

## Conclusion

In this article, a new driving waveform based on two stages is proposed, and the driving process of
gray scales is effectively fused. The first stage of the driving waveform can erase the original
image, and the second stage is used to realize a multilevel gray scales display. In addition, the
black state is used as the reference gray scale according to the mechanical analysis of particles
in microcapsules. The driving waveform proposed by us has the advantages of short driving time and
less voltage conversion times. It can effectively reduce the flicker sense and improve the reading
comfort while shortening the screen switching time. What is more, the DC balance rule is applied to
the design, which can effectively prevent the display breakdown caused by charge trapping. So, the
proposed driving waveform provides a reference for the continuous improvement of the EPD display
quality.

## Statements

### Data availability statement

The original contributions presented in the study are included in the article/Supplementary
Material; further inquiries can be directed to the corresponding authors.

### Author contributions

LW designed this project. PM and JZ carried out most of the experiments and data analysis. QW
performed part of the experiments and helped with discussions during manuscript preparation. All
authors have read and agreed to the published version of the manuscript.

### Funding

This research was funded by the Key Projects of the Second Batch of Social Welfare and Basic
Research in Zhongshan City (No. 2020B2021), the High-level Scientific Research Startup Project in
Zhongshan Polytechnic (No. KYG2104), and the Special Projects in Key Fields of Colleges and
Universities in Guangdong Province in 2020 (No. 2020ZDZX3083).

### Conflict of interest

The authors declare that the research was conducted in the absence of any commercial or financial
relationships that could be construed as a potential conflict of interest.

### Publisher’s note

All claims expressed in this article are solely those of the authors and do not necessarily
represent those of their affiliated organizations, or those of the publisher, the editors and the
reviewers. Any product that may be evaluated in this article, or claim that may be made by its
manufacturer, is not guaranteed or endorsed by the publisher.

## References

## Summary

Keywords

electrophoretic display, driving waveform, driving process, reference gray scale, direct current
balance

Citation

Wang L, Ma P, Zhang J and Wan Q (2021) Driving Waveform Design Based on Driving Process Fusion and
Black Reference Gray Scale for Electrophoretic Displays. *Front. Phys.* 9:723106. doi:
[10.3389/fphy.2021.723106](http://dx.doi.org/10.3389/fphy.2021.723106)

Received

10 June 2021

Accepted

05 July 2021

Published

09 August 2021

Volume

9 - 2021

Edited by

[Qiang Xu](https://loop.frontiersin.org/people/937289/overview), Nanyang Technological University,
Singapore

Reviewed by

[Weimin Li](https://loop.frontiersin.org/people/1369533/overview), Shenzhen Institutes of Advanced
Technology, CAS, China

[Jingling Yan](https://loop.frontiersin.org/people/1370550/overview), Ningbo Institute of Materials
Technology and Engineering, CAS, China

Updates

Copyright

[^1]: 28.

BertTDe SmetH. Dielectrophoresis in Electronic Paper. *Displays* (2003) 24(4-5):223–30.
10.1016/j.displa.2004.01.009

- [CrossRef](https://doi.org/10.1016/j.displa.2004.01.009)
- [Google
Scholar](http://scholar.google.com/scholar_lookup?author=T.%2BBert&author=H.%2BDe%2BSmet&publication
_year=2003&title=Dielectrophoresis%2Bin%2BElectronic%2BPaper&journal=Displays&volume=24&pages=223-30
)

[^2]: 29.

KangH-LKimCALeeS-IShinY-KLeeY-HKimY-C. Analysis of Particle Movement by Dielectrophoretic Force for
Reflective Electronic Display. *J Display Technol* (2016) 12(7):747–52. 10.1109/JDT.2016.2524023

- [CrossRef](https://doi.org/10.1109/JDT.2016.2524023)
- [Google
Scholar](http://scholar.google.com/scholar_lookup?author=H-L.%2BKang&author=CA.%2BKim&author=S-I.%2B
Lee&author=Y-K.%2BShin&author=Y-H.%2BLee&author=Y-C.%2BKim&publication_year=2016&title=Analysis%2Bof
%2BParticle%2BMovement%2Bby%2BDielectrophoretic%2BForce%2Bfor%2BReflective%2BElectronic%2BDisplay&jo
urnal=J+Display+Technol&volume=12&pages=747-52)

[^3]: 30.

CrowderTMRosatiJASchroeterJDHickeyAJMartonenTB. Fundamental Effects of Particle Morphology on
LungDelivery: Predictions of Stokes’Law and the Particular Relevance to Dry Powder Inhaler
Formulation and Development. *Pharm Res* (2002) 19(3):239–45. 10.1023/A:1014426530935

- [CrossRef](https://doi.org/10.1023/A:1014426530935)
- [Google
Scholar](http://scholar.google.com/scholar_lookup?author=TM.%2BCrowder&author=JA.%2BRosati&author=JD
.%2BSchroeter&author=AJ.%2BHickey&author=TB.%2BMartonen&publication_year=2002&title=Fundamental%2BEf
fects%2Bof%2BParticle%2BMorphology%2Bon%2BLungDelivery%3A%2BPredictions%2Bof%2BStokes%E2%80%99Law%2B
and%2Bthe%2BParticular%2BRelevance%2Bto%2BDry%2BPowder%2BInhaler%2BFormulation%2Band%2BDevelopment&j
ournal=Pharm+Res&volume=19&pages=239-45)

[^4]: 31.

HeWYiZShenSHuangZLiuLZhangT. Driving Waveform Design of Electrophoretic Display Based on Optimized
Particle Activation for a Rapid Response Speed. *Micromachines* (2020) 11(5):498. 10.3390/mi11050498

- [CrossRef](https://doi.org/10.3390/mi11050498)
- [Google
Scholar](http://scholar.google.com/scholar_lookup?author=W.%2BHe&author=Z.%2BYi&author=S.%2BShen&aut
hor=Z.%2BHuang&author=L.%2BLiu&author=T.%2BZhang&publication_year=2020&title=Driving%2BWaveform%2BDe
sign%2Bof%2BElectrophoretic%2BDisplay%2BBased%2Bon%2BOptimized%2BParticle%2BActivation%2Bfor%2Ba%2BR
apid%2BResponse%2BSpeed&journal=Micromachines&volume=11)
