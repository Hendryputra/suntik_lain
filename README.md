Bootloader for LPC2300
------

A basic bootloader for LPC2300 devices, written in C.
Usercode is received via UART0, checked for a valid ECDSA (Bitcoin) signature, and only booted if the signature is valid for a hard coded public key.

Huge thank you goes to the developers of [trezor-crypto](https://github.com/trezor/trezor-crypto). They developed the library used to verify the signatures.

### How to interface with the bootloader
The bootloader expects data on the serial interface (UART0). The baud rate is fixed to 115200 baud (see main.c).
Data is sent in chunks of 4096 bytes. After a chunk has been processed and written to flash, the bootloader sends 0x11 via the serial port.
The application sending the data to the bootloader must not send more than 4096 bytes (plus 16 bytes FIFO) without waiting for the acknowledgement from the device.

### Data format
The data that is sent to the bootloader must comply with the following format:
..* First four bytes are the size of the following data, including the ECDSA signature. Byte order is big endian.
..* Next 64 bytes are the ECDSA signature.
..* The following data is read by the bootloader until the previously sent number of bytes is received.

Data is sent using simple hex encoding. So the byte 0x4a is sent as "4a". Upper and lower case characters are supported.
The first four bytes sent are the amount of unencoded bytes, not the number of characters sent.
##### Example:
Your program has 236 bytes. The signature is added at the start, making the program 300 bytes long. You then add 0x0000012c at the beginning of the file. Your file is now 304 bytes in size.
The file is now hex encoded, resulting in a text file like this:

```
0000012c31828a1a9f...
```

This includes the size of the program (0x12c) and the first five bytes of the signature (31828a1a9f).

### Behaviour of h the bootloader after reset
The bootloader is accepting data if pin 2.1 is logic high on reset. If so, data is expected in the format described above.
If pin 2.1 is logic low, the bootloader tries to start the program in flash. To do so, the signature is checked against the hard coded public key.
If the signature is correct, the main code is executed. If the signature does not match, an LED is toggled infinitely.

### The display driver
This bootloader comes with a simple display driver. The display communicates via a parallel bus on port 1. On reset, the Bitcoin logo is displayed.
