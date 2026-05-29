# Drone Engineering Portfolio

This repository documents my hands-on journey in drone engineering, embedded systems, hardware testing, and iterative flight-controller development.

It serves as:
- a public record of my engineering progress,
- a technical portfolio for employers, collaborators, and reviewers,
- and a living repository for my university graduation project.

> This repository is still under active development. I will continue improving the documentation, adding new tests, updating prototypes, and refining the work over time.

## Engineering Philosophy

My motivation comes from practical engineering more than purely academic theory.

I believe real engineering is best understood through direct physical work: soldering, wiring, testing, debugging, redesigning, and building systems step by step. The goal of this repository is not only to use existing hardware, but to understand drone systems deeply by building, testing, and improving them from the ground up.

This repository is a record of that process.

## Shared Hardware Baseline

Several project stages in this repository were built and tested using a common hardware baseline:

- **Frame:** F450 quadcopter frame, chosen for strength and impact resistance during early flight testing.
- **Motors:** DJI 2212 920KV motors.
- **ESCs:** 30A ESCs in earlier stages, with some later upgrades to Hobbywing 40A V2 for improved current stability.
- **RC System:** HotRC transmitter/receiver system using the digital **SBUS** protocol for reduced wiring and faster signal handling.
- **Sensors:** Different sensors were tested across the project, including **MPU-6050**, **BNO055**, and **GPS 7M / 7N** modules.
- **Power System:** 3S 4200mAh LiPo battery.

## Repository Projects

This repository is intentionally organized as separate projects and experiments. Each folder represents a different stage, prototype, or engineering direction.

### 1. Arduino System
**Folder:** `DroneProject-Arduino Uno`

This was the first practical stage of the project.

It focused on learning the basics of quadcopter control, PWM motor output, embedded logic, and early flight-system behavior on an 8-bit platform.

#### Main challenge
The Arduino struggled to generate synchronized PWM signals for all four motors with the timing precision required for stable flight.

#### Engineering reason
The board’s internal timers created conflicts between motor outputs, leading to inconsistent control timing.

#### Result
This caused major latency in balance correction, unstable response, and damage to some of the first ESC units.

#### Project type
- Early flight-controller prototype
- Embedded systems experiment
- Learning and validation stage

---

### 2. ESP32-S3 System
**Folder:** `DroneProject2-esp32-s3`

This phase represented the move toward a more capable and modern embedded platform.

The ESP32-S3 was used to improve control performance, communication flexibility, and software architecture.

#### Key developments
- Solved UART / USB communication conflicts.
- Transitioned to **SBUS** for cleaner wiring and lower latency.
- Replaced the unreliable BNO055 unit with **MPU-6050** for faster raw sensor response.
- Improved software structure and overall stabilization logic.

#### Wireless interface work
A browser-based Web UI was built on top of the ESP32 embedded web server to:
- display telemetry data,
- visualize pitch and roll behavior,
- and allow wireless PID tuning directly from the browser.

#### Role of AI
AI was used as a development assistant for code optimization, troubleshooting timer and library conflicts, and supporting web-interface development.

#### Outcome
This stage led to the first successful stable takeoff.

#### Project type
- Flight-controller prototype
- Embedded software development
- Web telemetry and tuning interface
- AI-assisted development stage

---

### 3. ESP32-S3 Continued Development
**Folder:** `DroneProject3- esp32s3`

This folder represents a more advanced continuation of the ESP32-based drone work.

It includes further stabilization work, testing, PID tuning refinement, and practical development based on the earlier ESP32 phase.

It also includes external references and tutorial-based learning integrated into the practical engineering workflow.

#### Project type
- Improved flight-controller prototype
- Testing and stabilization stage
- Iterative refinement project

---

### 4. STM32-WB Project
**Folder:** `DroneProject-STM32-WB`

This project represents STM32-based development work related to flight systems, embedded logic, and interface-level experimentation.

It currently includes implementation and interface-related work that supports the wider progression of the drone platform.

