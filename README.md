# LoRa Mesh MKR 1300 Device Firmware

This repository contains all of the necessary files and libraries to flash the LoRa module with custom code and then communicate with the LoRa module using the main microcontroller. Below are the steps to guide you through the process. 

## Required boards

Download these boards onto your Arduino IDE:

  * Arduino MKR1300
    - Use for flashing the main microcontroller.
  * [Grasshopper-L082CZ](https://github.com/GrumpyOldPizza/ArduinoCore-stm32l0)
    - Use for compiling the binaries for the LoRa module

## Steps to build

These are the full steps to flash both modules.

### Write and compile LoRa module firmware

1. Write whatever code you would like to go on the LoRa module to this file: mlm32l07x01.ino
2. Go to Sketch->Include Library->Add Zip Library and add "RadioHead-1.112.zip"
3. Select the Grasshopper board from the Tools->Board menu.
4. Select Sketch->Verify/Compile (NOT UPLOAD).
5. Select Sketch->Export Binary, it should show up as mlm32l07x01.bin (if .dfu simply rename)

### Load the compiled Firmware onto the LoRa module

1. Convert the binary from last section into a header file using this command: 

```
echo -n "const " > fw.h && xxd -i mlm32l07x01.bin >> MKRFirmwareUpdate/fw.h
```

2. Now open MKRFirmwareUpdate/MKRWANFWUpdate_standalone.ino in Arduino
3. Select the MKR 1300 board from the Tools->Board menu.
4. Select Sketch->Upload
5. Open Tool->Serial Monitor and confirm the successful flashing

### Load code onto the main controller

1. Open mkr_serial_listener.ino in Arduino
2. Select the MKR 1300 board from the Tools->Board menu.
3. Select Sketch->Upload
4. Open Tool->Serial Monitor and confirm the successful flashing
5. See the two controllers in action interacting with eachother. 

## What can the current code do. 

* The main controller simply parrots back what the LoRaWAN module is getting for now. 
* The LoRaWAN module is successfully configuring itself as a RadioHead mesh controller and tries to construct a mesh table of the nodes that it should be able to see. It prints these results for the main controller to see. 



