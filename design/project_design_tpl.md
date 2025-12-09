# Human Machine Interface (HMI) Project Design

| **Author**              | `Ali Akram - Mahmoud Adgham - Ziad Khalil`                                       |
|:------------------------|:-----------------------------------------------------|
| **Status**              | `Release`                          |
| **Version**             | `1.0`                                                |
| **Date**                | `9/12/2025`                                         |

## Introduction

This document provides the detailed software design for the "Automotive Air Conditioning Control Panel" (ThermoGuard) project. This system is developed for the AVR ATmega328P microcontroller as part of the KH5023FTE Embedded System Design & Development module.

### Purpose

This project aims to build a reliable electronic system that functions as a complete automotive air conditioning control panel. It serves as the central processing unit that links user inputs to the physical cooling hardware. The system has four main goals:

1.  **Monitor and Display:** The system will constantly measure the current ambient temperature using an analog sensor (ADC) and clearly show the temperature, system status, and animations on the LCD screen.
2.  **Allow User Configuration:** The system will watch the KeyPad for button presses to adjust the Target Temperature, toggle "Turbo Mode" (Max Cooling), and toggle "Swing Mode" (Air Flow Distribution).
3.  **Regulate Temperature:** The system will automatically control the Fan (Motor) speed and direction to adjust the environment until it matches the temperature the user requested.
4.  **Safety & Efficiency:** The system will monitor for critical overheating to trigger emergency shutdown procedures and dim the display (Eco Mode) during periods of inactivity to save power.


### Scope




## Architectural Overview

This section describes where this module resides in the context of the software architecture
```plantuml



```

## Assumptions & Constraints
### Assumptions


### Constraints

## Functional Description



#### Application Logic Flow
```plantuml

```

## Implementation of the Module



## Integration and Configuration
### Static Files
All source and header files that comprise the project are listed below.
| File name | Contents |
| :--- | :--- |
| **...** |... |
| **..\_..** | ... |
| **...** | .... |
| **..** |  |
| **....** | .... |
| **....** | .... |
| **....** |.... |
| **....** | .... |
| **....** | .... |
| **....** | ..... |

### Include Structure


```plantuml


```

### Configuration
All hardware and application-level constants are defined in Hardware_Defs.h for easy configuration.
| Name | Value | Description |
|:---|:---|:---|
| *`LCD Pins`* | | |
| .... | .... | .... |
| .... | .... | .... |

| *`LED Pin`* | | |
| .... | .... | .... |
| .... | .... | .... |
| *`ADC Channels`* | | |
| .... | .... | .... |
| .... | .... | .... |
| *`Keypad Thresholds`*| | .... |

