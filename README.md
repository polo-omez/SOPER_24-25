# SOPER (Operating Systems) Labs - 2024/2025

This repository contains the laboratory assignments developed for the **Operating Systems** (*Sistemas Operativos - SOPER*) course (academic year 2024–2025). The projects are implemented in **C** for POSIX/Linux environments, covering core OS concepts such as process lifecycle management, signal handling, concurrent synchronization, and Inter-Process Communication (IPC).

---

## Project Structure & Overview

### **P1: Process Creation & Basic IPC (`P1_Soper/`)**
* **Topics:** Process hierarchies (`fork`, `exec`), standard I/O redirection, and unnamed pipes (`pipe`).
* **Description:** A Proof-of-Work (PoW) cryptocurrency mining simulation. The system manages miner processes (`miner.c`), verification monitors (`monitor.c`), and execution coordinators (`mrush.c`) exchanging blocks and hashes.

### **P2: Signals & Asynchronous Communication (`P2_Soper/`)**
* **Topics:** POSIX signal handling (`sigaction`, `sigprocmask`), asynchronous event dispatching, and alarms.
* **Description:** An electoral simulation system where candidate processes (`candidato.c`) and voter processes (`votante.c`) coordinate campaign events and vote tallies via signal passing managed by signal handlers (`handler.c`). Includes documentation in `Memoria.docx`.

### **P3: Shared Buffers & Concurrency Control (`P3_Soper/`)**
* **Topics:** Inter-process communication, bounded circular buffers (`buffer.c`), mutual exclusion, and process synchronization.
* **Description:** Enhances the mining architecture with synchronized message exchange and buffer queues to simulate concurrent transactions and state validation.

### **P4: POSIX Shared Memory & Semaphores (`P4_Soper/`)**
* **Topics:** POSIX shared memory segments (`shm_open`, `mmap`, `ftruncate`) and POSIX semaphores (`sem_open`, `sem_wait`, `sem_post`).
* **Description:** An advanced mining system (`shm_miner.c`) using shared memory segments for zero-copy data exchange between miners and the monitoring system, with strict concurrency controls and automated telemetry logging (`Salidas Registrator Memoria/`). Includes a detailed report in `Memoria.pdf`.


