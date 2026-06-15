# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Typical commands

| Goal | Command |
|------|---------|
| Build (Keil uVision) | `UV4 -b USER/RAI.uvprojx -o build.log` |
| Clean build artifacts | `./keilkilll.bat` |
| Flash binary to MCU | `UV4 -f USER/RAI.uvprojx -o flash.log` |
| Open project in Keil GUI | `UV4 USER/RAI.uvprojx` |

Notes:
- Main build target is `FreeRTOS` in `USER/RAI.uvprojx`.
- Project output name is `RAI`; project is configured to generate both executable and HEX output into `OBJ/`.
- Repository currently has no unit-test framework, no test runner, no lint script, and no Makefile/CMake-based CLI workflow. Do not invent test or lint commands.
- There is no current single-test command because no automated test harness exists in this repository.

## Architecture overview

This is STM32F407VE embedded firmware built around FreeRTOS, ST's Standard Peripheral Library, and a Keil uVision project.

### System shape

- `USER/main.c` is entry point. It calls `systemInit()`, creates a bootstrap FreeRTOS task, then starts scheduler with `vTaskStartScheduler()`.
- `BALANCE/system.c` performs board and subsystem initialization: NVIC, delays, LED, buzzer, enable pin, OLED, keys, UART1/UART3/UART4, CAN, ADC, motor encoders, PWM, I2C, MPU6050, PS2 input, and charging-hardware detection.
- `BALANCE/robot_select_init.c` chooses robot model from potentiometer ADC reading at boot, then loads geometry and drivetrain parameters for that chassis.
- `BALANCE/balance.c` is main motion-control layer. It converts requested robot X/Y/Z motion into per-wheel targets and applies motor output.
- `HARDWARE/usartx.c` packages telemetry and emits it over UART1, UART3, and CAN.
- `BALANCE/show.c` handles OLED status display and battery/charging UI.

### Runtime task model

FreeRTOS tasks are created in `USER/main.c:start_task`:

- `Balance_task` — main motion-control loop, runs at 100 Hz.
- `MPU6050_task` — IMU sampling task.
- `show_task` — OLED, buzzer, battery, and APP display handling, runs at 10 Hz.
- `led_task` — LED status loop.
- `pstwo_task` — PS2 controller input handling.
- `data_task` — telemetry packaging and UART/CAN transmission, runs at 20 Hz.

Most periodic tasks use `vTaskDelayUntil(...)` to keep stable loop timing.

### Control/data flow

1. Hardware init runs in `systemInit()`.
2. Robot type is selected from ADC input, which sets wheel geometry, encoder precision, gear ratio, and drivetrain constants.
3. Motion commands come from one of several control sources: APP, remote controller, PS2, CAN, USART, or ROS over UART3.
4. `Drive_Motor(...)` in `BALANCE/balance.c` performs inverse kinematics for either mecanum or omni mode and writes per-motor target velocities.
5. Main control loop reads encoder feedback, updates motor PWM outputs, and applies polarity differences for different chassis variants.
6. `data_task` computes telemetry frame data from encoder state and IMU state, then sends packed binary frames over UART1, UART3, and CAN IDs `0x101`/`0x102`/`0x103`.
7. `show_task` renders selected mode, motor state, angular velocity, and battery voltage to OLED.

### Key directories

- `USER/` — entry point, STM32 interrupt file, Keil project files.
- `BALANCE/` — high-level robot behavior: init, control, robot model selection, display support.
- `HARDWARE/` — board/peripheral drivers for ADC, CAN, encoders, motors, OLED, timers, UART, PS2, LEDs.
- `MPU6050/` — IMU driver and I2C support.
- `AutoRecharge/` — charging/auto-recharge logic.
- `HUB_DRIVER/` — hub motor / CAN driver support.
- `SYSTEM/` — delay, sys, and USART support glue.
- `FWLIB/` — STM32F4 Standard Peripheral Library.
- `FreeRTOS/` — RTOS kernel, ARM_CM4F port, heap implementation.
- `CORE/` — CMSIS core headers and startup assembly.

### Important project constraints

- Build is Keil-first. Source-of-truth for includes, defines, target MCU, and output settings is `USER/RAI.uvprojx`, not a Makefile.
- Target MCU is `STM32F407VE`.
- Toolchain in project file is ARMCC/Keil (`V5.06 update 6`), with STM32F4 device pack.
- Preprocessor defines in project include `STM32F40_41xxx`, `USE_STDPERIPH_DRIVER`, `__FPU_PRESENT=1`, `__TARGET_FPU_VFP`, `ARM_MATH_CM4`, and `__CC_ARM`.
- FreeRTOS portable layer in active project is `FreeRTOS/portable/RVDS/ARM_CM4F/port.c` with `heap_4.c`.
- `keilkilll.bat` is destructive cleanup for generated artifacts across repo; inspect before extending it.
- Repository includes generated artifacts already (`OBJ/`, `mecanum.hex`). Avoid assuming clean source-only tree.

### External interfaces

- UART1 and UART3 both carry telemetry; comments indicate UART3 is default ROS-facing serial link.
- UART4 is used for Bluetooth APP communication.
- CAN is both control/telemetry path and charging-hardware detection path.
- OLED is primary on-device debug/status surface.
- PS2 controller support is built in.

### Assistant notes

- No `README.md`, `.cursorrules`, `.cursor/rules/`, or `.github/copilot-instructions.md` were found when this file was created.
- If you change task rates, robot model selection, telemetry framing, or motor polarity logic, inspect both `USER/main.c` and `BALANCE/*.c` together before editing.
- If you need to understand why a chassis behaves differently by mode, start with `BALANCE/robot_select_init.c` and `BALANCE/balance.c`.
