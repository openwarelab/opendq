import struct as struct

from collections import OrderedDict

from Parser import Parser, uint2int

class ParserFSA(Parser):
    
    def __init__(self):
        # Create an ordered dictionary
        self.dict = OrderedDict([
                 ('current_time', None),
                 ('current_slot', None),
                 ('data_state', None),
                 ('data_address', None),
                 ('data_rssi', None),])
    
    def parse_frame(self, time, payload):
        # Parse the data into bytes
        data = struct.unpack('<BBHBB', payload)
        
        # Put the bytes into the dictionary
        self.dict['current_time'] = str(time)
        self.dict['current_slot'] = str(data[0])
        self.dict['data_state']   = self._parse_data(data[1])
        self.dict['data_address'] = str(data[2])
        self.dict['data_rssi']    = uint2int(data[3])
        
        return self.dict
    
    def _parse_data(self, data_state):
        if (data_state == 0):
            return "EMPTY"
        elif (data_state == 1):
            return "ERROR"
        elif (data_state == 2):
            return "SUCCESS"
        else:
            return "UNDEFINED"
    
