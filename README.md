# C.R.A.B. — Controlled Regulable Agitation Box

Automated mechanical stimulation system developed for sleep-deprivation experiments in the crab *Neohelice granulata*.

This project was developed by **Anabella De Bortoli** and **Javier García Cueto** as part of Laboratorio 6 (Physics Department, FCEyN, Universidad de Buenos Aires), under the supervision of Julieta Sztarker and Matías Alejandro Gultig. The experimental work was carried out at **IFIBYNE (CONICET-UBA)** in 2025.

## Project overview

C.R.A.B. is an automated rocking platform designed to generate programmable mechanical stimulation patterns. Its purpose is to keep *Neohelice granulata* crabs from entering prolonged periods of inactivity while minimizing excessive stress and habituation to the stimulus.

The system integrates:
- Mechanical design and construction of an adjustable rocking platform.
- Electronic motor control based on an Arduino Uno.
- Embedded programming for configurable and randomized stimulation protocols.
- Experimental testing with behavioral video recordings.
- Python-based processing, classification and visualization of behavioral responses.

## Embedded control

The Arduino program implements a state-machine architecture controlling movement, pauses, longer rest intervals, manual operation and shutdown.

Main features include:
- PWM motor-speed control.
- Randomized movement, pause and rest durations within configurable ranges.
- Serial interface for changing experimental parameters.
- EEPROM storage of configuration parameters.
- Button debouncing and manual-control mode.
- Maximum experiment duration control and watchdog-based fault handling.

The controller source is available in [`arduino/crab_controller.ino`](arduino/crab_controller.ino).

## Experimental analysis

C.R.A.B. was tested with groups of *Neohelice granulata* during periods of low spontaneous activity. Behavioral responses were recorded on video and classified according to locomotor response.

The analyses included comparisons between responses at the beginning of the experiment, after one hour of stimulation and near the end of experiments lasting up to six hours. Event/raster-style visualizations were used to represent responses to successive stimuli.

The corresponding Python notebooks are available in the [`analysis/`](analysis/) directory.

## Results

During the tested stimulation protocol, the animals continued to show locomotor responses for experiments lasting up to six hours. In the analyzed windows, no crab remained unresponsive for a period long enough to be associated with sleep under the criterion used in the study.

These results supported the use of C.R.A.B. as a programmable experimental device for controlled sleep-deprivation protocols in *Neohelice granulata*.

## Repository structure

```text
crab-experimental-system/
├── README.md
├── arduino/
│   └── crab_controller.ino
├── analysis/
│   ├── socializados.ipynb
│   ├── aislados.ipynb
│   └── rasters.ipynb
├── docs/
│   ├── report.pdf
│   └── user_manual.docx
└── figures/
```

## Technologies

**Embedded systems:** Arduino Uno · C/C++ · PWM · EEPROM · serial communication  
**Data analysis:** Python · Jupyter Notebook  
**Experimental work:** behavioral video analysis · data classification · scientific visualization  
**Design and prototyping:** electronics · mechanical design · 3D-printed components

## Authors

**Anabella De Bortoli**  
**Javier García Cueto** — [GitHub](https://github.com/javiergarciacueto)

## Academic context

Laboratorio 6 — Physics Department, FCEyN, Universidad de Buenos Aires  
IFIBYNE — CONICET / Universidad de Buenos Aires  
2025
