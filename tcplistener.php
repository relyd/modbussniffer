<?php
// Simple TCP listener for getting current use using N1-CT Modbus Current sensor
// together with Elfin EW-11 serial converter.

// set host and port
$host = "192.168.1.40";
$port = 99;

// we expect the master, in this case the Wallbox Pulsar plus to query the N1-CT for current
// we assume there is no mains connected, so there are now power measurements, only current

// Master : READ Current at register 500A length 02
// 01 03 50 0A 00 02  CHECKSUM F509
// Slave : Response
// 01 03 04 3F A1 68 73 C9 E0
// current value is 2 bytes Float - Big Endian (ABCD) so we need to convert HEX to Float

$checkmodbus = "0103500a0002f509010304";

set_time_limit(0);

($socket = socket_create(AF_INET, SOCK_STREAM, 0)) or
    die("Could not create socket\n");
($result = socket_bind($socket, $host, $port)) or
    die("Could not bind to socket\n");
($result = socket_listen($socket, 3)) or
    die("Could not set up socket listener\n");
($tcplistensocket = socket_accept($socket)) or
    die("Could not accept incoming connection\n");

while ($raw = socket_read($tcplistensocket, 512)) {
    $input = bin2hex($raw);
    $check = substr($input, 0, 22);

    if ($check == $checkmodbus) {
        $currentvalue = substr($input, 22, 8);
        echo "\nRaw HEX : " . $input;
        echo "\nClient Message : " . $check;
        echo "\nCurrent Raw bytes : " . $currentvalue;
        echo "\n";
        $current = unpack("G", hex2bin($currentvalue))[1];
        echo "\n" . round($current, 2) . "Amps";
    } else {
        echo "\nWaiting..";
    }
}

// Close sockets
socket_close($tcplistensocket);
socket_close($socket);

?>
