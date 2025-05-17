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
2) flash the firmware onto an XIAO-ESP32-C3
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
|   | SCK  | GPIO 2 |
|   | DT  | GPIO 2 |

#### Display

| Display | ESP32 |
|---|---|
| VCC | VCC/3.3 |
| GND | GND |
| SCL | GPIO 7 |
| SDA | GPIO 6 |

#### Relay

| Relay | ESP32 | Grinder |
|---|---|---|
| + | VCC/3.3 | |
| - | GND | |
| S | GPIO 4 | |
| Middle Screw Terminal | | push button |
| NO Screw Terminal | | push button |

#### Rotary Encoder

| Encoder | ESP32 |
|---|---|
| VCC/+ | VCC/3.3 |
| GND | GND |
| SW | GPIO 10 |
| DT | GPIO 9 |
| CLK | GPIO 8 |

### MX Cherry Switch

One side to pin 8 
one side to GND

-----------

### BOM

1x XIAO-ESP32-C3  
1x HX711 load cell amplifier  
1x 0.9" OLED Display  
1x KY-040 rotary encoder  
1x 500g load cell 60 x 15,8 x 6,7 https://www.amazon.de/gp/product/B019JO9BWK/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1  
1x 5 volt single channel relay (if you can't hook dirently to the existing relay)

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

The PCB requires that you solder the XIAO to the board. It also makes use of JST-XH connectors on the outward connections. 

-----------