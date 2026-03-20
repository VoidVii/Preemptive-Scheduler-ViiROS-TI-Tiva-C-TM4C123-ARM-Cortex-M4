# Preemptive Scheduler ViiROS for ARM Cortex-M4

Ein minimaler, präemptiver Echtzeitkernel für den TI Tiva TM4C123 (ARM Cortex-M4).
Entwickelt als Lernprojekt für RTOS-Konzepte.

Fri Mar 20, 2026 23:03:00: The stack pointer for stack 'CSTACK' (currently 0x2000'0408) is  outside the stack range (0x2000'0480 to 0x2000'247f) 

Die CSTACK-Warnung im Debugger ist normal – Threads laufen auf eigenem Stack (PSP), der außerhalb von CSTACK liegt.