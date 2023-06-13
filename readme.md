modbussniffer
==============

A simple PHP TCP listener for passive Modbus sniffing. 

Usage : report data from a modbus energy meter, used by a Wallbox charger. 

Used for troubleshooting WallBox PowerBoost behaviour.

This setup may also be used to sniff Solar inverters - Energy meter combinations

(If you want it faster/easier, see the arduino/C++ script for an ESP8266 that pushes the Current value of the N1-CT that is reported to the wallbox to an MQTT broker)


Setup
--------------
You will need :

- a Wallbox Pulsar Plus charger
- a power/current meter, in this case the Ineprometering N1-CT
- an Elfin EW11 serial/modbus wifi bridge/converter
- a device able to run PHP

```
Connections (see image): 
powermeter -> modbus -> charger 
modbus -> Elfin EW11
Elfin EW11 -> WIFI AP -> PC running PHP tcp listener
```

Background:
--------------
A typical home installation has a maximum contracted power, and the corresponding current.
A charger may prioritise the regular use over charging, for this it needs to know this maximum.

In this case, with a Wallbox Pulsar plus using PowerBoost, you set the maximum in your wallbox using a (tiny) rotary switch

```
POSITION    0   1 2  3  4  5  6  7  8   9
MAX CURRENT *PS 6 10 13 16 20 25 32 *PS *PS
```
E.g. the max contracted power is 5.7KW => 20 Amps => setting 5

Using the PowerBoost function, we assume that whenever home appliances use energy, the charger is throttled down or even switched off. 

In that case it should show "Waiting for energy"

In order to do that, a power meter is connected via modbus, and reported to the charger.

The charger polls the meter continuously, after an initial handshake.

Problem:
--------------
Even when using Powerboost with correct settings, the main fuse may trip for various reasons, like clamp misfit, cable losses etc. 
The charger "knows" the current that is flowing in the whole system, and the current it charges with, 
but Wallbox so far does not bother providing that detail in any interface, nor the web interface, nor the mobile app.

This makes debugging a pain. Furthermore the precise algorithm how powerboost responds is not clear from the documentation.

To read out the total current, you need to sniff the modbus traffic and decode the responses from the meter.

Installation
--------------

Preparing the charger
- Install charger and power meter according the official instructions and regulations.
- Keep charger switched off

Preparing the EW11
- Power the EW11. Do not use the 12v provided by your charger, this is untested because we have no specs on the max power it can deliver.
- Set the Serial Port parameters to : 9600 8,n,1 , buffer 512, Flow control disable, Protocol settings NONE
- Create a new "Communication profile" : TCP client, server = 192.168.1.40, port 99, route UART (in this case we presume the monitoring device that runs the sniffer has 192.168.1.40 as the IP address) 

Starting the listener
- check the IP address in the php code, change it to your local IP address. 
- run the PHP script

Connecting the EW11 
- power off both charger and EW11
- Connect the EW11 serial lines (A,B) with the modbus lines D+,D-
- Power on the EW11
- Power on the charger

From this point on every second the current measured should be available.

References and tools
-----------
- N1-CT manual :  https://ineprometering.com/wp-content/uploads/2022/01/N1-CT-Short-user-manual-V1.03.pdf
- EMS installation manual : https://support.wallbox.com/wp-content/uploads/2022/12/EN_EMS_Installation_Guide_English.pdf
- activating powerboost : https://support.wallbox.com/en/knowledge-base/how-to-activate-power-boost/
- Modbus decoder helper : https://rapidscada.net/modbus/
- Hex converter : https://www.scadacore.com/tools/programming-calculators/online-hex-converter/
- Realterm : easy to send sample hex data via a USB-RS485 for testing.
- 

Wallbox Pulsar plus notes
-----
- Working on mains equipment is dangerous. Don't electrocute yourself. Know the rules.
- Wallbox, Pulsar and PowerBoost are registered names of Wallbox.com. This repo is not affiliated with or endorsed by Wallbox.
- The initial handshake between Wallbox and N1-CT has not been investigated, it probably reads meter ID, sets comm parameters etc.
- The modbus traffic analysed here is based on PowerBoost only.
- Passively sniffing a modbus should not void any warranty.
- For PowerBoost operation, it is not required that the mains voltage is connected to the N1-CT, hence it will not report any voltage or power data.
- The best solution would be that Wallbox.com would push ALL known data to a user settable Mqtt broker. Thank you.

General Notes
-------------
- No modbus checksum validation is performed, so do not rely on the values reported for automation
- In case of trouble check your modbus termination. It depends on the distance between charger and meter.
- Don't be tempted to add another master to the modbus and query the meter next to the charger. 
- For a more permanent solution, a dedicated ESP32 with Tasmota running modbusmonitor would be a much better option. See https://github.com/arendst/Tasmota/discussions/18618 
- If you want to contribute, please help testing the Tasmota variant
- See the arduino script for a simple ESP8266 modbus to MQTT bridge


Additional information
----------------------
Once steady after bootup of the wallbox, the following data can be observed:
```
====================
READ Current at register 500A length 02 
01 03 50 0A 00 02   F509 

Response 4 bytes : 3FA1 6873 	=> 1.25 Amps
01 03 04 3F A1 68 73 C9 E0 
=========================
READ voltage (5000) length 02
01 03 50 00 00 02 D5 0B 

RESPONSE 3D8F 5C29 
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
