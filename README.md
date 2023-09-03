# DX11-Particle-System
Customizable particle system generator in DX11

A simple particle generator.

![2](https://github.com/JTGaming/DX11-Particle-System/assets/38790084/689d6405-3dac-47a9-ac06-dfbcea6922e2)


All calculations are run on the CPU, before being sent in batches to the GPU for rendering.
A speed-up would be had by moving all calculations to the GPU, but that was out of scope of this task.

There are a myriad of customizations that can be had, some of them being: particle quantity, size, color, lifetime, spread velocity and pattern, gravity and air drag.

This system can handle simulating hundreds of thousands of particles on a reasonable system.

![1](https://github.com/JTGaming/DX11-Particle-System/assets/38790084/4d04eda8-3b06-4df4-b707-390bbfc5d8f0)
