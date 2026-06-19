# Robot Safety Simulator

Qt Widgets based robot safety state simulator.

This repository is currently used for mid-stage collaboration.

## Purpose

The program receives robot safety related input values and determines the robot state.

Main input values:

- Human distance
- Obstacle distance
- Door open rate
- Robot speed
- Deceleration
- Emergency signal

Output states:

- MOVE
- WARNING
- DANGER
- STOP

## Main Files

- `robotSafety/RobotRiskSimulator.pro`: Qt project file
- `robotSafety/main.cpp`: program entry point
- `robotSafety/SafetyModel.h`: input and output data structures
- `robotSafety/SafetyModel.cpp`: safety decision logic
- `robotSafety/riskpage.h`: UI class declarations
- `robotSafety/riskpage.cpp`: UI, simulation, drawing, logging, and button behavior

## How to Run

1. Open Qt Creator.
2. Open `robotSafety/RobotRiskSimulator.pro`.
3. Select a Qt Kit.
4. Build the project.
5. Run the program.

## Current Status

The Risk Page is implemented as the main working page.

Implemented features:

- Slider based sensor input
- Robot state decision
- Brake distance calculation
- Safety margin calculation
- Risk score display
- QPainter based simulation view
- Risk history view
- Simulation log table
- CSV export
- Dark and light mode toggle

## Collaboration Note

Final cleanup is not finished yet. Build folders may still exist in this repository during collaboration. Before final submission, build output folders should be removed and `.gitignore` should be added.
