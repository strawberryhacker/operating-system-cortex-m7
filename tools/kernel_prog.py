import serial
import argparse
import math
import sys
import time
   

class flasher:

    START_BYTE = 0xAA
    END_BYTE   = 0x55

    POLYNOMIAL = 0x07

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
                                     baudrate=19200,
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

        print(fcs[0])

        self.com.write(start_byte + cmd_byte + size + data + fcs + end_byte)
        #tmp_list = start_byte + cmd_byte + size + data + fcs + end_byte

        self.com.write(start_byte)
        time.sleep(1)
        self.com.write(cmd_byte)
        time.sleep(2)
        self.com.write(size)
        time.sleep(1)
        self.com.write(data)
        time.sleep(1)
        self.com.write(fcs)
        time.sleep(1)
        self.com.write(end_byte)
        time.sleep(1)
            

        # Listen for the response
        resp = self.com.read(size = 1)

        if (len(resp) == 0):
            print("Timeout occured")
            sys.exit()
        
        # We have a response
        return resp


    def test(self):
        self.serial_open()
        self.send_frame(0xAA, "Hello dude".encode())
        self.serial_close()

# Run the functions
test = flasher()
test.parser()
test.test()