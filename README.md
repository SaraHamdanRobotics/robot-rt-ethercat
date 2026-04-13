# Robot RT EtherCAT

A simple real-time control loop for industrial robot drives using EtherCAT.

## Introduction

This project demonstrates a minimal real-time control loop for industrial robot drives using EtherCAT. The goal is to build a low-level control system from scratch to understand real-time communication, drive state machines, and deterministic execution.

For a detailed introduction to EtherCAT setup and configuration, see [simple_intro.md](simple_intro.md).

## Context (Robotics)

This project is part of building a real-time control stack for robotic manipulators (6-DoF arms), where deterministic communication and low-level control are critical for:

- motion control
- safety
- synchronization across joints

## Features

- EtherCAT master using IgH stack
- PDO mapping and domain configuration
- 1 ms cyclic real-time loop
- CiA 402 state machine (drive enable)
- Process data exchange (read/write)
- Basic position control

## What I Learned

- How to bring slaves from PREOP to OP
- Importance of PDO configuration
- Real-time cyclic communication design
- Drive state machine handling (control/status word)
- Debugging EtherCAT communication issues

## Prerequisites

- C++ compiler (e.g., g++)
- EtherCAT library (IgH stack)
- Root privileges for running EtherCAT applications

## Installation

Clone the repository:

```bash
git clone https://github.com/yourusername/robot-rt-ethercat.git
cd robot-rt-ethercat
```

## Usage

Compile and run the application:

```bash
g++ src/simple_rt_ethercat.cpp -o app -lethercat
sudo ./app
```

## Notes

- Tested with ZeroErr EtherCAT drives
- This is a simplified example and not production-safe

## Contributing

Contributions are welcome! Please open an issue or submit a pull request.

## License

This project is licensed under the MIT License.