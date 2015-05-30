import CRC as CRC

class HDLCException():
    pass

class HDLC(object):
    HDLC_FLAG = '\x7E'
    HDLC_FLAG_ESCAPED = '\x5E'
    HDLC_ESCAPE = '\x7D'
    HDLC_ESCAPE_ESCAPED = '\x5D'
    
    def __init__(self):
        pass
    
    # Converts a buffer into an HDLC frame
    def hdlcify(self, input = None):
        # Make a copy of the original input
        output = ''.join(input[:])
        
        # Check the CRC checksum
        crc_engine = CRC.CRC()
        for o in output:
            crc_engine.push(o)
        crc_result = crc_engine.get()
        
        # Append the CRC checksum
        output += chr((crc_result >> 8) & 0xFF)
        output += chr((crc_result >> 0) & 0xFF)
        
        # Substitute the HDLC flags
        output = output.replace(self.HDLC_ESCAPE, self.HDLC_ESCAPE + self.HDLC_ESCAPE_ESCAPED)
        output = output.replace(self.HDLC_FLAG, self.HDLC_ESCAPE + self.HDLC_FLAG_ESCAPED)
        
        # Append the HDLC flags
        output = self.HDLC_FLAG + output + self.HDLC_FLAG
        
        return output
    
    # Parses an HDLC frame
    # Returns the extracted frame or -1 if wrong CRC checksum
    def dehdlcify(self, input = None):
        # Check if the input contains HDLC_FLAG
        assert input[0] == self.HDLC_FLAG
        assert input[-1] == self.HDLC_FLAG
        
        # Make a copy of the original input
        output = input[1:-1]
        
        # Replace the HDLC flags
        output = output.replace(self.HDLC_ESCAPE + self.HDLC_FLAG_ESCAPED, self.HDLC_FLAG)
        output = output.replace(self.HDLC_ESCAPE + self.HDLC_ESCAPE_ESCAPED, self.HDLC_ESCAPE)
        
        # Check output input size
        if (len(output) < 2):
            raise HDLCException("Packet is too short!")
        
        # Get the CRC checksum
        crc_value = int(output[-2:].encode('hex'), 16)
        
        # Compute the CRC checksum
        crc_engine = CRC.CRC()
        for o in output[:-2]:
            crc_engine.push(o)
        crc_result = crc_engine.get()
        
        # Check CRC checksum
        if (crc_value != crc_result):
            raise HDLCException("CRC checksum not valid!")
        
        # Remove the CRC checksum
        output = output[:-2]
        
        return output

def test1():
    input = "\x31\x32\x33\x34\x35\x36\x37\x38\x39"
    check = "\x7E\x31\x32\x33\x34\x35\x36\x37\x38\x39\x29\xB1\x7E"
    
    hdlc_test = HDLC()
    result = hdlc_test.hdlcify(input)
    
    assert (result == check)  

def test2():
    input = "\x31\x32\x33\x7E\x34\x35\x7D\x36\x37\x38\x39"
    check = "\x7E\x31\x32\x33\x7D\x5E\x34\x35\x7D\x5D\x36\x37\x38\x39\xDF\x72\x7E"
    
    hdlc_test = HDLC()
    result = hdlc_test.hdlcify(input)
    
    assert (result == check)
        
def test3():
    input = "\x31\x32\x33\x34\x35\x36\x37\x38\x39"
    
    hdlc_test = HDLC()
    result = hdlc_test.hdlcify(input)
    result = hdlc_test.dehdlcify(result)
    
    assert (result == input)

def test4():
    input = "\x31\x32\x33\x7E\x34\x35\x7D\x36\x37\x38\x39"
    
    hdlc_test = HDLC()
    result= hdlc_test.hdlcify(input)
    result = hdlc_test.dehdlcify(result)
    
    assert(result == input)
    
if __name__ == "__main__":
    test1()
    test2()
    test3()
    test4()
    
