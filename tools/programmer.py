import serial
import argparse
import math
import sys
import time
   

class flasher:
    def __init__(self):
        pass
    
    def parser(self):
        parser = argparse.ArgumentParser(description="Chip flasher");

        parser.add_argument("-com", "--com_port",
                            help="Specify the com port COMx or /dev/ttySx")

        parser.add_argument("-f", "--file",
                            required=True,
                            help="Specifies the binary")

        args = parser.parse_args()

        self.file = args.file;
        self.com_port = args.com_port;

    def serial_open(self):
        try:
            self.com = serial.Serial(port=self.com_port, 
                                     baudrate=115200,
                                     rtscts=True,
                                     dsrdtr=True);

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

    def load_image(self):
        # Read the binary image into data and calculate sizes
        data = self.file_read()
        length = len(data)
        number_of_block = math.ceil(length / 512)

        self.serial_open()

        # Go to the bootloader
        print("Vanilla bootloader starting...")
        self.serial_print(bytearray([23, 234, 2, 0]))

        self.wait_ack()

        # Erase the flash
        flash_erase = bytearray([0b10101010, 2, 4, 0])
        flash_erase += bytearray([len(data) & 0xFF])
        flash_erase += bytearray([(len(data) >> 8) & 0xFF])
        flash_erase += bytearray([(len(data) >> 16) & 0xFF])
        flash_erase += bytearray([(len(data) >> 24) & 0xFF])
        flash_erase += bytearray([0])
        self.serial_print(flash_erase)

        self.wait_ack()

        # Chips is in the bootloader and is ready to receive image
        print("Downloading kernel...")

        for i in range(number_of_block):
            # Current fragment of the binary file
            frag = data[i*512:(i+1)*512]

            # Packet start byte
            packet_start = bytearray([0b10101010])

            # Packet command
            if i == (number_of_block - 1):
                packet_cmd = bytearray([1])
            else:
                packet_cmd = bytearray([0])

            # Packet payload size
            packet_size = bytearray([len(frag) & 0xFF, (len(frag) >> 8) & 0xFF])

            # Packet payload
            packet_payload = bytearray(frag)

            # Packet CRC
            packet_crc = bytearray([0])
            
            # Construct the packet
            packet = packet_start + packet_cmd + packet_size + packet_payload \
                + packet_crc

            self.serial_print(packet)

            self.wait_ack()

        print("Kernel download complete!")

# Run the functions
test = flasher()
test.parser()
test.load_image()