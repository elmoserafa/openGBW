# Differences to sybutter's openGBW:
- back to using ESP32 DevKit V1

# Recent Changes & Fixes (July 2025)

## Platform Migration
- **Hardware**: Migrated from ESP32-C3 to ESP32 DevKit v1
- **Pin Configuration**: Updated all GPIO pin assignments for ESP32 DevKit v1 compatibility
- **Build System**: Updated PlatformIO configuration for esp32dev board

## Rotary Encoder Improvements
- **Fixed Input Detection**: Resolved issue where rotary encoder input was not being registered
- **Enhanced Responsiveness**: Improved encoder sensitivity for precise value setting
- **Precision Tuning**: Calibrated encoder to provide exact 0.1g increments for weight adjustment and 0.01g for offset adjustment
- **Wake on Input**: Added wake functionality for both encoder rotation and button press when screen is asleep
- **Debugging**: Added comprehensive serial output for encoder troubleshooting

## Technical Fixes
- **GPIO Compatibility**: Changed encoder button pin from GPIO 34 to GPIO 27 (supports pull-up resistors)
- **Interrupt Handling**: Added manual interrupt attachment for reliable encoder operation
- **Pin Configuration**: Implemented proper INPUT_PULLUP modes for encoder pins
- **Menu Navigation**: Enhanced menu system responsiveness and accuracy
- **Grinder Control**: Added support for NPN transistor control of 5V grinder signals that need to be pulled to ground

## Documentation Updates
- **Wiring Tables**: Updated all pin assignments to match ESP32 DevKit v1
- **Component References**: Changed hardware references from ESP32-C3 to ESP32 DevKit v1
- **Troubleshooting**: Added encoder calibration and debugging information

-----------

# Differences to jb-xyz's openGBW:
 Extends https://github.com/jb-xyz/openGBW

 - Commented the code and added some more structure to it all
 - Cup weight now has an escape to it as well as a confirmation screen to subvert the escape
 - Info menu to see offset and cup weight
 - Wake on rotary turn
 - Sleep adjustable via menus instead of code compile
 - Now uses XIAO-ESP32-C3
 - Added the ability to use a button as a grinder trigger

-----------

# OpenGBW

This Project extends and adapts the original by Guillaume Besson

More info: https://besson.co/projects/coffee-grinder-smart-scale


This mod will add GBW functionality to basically any coffe grinder that can be started and stopped manually.

The included 3D Models are adapted to the Eureka Mignon XL and the Macap Leo E but the electronics can be used for any Scale.

-----------

### Getting started

1) 3D print the included models for a Eureka Mignon XL or design your own
2) flash the firmware onto an ESP32 DevKit v1
3) connect the display, relay, load cell and rotary encoder to the ESP32 according to the wiring instructions
4) go into the menu by pressing the button of the rotary encoder and set your initial offset. -2g is a good enough starting value for a Mignon XL
5) if you're using the Mignon's push button to activate the grinder set grinding mode to impulse. If you're connected directly to the motor relay use continuous.
6) if you only want to use the scale to check your weight when single dosing, set scale mode to scale only. This will not trigger any relay switching and start a timer when the weight begins to increase. If you'd like to build your own brew scale with timer, this is also the mode to use.
7) calibrate your load cell by placing a 100g weight on it and following the instructions in the menu
8) set your dosing cup weight
5) exit the menu, set your desired weight and place your empty dosing cup on the scale. The first grind might be off by a bit - the accuracy will increase with each grind as the scale auto adjusts the grinding offset

-----------

### Wiring

#### Load Cell

| Load Cell  | HX711 | ESP32  |
|---|---|---|
| black  | E-  | |
| red  | E+  | |
| green  | A+  | |
| white  | A-  | |
|   | VCC  | VCC/3.3 |
|   | GND  | GND |
|   | SCK  | GPIO 17/TX2|
|   | DT  | GPIO 16/RX2| 

#### Display

| Display | ESP32 |
|---|---|
| VCC | VCC/3.3 |
| GND | GND |
| SCL | GPIO 22 |
| SDA | GPIO 21 |

#### Grinder Control

**Option 1: Direct Relay Control**
| Relay | ESP32 | Grinder |
|---|---|---|
| + | VCC/3.3 | |
| - | GND | |
| S | GPIO 35 | |
| Middle Screw Terminal | | push button |
| NO Screw Terminal | | push button |

