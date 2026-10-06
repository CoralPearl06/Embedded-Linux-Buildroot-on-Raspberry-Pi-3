# Embedded Linux-Based Multicore Web Server and Hardware Controller

## Project Overview
This project develops a minimal Embedded Linux system for Raspberry Pi 3 using Buildroot.
The system runs a network-based Web/WebSocket server alongside a concurrent hardware-control application. Users can interact with the system through a web interface, while the hardware controller performs time-sensitive operations.

The project focuses on Operating Systems concepts including:
- Processes and threads
- Multicore scheduling
- CPU affinity
- Task priorities
- Mutexes
- Critical sections
- Race conditions
- Inter-process communication (IPC)

## System Architecture
Web Browser
     |
     | HTTP / WebSocket
     v
Web Server Process
     |
     | IPC
     v
Hardware Controller Process
     |
     | GPIO
     v
LED / Relay / Motor

## Project Structure
- server/ - Web/WebSocket server
- controller/ - Hardware-control application
- ipc/ - Inter-process communication
- tests/ - OS concept demonstrations and tests
- web/ - Web interface
- buildroot/ - Buildroot configuration and deployment
- docs/ - Architecture, testing and project documentation
