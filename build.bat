@echo off
echo Building OS PBL Simulation...
gcc -c List.c
gcc -c PCB.c
gcc -c deadlock.c
gcc -c main.c
gcc List.o PCB.o deadlock.o main.o -o sim.exe
echo Build Complete. Run sim.exe to start.