#### Project type
- Embedded systems experiment
- STM32-based control development
- Supporting project stage

---

### 5. STM32F103 Prototype
**Related stage:** custom PCB prototype development

One major stage in this repository involved designing and building a custom PCB around **STM32F103**.

This marked the transition from development boards to a more professional custom flight-controller design.

#### Main work completed
- Designed a custom PCB using EasyEDA.
- Printed and assembled the board.
- Adapted a lightweight older version of Betaflight to fit STM32F103 limitations.
- Enabled SBUS support.

#### Hardware debugging work
- Repaired a broken 5V internal power line using an external jumper.
- Removed the buzzer because of routing conflicts with the main programming interface.
- Reset and simplified ESC configuration for compatibility.

#### Current issues observed
- Random imbalance during flight.
- Sudden motor-speed increase in Angle mode.
- Symptoms consistent with **I-term windup**.
- Motor whining, shaking, and incomplete power delivery.

#### Fixes and improvements attempted
- Designed anti-vibration mounts to isolate the IMU.
- Reworked PID logic and control behavior.

#### Project type
- Custom flight-controller prototype
- PCB design and debugging stage
- Advanced embedded systems development

---

### 6. STM32F405 Advanced Prototype
**Folder:** `DroneProject-stm32f405`

This is one of the most advanced project stages in the repository.

It is based on a more powerful **STM32F405** platform and is intended to become a more complete and professional all-in-one flight-controller design.

#### Design goals
- Support advanced flight stacks such as **Betaflight** and **INAV**.
- Integrate camera, GPS, and Wi-Fi support.
- Solve earlier limitations related to memory size and processing power.
- Prepare the platform for more advanced autonomous and robotics-oriented work.

#### Current status
The board design is approximately **90% complete** and is currently paused before final routing and production.

#### Project type
- Advanced custom PCB flight-controller project
- Professional embedded design stage
- Future main platform

---

### 7. FCWB-ESP12S
**Folder:** `FCWB-ESP12S`

This folder appears to represent supporting or experimental ESP-based work connected to the wider drone-system ecosystem.

It is best presented as a support or communication-related embedded project unless later documentation defines it more specifically.

#### Project type
- Support module
- Embedded wireless / auxiliary experiment

---

## Mechanical Innovations and Test Tools

### DIY Tripod Balance Test Rig
A camera tripod was converted into a safe mechanical testing rig for calibration and PID tuning.

By using the tripod’s ball head, the drone could rotate across the flight axes while remaining physically constrained. This made early tuning and response analysis much safer and allowed limited vertical thrust testing.

### Hall Effect RPM Measurement Concept
To estimate motor RPM without integrated feedback sensors, small magnets were attached to rotating motor parts and read using Hall effect sensors.

This allowed individual motor-speed estimation and helped verify motor consistency before flight.

## What This Repository Includes

This repository includes multiple types of work:
- real drone flight-controller prototypes,
- custom embedded firmware development,
- PCB design and debugging,
- personal engineering experiments,
- AI-assisted programming work,
- wireless interface development,
- and hardware testing concepts.

## Current Status

This is an ongoing engineering project and not a finished product.

The repository will continue to evolve through:
- new tests,
- code improvements,
- documentation updates,
- hardware revisions,
- better folder naming,
- and future prototype completion.

## Main Skills Demonstrated

- Embedded C/C++ development
- Drone flight-control engineering
- Sensor integration and validation
- PCB design and hardware debugging
- PWM / SBUS / UART integration
- PID tuning and control troubleshooting
- Mechanical test setup design
- AI-assisted engineering workflows
- Prototype iteration and system improvement

## Future Improvements

Planned future improvements include:
- clearer naming consistency across project folders,
- project-specific README files,
- more diagrams, images, and videos,
- better documentation for support modules,
- and continued progress on the STM32F405 platform.

## Note

This repository is meant to show real engineering progress, including experiments, failures, redesigns, partial results, and unfinished work. That is intentional and reflects the actual development process.
