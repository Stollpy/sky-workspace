## Tools
- Soldering iron with fine tip
- Solder paste and stencil (for QFN/BGA/0402 components)
- Hot air rework station
- Microscope or magnifying lamp
- Precision tweezers
- Multimeter with continuity test
- Precision screwdriver set (M1.4 compatible)
- 3D printer (FDM for PETG, SLA for clear resin)
- Deburring tool or craft knife
- USB-C to USB-A/C cable (for programming)
- Small pliers or wire cutters

## Assumptions
- Custom 4-layer PCB has been manufactured externally
- Builder has experience with surface-mount device (SMD) soldering, including fine-pitch and QFN/BGA packages
- Basic familiarity with ESP32 firmware development (ESP-IDF or Arduino IDE)
- Access to 3D printing resources for specified materials and settings
- Understanding of I2C, SPI, UART, I2S, and PDM communication protocols

## 1. Fabrication
### 1.1 3D print all PETG enclosure parts
*(not yet generated)*

### 1.2 3D print HR sensor optical lens from clear resin
*(not yet generated)*

### 1.3 Clean and deburr all 3D printed components
*(not yet generated)*

### 1.4 Inspect custom PCB for manufacturing defects
*(not yet generated)*

## 2. Wiring
### 2.1 Solder ESP32-PICO-V3-02 and AXP2101 to PCB
*(not yet generated)*

### 2.2 Solder LTE (SIM7080G, LDO, bulk capacitor, eSIM, FPC connector) and GPS (MIA-M10Q, FPC connector) modules/components
*(not yet generated)*

### 2.3 Solder I2C (PCF8563, BMI270, MAX30102, DRV2605L, FT6336U), SPI (ST7789V3), and audio (SPM1423HM4H-B, MAX98357A) components
*(not yet generated)*

### 2.4 Solder USB-C connector, power/boot buttons, and passives (0402 resistors, capacitors)
*(not yet generated)*

### 2.5 Connect LRA vibration motor and small speaker to their respective drivers on the PCB
*(not yet generated)*

### 2.6 Connect LiPo battery to AXP2101 input pads and coin cell battery to PCF8563 input pads
*(not yet generated)*

## 3. Bring-up
### 3.1 Perform initial power-on via USB-C and verify AXP2101 operation and voltage rails
*(not yet generated)*

### 3.2 Connect ESP32 via USB-C and verify basic connectivity and flash initial bootloader/firmware
*(not yet generated)*

### 3.3 Test all I2C buses (Bus 0 & Bus 1) to confirm recognition of AXP2101, PCF8563, BMI270, MAX30102, DRV2605L, and FT6336U
*(not yet generated)*

### 3.4 Test SPI display (ST7789V3) functionality and confirm image output and backlight control
*(not yet generated)*

### 3.5 Verify PDM microphone (SPM1423HM4H-B) input and I2S speaker amplifier (MAX98357A) output
*(not yet generated)*

### 3.6 Initiate communication with SIM7080G LTE module and MIA-M10Q GPS module via UART
*(not yet generated)*

## 4. Assembly
### 4.1 Mount PCB into the main enclosure using standoffs and screws
**Mount the main PCB into the smartwatch enclosure with standoffs and screws.**
1. Carefully position the assembled 38x34mm PCB inside the 3D-printed Smartwatch Main Enclosure.
2. Align the PCB's mounting holes with the four integrated mounting points in the enclosure.
3. Insert four M1.4x3mm PCB Mounting Standoffs through the PCB's mounting holes into the enclosure.
4. Secure the PCB by fastening four M1.4x2mm PCB Mounting Screws into the standoffs, ensuring a snug fit.
  > Tip: Avoid overtightening the tiny screws to prevent damage to the PCB or 3D-printed enclosure mounts.

### 4.2 Install USB-C port gasket and waterproof button gaskets, then insert button actuators
*(not yet generated)*

### 4.3 Secure the display (ST7789V3) and touch panel (FT6336U) into the retaining frame, then attach to the main enclosure with adhesive tape
*(not yet generated)*

### 4.4 Place the LiPo battery within the enclosure, stacked as specified
*(not yet generated)*

### 4.5 Insert the HR sensor optical lens into the back cover, then secure the back cover to the main enclosure
*(not yet generated)*

### 4.6 Integrate LTE and GPS FPC antennas into the smartwatch strap and connect to FPC connectors on PCB
*(not yet generated)*

### 4.7 Attach the completed watch case to the smartwatch strap
*(not yet generated)*

### 4.8 Perform final system test of all functions
*(not yet generated)*
