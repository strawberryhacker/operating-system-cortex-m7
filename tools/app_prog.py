import serial
import argparse
import math
import sys
import time
   

class flasher:
    def __init__(self):
        pass
    
    def parser(self):
        parser = argparse.ArgumentParser(description="Chip flasher")

        parser.add_argument("-com", "--com_port",
                            help="Specify the com port COMx or /dev/ttySx")

        parser.add_argument("-f", "--file",
                            required=True,
                            help="Specifies the binary")

        args = parser.parse_args()

        self.file = args.file
        self.com_port = args.com_port

    def serial_open(self):
        try:
            self.com = serial.Serial(port=self.com_port, 
                                     baudrate=115200,
                                     rtscts=True)

        except serial.SerialException as e:
            print(e)
            exit()

    def serial_close(self):
        self.com.close()
    
    def serial_print(self, data):
        self.com.write(bytes(data))

    def file_read(self):
        f = open(self.file, "rb")
        return f.read()

    def wait_ack(self):
        # Wait for some ACK
        status = self.com.read(1)
        if status != b'\x00':
            print("Error")
            print(status)
            exit()

    def load_app(self):
        # Read the binary image into data and calculate sizes
        data = self.file_read()
        length = len(data)

        print("Size: ", length)

        self.serial_open()

        # Erase the flash
        app_size = bytearray([0xAA])
        app_size += bytearray([len(data) & 0xFF])
        app_size += bytearray([(len(data) >> 8) & 0xFF])
        app_size += bytearray([(len(data) >> 16) & 0xFF])
        app_size += bytearray([(len(data) >> 24) & 0xFF])

        self.serial_print(app_size)

        time.sleep(1)

        # Chips is in the bootloader and is ready to receive image
        print("Downloading application...")

        self.serial_print(bytearray(data))

        print("Application download complete!")


# Run the functions
test = flasher()
test.parser()
test.load_app()