# Arduino Flight Controller Prototype

This folder contains the first practical development stage of the drone project using Arduino Uno.

## Purpose

This project was used to explore the basics of quadcopter control, PWM output, embedded logic, and early stabilization behavior on a small 8-bit platform.

## What this project includes

- early flight-controller experimentation
- motor control logic
- embedded timing tests
- initial engineering validation work

## Main technical challenge

The Arduino platform struggled to generate synchronized PWM signals for four motors with the timing precision required for stable quadcopter flight.

Timer conflicts between outputs created latency and unstable behavior during balance correction.

## Outcome

This project was an important learning and validation stage, but it also revealed the limitations of Arduino-class hardware for this application.

## Status

Prototype / early-stage experimental platform.
