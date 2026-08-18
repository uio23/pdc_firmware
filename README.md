# Rocket Power Distribution Controller Firmware
Developed for the University of Waikato Astronautics Club on a microcontroller by Elven Aerospace Industries Ltd.  

## Features
- Turn connected power channels on/off
- ~~Monitor current drawn by each channel~~
- ~~Shutdown channels drawing too much current in flight~~

The latter two features are untested and disabled in the source code because the hardware does not currently support current readings.

## CAN bus protocol
- Exclusively considers frames whose `frame.id` matches `GROUND_CONTROL_CAN_ID`.  
- Expects commands with exactly 8 characters, pad with spaces on the right if needed.  
- Sends response of exactly 8 characters, padded by spaces on the right if needed.

### Command format
`G/S channel_name C/S [1/0]` -> Get or Set the Current or State of the channel with name "channel_name".  
When setting state, pass the new state value as 1 for on and 0 for off. Otherwise, pad the command
with spaces on the right to be 8 characters.  
*e.g. `S H2 S 0` means "Set channel H2 state to 0", i.e. "turn off channel H2".*   
*e.g. `G H2 S<Space><Space>` means "Get channel H2 state".*  
Note:  Current cannot be set.  

### Response format
`channel_name C/S val` -> The Current or State of the channel with name "channel_name" is val (an integer).  
*e.g. `H2 S 1<Space><Space>` means "Channel H2 has state 1", i.e. "Channel H2 is on".*
*e.g. `H2 C 19<Space>` means "Channel H2 is drawing 19A".*

~~If an automatic shutdown of a channel drawing too much power occurs, a message is broadcast:~~  
~~`A channel_name S 0`~~  

## Configuration
Consult the header file `PDC.h` to see/change the `PDC_CAN_ID` and expected `GROUND_CONTROL_CAN_ID`.<br>
This header file also defines the channels, their relevant pins and max current values.

## Author
Alexander Kashpir 

## Credits
- [University of Waikato Astronautics Club](https://www.uwac.nz/)
- [Elven Aerospace Industries Ltd](https://sites.google.com/view/elven-aerospace-industries-ltd/)
- [Arduino MCP2515 CAN interface](https://github.com/autowp/arduino-mcp2515)