**Option 2: NPN Transistor Control (for 5V signal that needs to be pulled to ground)**
| Component | Connection |
|---|---|
| Resistor (1kΩ) | GPIO 33 → NPN Base |
| NPN Emitter | GND |
| NPN Collector | Grinder 5V signal line |
| Grinder 5V+ | Grinder power supply |

*When ESP32 GPIO 33 goes HIGH → NPN turns ON → Pulls 5V signal to ground → Grinder starts*  
*When ESP32 GPIO 33 goes LOW → NPN turns OFF → 5V signal stays high → Grinder stops*

#### Rotary Encoder

| Encoder | ESP32 |
|---|---|
| VCC/+ | VCC/3.3 |
| GND | GND |
| SW | GPIO 27 |
| DT | GPIO 32 |
| CLK | GPIO 23 |

### MX Cherry Switch

One side to GPIO 25 
one side to GND

-----------

### BOM

1x ESP32 DevKit v1  
1x HX711 load cell amplifier  
1x 0.9" OLED Display  
1x KY-040 rotary encoder  
1x 500g load cell 60 x 15,8 x 6,7 https://www.amazon.de/gp/product/B019JO9BWK/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1  

**Grinder Control Options:**
- Option 1: 1x 5V single channel relay (for direct relay control)
- Option 2: 1x NPN transistor (2N2222, BC547, or similar) + 1x 1kΩ resistor (for 5V signal control)

Various jumper cables I would recommend shielded cables  
A few WAGO or similar connectors

-----------

### 3D Files

You can find the 3D STL models on thangs.com

Eureka XL: https://thangs.com/designer/jbear-xyz/3d-model/Eureka%20Mignon%20XL%20OpenGBW%20scale%20addon-834667?manualModelView=true

These _should_ fit any grinder in the Mignon line up as far as I can tell.

There's also STLs for a universal scale in the repo, though it is mostly meant as a starting off point to create your own. You can use the provided files, but you'll need to print an external enclosure for the ESP32, relay and any other components your setup might need.

-----------

### PCB

This PCB was designed using the many resources in EAF and an absolute crap ton of youtube. KiCad is truly amazing. 

If you want to just **get the pcb** made:
The **openGBW_pcbv1.3FF.zip** is the zipped file you'll want to send to PCBWay/JLCPCB 

If you want to **modify and remix** the pcb:
The **openGBW_v1.3_proj.zip** is the file you'll want to download

This was done in KiCad as well. 

The PCB requires that you solder the ESP32 DevKit v1 to the board. It also makes use of JST-XH connectors on the outward connections. 

-----------

### Troubleshooting

#### Rotary Encoder Issues

**Problem**: Encoder button works but rotation is not detected
- **Solution**: Check wiring connections to GPIO 23 (CLK) and GPIO 32 (DT)
- **Debugging**: Monitor serial output at 115200 baud for encoder delta values

**Problem**: Encoder is unresponsive or requires many clicks to change values
- **Solution**: Encoder sensitivity has been calibrated for standard KY-040 encoders
- **Technical**: Each physical detent now provides exactly 0.1g increment for weight and 0.01g for offset

**Problem**: Values jump in large increments instead of precise steps
- **Solution**: Recent firmware updates have fixed precision issues
- **Verification**: Check serial monitor for "Weight:" and "Offset:" debug messages

**Problem**: Encoder input not registered at all
- **Potential Causes**: 
  - GPIO 34 cannot be used for buttons (input-only, no pull-up support)
  - Missing pull-up resistors on encoder pins
  - Incorrect interrupt configuration
- **Solutions**: 
  - Use GPIO 27 for encoder button (has pull-up support)
  - Ensure proper INPUT_PULLUP pin modes
  - Verify manual interrupt attachment in code

#### General Hardware Issues

**Problem**: Scale readings are unstable
- **Solution**: Ensure load cell is properly mounted and HX711 connections are secure
- **Calibration**: Use the menu system to calibrate with a known 100g weight

**Problem**: Display not working
- **Solution**: Check I2C connections on GPIO 21 (SDA) and GPIO 22 (SCL)
- **Power**: Verify 3.3V power supply to display

-----------