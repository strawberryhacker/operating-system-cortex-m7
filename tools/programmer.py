import serial
import argparse
import hashlib
import math
import sys
import time
   

class flasher:
    def __init__(self):
        pass
    
    def parser(self):
        parser = argparse.ArgumentParser(description="Chip flasher");
        parser.add_argument("-com", "--com_port", help="Specify the com port \
            COMx or /dev/ttySx")
        parser.add_argument("-f", "--file", required=True, help="Specifies the binary")
        args = parser.parse_args()

        self.file = args.file;
        self.com_port = args.com_port;

    def generate_hash(self, data):
        h = hashlib.sha256(bytes(data))
        b = h.hexdigest()
        print(b)
        return b


    def serial_open(self):
        try:
            self.com = serial.Serial(port=self.com_port, baudrate=115200, dsrdtr=True);
        except serial.SerialException as e:
            print("You are doomed... ", e)
            exit()

    def serial_close(self):
        self.com.close()
    
    def serial_print(self, data):
        self.com.write(bytes(data))

    def file_read(self):
        f = open(self.file, "rb")
        return f.read()

    def update_progress(self, info, progress):
        bar_size = 50
        block_size = int(round(bar_size*progress))
        sys.stdout.write("%s [%s%s] %i %%\r" % (info, "#"*block_size, "."*(bar_size-block_size), progress*100))
        #sys.stdout.flush()

    def file_transfer(self):

        data = self.file_read()


        length = len(data)
        number_of_block = math.ceil(length / 512)
        print("Number of blocks: ", number_of_block)
        self.serial_open()

        self.serial_print(bytearray([98]))

        time.sleep(0.3)
        curr_size = 0
        for i in range(number_of_block):
            frag = data[i*512:(i+1)*512]
            b = bytearray(frag)
            #if i >= (number_of_block - 1):
            #    b = bytearray([0b10101010, 1, len(frag) & 0xFF, (len(frag) >> 8) & 0xFF]) + b
            #else:
            b = bytearray([0b10101010, 1 if i == (number_of_block - 1) else 0, len(frag) & 0xFF, (len(frag) >> 8) & 0xFF]) + b


            self.serial_print(b)

            # Wait for some ACK
            status = self.com.read()
            if status != b'A':
                print("Error")
                exit()  

            curr_size += len(frag)
            self.update_progress("Programming: ", curr_size / length) 

test = flasher()
test.parser()
test.file_transfer()

#test.serial_open()
#test.serial_print("Hello my name is StrawberryHacker\n")
#test.serial_close()
#test.generate_hash("This is a test")