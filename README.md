# Energy Consumption Automation using AI

## Project Overview
This project investigates energy consumption patterns using AI-based automation compared to standard Automation in a household setup. The focus is on optimizing energy usage for appliances such as lights and fans.

The study primarily involved physical setups using the Atmega328p microcontroller to gather real-world data over two months. Simulations were conducted to validate hardware configurations before deployment.

The full findings and methodologies are documented in a published thesis. For detailed information, please refer to the complete thesis [here](https://journalajrcos.com/index.php/AJRCOS/article/view/516).

---

## Data Collection
Key aspects of the data collection process include:

- Physical setups using the Atmega328p microcontroller to control lighting and fan systems.
- Real-world data capturing energy usage across various phases: pre-override, manual override, and post-override.
- Energy consumption was calculated using the formula: 

    ```
    Energy (kWh) = Voltage (V) × Current (A) × Time (hours)
    ```
- Simulations were used to test hardware scenarios before actual implementation.

All datasets, including hourly and daily energy consumption, are stored in CSV files within the `/data` directory.

---

## Results
Key findings from the project:

- The AI automation system achieved noticeable energy savings compared to standard Automation, particularly during the manual override and post-override phases.
- AI automation adapted to human behavior patterns to optimize energy usage while maintaining comfort.
- The `/results` folder contains graphs and tables showcasing energy consumption trends, comparisons, and cumulative savings.

---

