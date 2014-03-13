A basic bootloader for LPC2300 devices, written in C.
Usercode is received via UART0, checked for a valid ECDSA (Bitcoin) signature, and only booted if the signature is valid for a hardcoded public key.

Huge thank you goes to the developers of [trezor-crypto](https://github.com/trezor/trezor-crypto). They developed the library used to verify the signatures.
