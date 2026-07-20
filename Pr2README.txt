CS 4440 - Operating Systems
Project 2

Students:
Esmeralda Amado
Cristian Hernandez Juan
Gustavo Trejo

Files Included:
- producer_consumer.c
- mh.c
- airline.c
- Makefile
- typescript
- Pr2README.txt

Compilation:
Run the following command:

make

Execution:

Problem 1:
./producer_consumer

Problem 2:
./mh N

Problem 3:
./airline P B S F

Observations:

Problem 1:
The bounded-buffer producer/consumer problem was implemented using POSIX threads, a mutex, and semaphores. The program demonstrates synchronization between the producer and consumer while handling empty and full buffer conditions.

Problem 2:
The Mother Hubbard problem was implemented using two threads synchronized with semaphores. The program follows the required order of tasks for the mother and father while avoiding busy waiting.

Problem 3:
The Airline Passengers problem was implemented using multiple threads and synchronization primitives. Passengers are processed through baggage handling, security screening, and boarding before the plane departs.