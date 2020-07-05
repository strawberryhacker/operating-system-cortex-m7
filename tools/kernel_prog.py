import serial
import argparse
import math
import sys
import time
   

class flasher:

    START_BYTE = 0xAA
    END_BYTE   = 0x55

    POLYNOMIAL = 0xB2

    CMD_ERASE_FLASH     = 0x03
    CMD_WRITE_PAGE      = 0x04
    CMD_WRITE_PAGE_LAST = 0x05

    def __init__(self):
        pass
    
    def parser(self):
        parser = argparse.ArgumentParser(description="Chip flasher");

        parser.add_argument("-c", "--com_port",
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
                                     timeout=1);

        except serial.SerialException as e:
            print(e)
            exit()

    def serial_close(self):
        self.com.close()
    
    def serial_write(self, data):
        self.com.write(bytes(data))

    def file_read(self):
        f = open(self.file, "rb")
        return f.read()

    def wait_ack(self):
        pass

    def calculate_fcs(self, data):
        crc = 0
        for i in range(len(data)):
            crc = crc ^ data[i]

            for j in range(8):
                if crc & 0x01:
                    crc = crc ^ self.POLYNOMIAL
                crc = crc >> 1
        
        return crc

    def send_frame(self, cmd, payload):

        payload_size = len(payload)

        # Start fragment consists of start byte, cmd and little-endian 16 bit
        # size
        start_byte = bytearray([self.START_BYTE])
        cmd_byte   = bytearray([cmd])

        size = bytearray([payload_size & 0xFF, (payload_size >> 8) & 0xFF])

        # Payload 
        data = bytearray(payload)

        # End fragment consist of payload crc
        fcs = bytearray([self.calculate_fcs(cmd_byte + size + payload)])
        end_byte = bytearray([self.END_BYTE])

        self.com.write(start_byte + cmd_byte + size + data + fcs + end_byte)

        # We have a response
        return self.get_response()

    def get_response(self):
        # Listen for the response
        resp = self.com.read(size = 1)

        if (len(resp) == 0):
            print("Timeout occured")
            sys.exit()
        return resp

    def load_kernel(self):

        # Open the com port
        self.serial_open()

        # Load the kernel binary
        kernel_binary = self.file_read()

        # Get the board to enter the bootloader
        self.com.write(b'\x00')
        response = self.get_response()
        if response != b'\x00':
            print("Response includes errors: ", response)
            sys.exit()

        print("Vanilla bootloader started...")

        # Instruct the board to erase some of the flash
        new_kernel_size = len(kernel_binary)

        erase_payload    = bytearray(4)
        erase_payload[0] = new_kernel_size & 0xFF
        erase_payload[1] = (new_kernel_size >> 8) & 0xFF
        erase_payload[2] = (new_kernel_size >> 16) & 0xFF
        erase_payload[3] = (new_kernel_size >> 24) & 0xFF
    
        response = self.send_frame(self.CMD_ERASE_FLASH, erase_payload)

        if response != b'\x00':
            print("Response includes errors: ", response)
            sys.exit()

        # Write the kernel binary to the board in blocks of 512 bytes
        
        # Calculate the number of pages to write
        length = len(kernel_binary)
        number_of_block = math.ceil(length / 512)

        print("Downloading kernel...")
        for i in range(number_of_block):
            binary_fragment = kernel_binary[i*512:(i+1)*512]

            cmd = 0 
            if i == (number_of_block - 1):
                # This is the last block
                cmd = self.CMD_WRITE_PAGE_LAST
            else:
                cmd = self.CMD_WRITE_PAGE

            status = self.send_frame(cmd, binary_fragment)

            if response != b'\x00':
                print("Response includes errors: ", response)
                sys.exit()

        print("Kernel download complete!")

# Run the functions
test = flasher()
test.parser()
test.load_kernel()