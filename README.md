# LoRa Mesh MKR 1300 Device Firmware

This repository contains code to setup and interface with the Arduino MKR WAN 1300 using both LoRaWAN configurations and the RadioHead mesh+serial library.

## Getting Started

1. Open mkr_serial_station/mkr_serial_station.ino in Arduino IDE.
2. Import the RadioHead and MKWAN zip files using Sketch -> Include Library -> Add .ZIP Library
  - __WARNING__: the MKRWAN ZIP is custom so make sure to use this version and DO NOT update it.
3. Add the Arduino MKR WAN 1300 board to your Arduino IDE by going to Tools -> Boards -> Boards Manager and searching + installing its package. 
4. From here you can now compile and upload this sketch to the board.
5. Open the Tools -> Serial Monitor to see how the board is performing on the network. 

## Known bugs

- Currently the Join Network AT command times out. This could be because the timeout length for a response is too short or because there is no gateway to connect to yet. Will test both options when gateway is available
- Sometimes that MKR 1300 will have a bootloader panic and will require a double-tap of the reset button to become responsive again.

## Additional Resources 

- [AT messages reference](https://www.multitech.com/documents/publications/manuals/s000574.pdf)
- [FW bootloader for MKR Murata Module](https://github.com/arduino-libraries/MKRWAN/tree/master/examples/MKRWANFWUpdate_standalone)
- [Murata Software modem communication guide](https://wireless.murata.com/pub/RFM/data/SoftwareModem_Ref.pdf)
