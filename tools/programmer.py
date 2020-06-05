import serial
import argparse
import hashlib
   

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
        h = hashlib.sha256(bytes(data, 'utf-8'))
        print(h.hexdigest())
        print(len(h.hexdigest()))


    def serial_open(self):
        try:
            self.com = serial.Serial(port=self.com_port, baudrate=115200, dsrdtr=True);
        except serial.SerialException as e:
            print("You are doomed... ", e)
            exit()

    def serial_close(self):
        self.com.close()
    
    def serial_print(self, data):
        self.com.write(bytes(data, 'utf-8'))
    

test = flasher()

#test.parser()

#test.serial_open()
#test.serial_print("Hello my name is StrawberryHacker\n")
#test.serial_close()
#test.generate_hash("This is a test")

#define WIDTH  (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))
def crc_t():
    remainder = 0
    data = []
    for div in range(0, 256): # 1- 256?
        remainder = div << (8 - 8)
        
        for bit in range(8, 0, -1):
            if remainder & (1 << 0):
                remainder = (remainder << 1) ^ 0xCA
            else:
                remainder = (remainder << 1)
                
        # write array
        data.append(remainder)
    # print array
    if len(data) == 256:
        for i in data:
            print(hex(i), end=', ')
crc_t()