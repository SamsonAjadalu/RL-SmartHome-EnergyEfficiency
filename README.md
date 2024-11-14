# Energy Consumption Automation using AI

## Project Overview

This project compares AI-based automation with normal automation in a household setup, focusing on lighting and fan energy use. The published study describes a physical setup using an Atmega328p microcontroller and a 60-day evaluation. See the [published paper](https://journalajrcos.com/index.php/AJRCOS/article/view/516) (DOI: 10.9734/ajrcos/2024/v17i11516).

## Energy Data

Energy values in the datasets and results are expressed in watt-hours (Wh).

```text
Energy (Wh) = Voltage (V) × Current (A) × Time (hours)
Energy (kWh) = Energy (Wh) / 1000
```

## Results

| Phase | AI light (Wh) | AI fan (Wh) | AI total (Wh) | Normal light (Wh) | Normal fan (Wh) | Normal total (Wh) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Pre-override | 729.60 | 7,073.28 | 7,802.88 | 760.00 | 7,368.00 | 8,128.00 |
| Manual override | 333.60 | 3,225.60 | 3,559.20 | 347.50 | 3,360.00 | 3,707.50 |
| Post-override | 703.20 | 6,854.40 | 7,557.60 | 732.50 | 7,270.50 | 8,003.00 |
| **60-day total** | **1,766.40** | **17,153.28** | **18,919.68** | **1,840.00** | **17,998.50** | **19,838.50** |

Over 60 days, AI automation used **18,919.68 Wh** and normal automation used **19,838.50 Wh**, for an energy reduction of **4.63%**.
