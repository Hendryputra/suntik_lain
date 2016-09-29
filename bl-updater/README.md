Demo program to update code via bootloader
------

This demo shows a basic example on how to interface with the bootloader. Code is written in C and should compile and run on every Linux box.

To compile and run do:

```
make all
./bl-updater [port] [file]
```

[port] is the serial port (/dev/ttyUSB0 for example), [file] is the file to send to the device. Note that the file has to contain the size of the code and a signature
as described in the README.md file for the bootloader.
