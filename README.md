# Rocket Power Distribution Controller Firmware
Developed for the University of Waikato Astronautics Club on a microcontroller by Elven Aerospace Industries Ltd. & Gareth Reid.

## Features
- Toggle connected power channels on/off
- Broadcast channel states and battery voltages
- ~~Monitor E-fuse currents and states~~

Hardware does not currently support the last feature.

## CAN bus protocol
- For set requests, exclusively considers frames whose `frame.id` matches `GROUND_CONTROL_CAN_ID`.  
- Responses are always 8 characters long, numbers may be left-padded with 0s to achieve this.  

### Request format
`channel_name/battery_name S/V [val]` -> Get or Set the state of a channel, get the voltage of a battery or set the voltage percentage adjustment.  
When setting state, pass `val` to 1 for on and 0 for off. When setting voltage percentage, the format is vvv for vvv% .  
*e.g. `C2 S` means "Get channel C2 state".*  
*e.g. `C2 S 0` means "Set channel C2 state to 0", i.e. "turn off channel H2".*   
*e.g. `B1 V` means "Get battery B1 voltage".*
*e.g. `B1 V 112` means "Scale the voltage sense of B1 to 112%".*

### Response format
`channel_name/battery_name S/V val` -> The state or voltage or voltage percentage of the channel or battery with the matching name is val (an integer).  
*e.g. `C2 S 1` means "Channel C2 has state 1", i.e. "Channel C2 is on".*  
*e.g. `B1 V 152` means "Battery B1 is at 15.2V OR Battery B1 voltage is scaled 152%"*

### Broadcast format
Every 250 milliseconds, the PDC will broadcast 2 8-character frames over the CAN bus, the first reporting the states of the channels,
and the second reporting the voltages of the connected batteries.  
This is the format of each message, for logging purposes (only the values are not actually separated by spaces):  
```
S:C1 C2 C3 C4 C5 F1 -> Each state is a 1 character bit
V:B1 BF -> Each voltage is a 3 character decimal
```
*e.g. `S:100000` means "C1 is on, the rest of the channels are off"*  
*e.g. `V:000100` means "Battery B1 is at 0 volts, battery BF is at 10.0 volts"*  

## Configuration
Consult the header file `PDC.h` for configuration.  
To change what B1 and BF battery's voltage percentage adjustment is initialised to, edit `B1_PERC` and `BF_PERC`, (default 100%).  
To change the PDC's CAN ID or the expected ground control CAN ID, edit `PDC_CAN_ID` and `GROUND_CONTROL_CAN_ID`.   
This header file also defines the broadcast frequency (time), and the channel and battery names with their relevant pins.  

## Author
Alexander Kashpir 

## Credits
- [University of Waikato Astronautics Club](https://www.uwac.nz/)
- [Elven Aerospace Industries Ltd](https://sites.google.com/view/elven-aerospace-industries-ltd/)
- [Arduino MCP2515 CAN interface](https://github.com/autowp/arduino-mcp2515)
