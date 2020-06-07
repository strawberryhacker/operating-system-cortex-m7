# Programmer

This python script can be used to update the vanilla kernel firmware (or any image in general). It talks over a shared serial port with the Cortex-M7 processor.

## Usage

```console
straberryhacker@home:~$ python3 programmer.py [-h] [-com] [-f]
```

- [-h] - help 
- [-com] - specify the COM port
- [-f] - path to the .bin file

The script will set up a serial connection with the following configuration

- Stopbits: **one**
- Parity: **none**
- Flow control: **DTR/DSR**
- Baud rate: **115200**
