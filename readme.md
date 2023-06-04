phpwallboxsniffer
==============

A simple PHP TCP listener for passive Modbus sniffing. 

Usage : report System Current as is reported by an energy meter to your Wallbox. Used for troubleshooting PowerBoost behaviour.

This setup may also be used to sniff Solar inverters / energy meters.


Setup
--------------
You will need :

- a Wallbox Pulsar Plus charger
- a power/current meter, in this case the Ineprometering N1-CT
- an Elfin EW11 serial/modbus wifi bridge/converter
- a device able to run PHP

```
powermeter -> modbus -> charger 
              modbus -> Elfin EW11 TCP client
                        Elfin EW11 TCP client -> PC running PHP tcp listener ip 192.168.1.40 port 99
```
Background:
--------------
A typical home installation has a maximum contracted power, and the corresponding current.
A charger may prioritise the regular use over charging, for this it needs to know this maximum.

In this case, with a Wallbox Pulsar plus using PowerBoost, you set that maximum in your wallbox using a (tiny) rotary switch

```
POSITION    0   1 2  3  4  5  6  7  8   9
MAX CURRENT *PS 6 10 13 16 20 25 32 *PS *PS
```
E.g. the max contracted power is 5.7KW => 20 Amps => setting 5

Using the PowerBoost function, we assume that whenever any home appliance uses power, the charger is throttled down. 

In order to do that, a power meter is connected via modbus, and reported to the charger.

The charger polls the meter continuously, after an initial handshake.

Problem:
--------------
Even when using Powerboost with correct settings, the main fuse may trip. 
The charger "knows" the current that is being used in the whole system, and the current it charges with, 
but Wallbox so far does not bother providing that detail in any interface. 

This makes debugging a pain.

To read out this value, you need to sniff the modbus traffic and decode the responses from the meter.

Installation
--------------

Preparing the charger
- Install charger and power meter according the official instructions and regulations.
- Keep charger switched off

Preparing the EW11
- Power the EW11. Do not use the 12v provided by your charger, this is untested because we have no specs on the max power it can deliver.
- Set the Serial Port parameters to : 9600 8,n,1 , buffer 512, Flow control disable, Protocol settings NONE
- Create a new "Communication profile" : TCP client, server = 192.168.1.40, port 99, route UART

Starting the listener
- check the IP address in the php code, change it to your local IP address
- run the PHP script

Connecting the EW11 
- power off both charger and EW11
- Connect the EW11 serial lines (A,B) with the modbus lines D+,D-
- Power on the EW11

From this point on every second the current measured should be available.

References
-----------
- N1-CT manual :  https://ineprometering.com/wp-content/uploads/2022/01/N1-CT-Short-user-manual-V1.03.pdf
- Modbus decoder helper : https://rapidscada.net/modbus/
- https://www.scadacore.com/tools/programming-calculators/online-hex-converter/


Notes
-----
- no checksum validation is performed, so do not rely on the values reported for automation
- the initial handshake between Wallbox and N1-CT has not been investigated
- the modbus traffic is based on PowerBoost only.
- passively sniffing a modbus should not void any warranty.



Once steady, the buffer holds the following
```
====================
READ Current at register 500A length 02 
01 03 50 0A 00 02   F509 

Response 4 bytes : 3FA1 6873 	
01 03 04 3F A1 68 73 C9 E0 
=========================
READ voltage (5000) length 02
01 03 50 00 00 02 D5 0B 

RESPONSE 3D8F 5C29 ?? = float 0.07 ??
01 03 04 3D 8F 5C 29 3E AA 
========================
READ total active energy (6000) length 02
01 03 60 00 00 02 DA 0B 

RESPONSE 0000 0000
01 03 04 00 00 00 00 FA 33 
========================
READ total active power (5012) length 02
01 03 50 12 00 02 75 0E 

RESPONSE 0000 0000
01 03 04 00 00 00 00 FA 33 
=========================
READ REVERSE Active energy (6018) length 02
01 03 60 18 00 02 5A 0C 

RESPONSE 0000 0000
01 03 04 00 00 00 00 FA 33
=========================
```
