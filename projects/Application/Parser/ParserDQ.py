import struct as struct

from collections import OrderedDict

from Parser import Parser, uint2int

class ParserDQ(Parser):
    
    def __init__(self):
        # Create an ordered dictionary
        self.dict = OrderedDict([
                         ('current_time', None),
                         ('arp1_state', None),
                         ('arp1_random', None),
                         ('arp1_rssi', None),
                         ('arp2_state', None),
                         ('arp2_random', None),
                         ('arp2_rssi', None),
                         ('arp3_state', None),
                         ('arp3_random', None),
                         ('arp3_rssi', None),
                         ('data_state', None),
                         ('data_address', None),
                         ('data_arp', None),
                         ('crq_global', None),
                         ('dtq_global', None)])
    
    def parse_frame(self, time, payload):
        # Parse the data into bytes
        data = struct.unpack('<BBBBBBBBBBHHHHHHHHHH', payload)
        
        # Put the bytes into the dictionary
        self.dict['current_time'] = str(time)
        self.dict['arp1_state'] = self._parse_arp(data[0])
        self.dict['arp1_random'] = str(data[10])
        self.dict['arp1_rssi'] = uint2int(data[4])
        self.dict['arp2_state'] = self._parse_arp(data[1])
        self.dict['arp2_random'] = str(data[11])
        self.dict['arp2_rssi'] = uint2int(data[5])
        self.dict['arp3_state'] = self._parse_arp(data[2])
        self.dict['arp3_random'] = str(data[12])
        self.dict['arp3_rssi'] = uint2int(data[6])
        self.dict['data_state'] = self._parse_data(data[3])
        self.dict['data_address'] = str(data[13])
        self.dict['data_arp'] = str(data[7])
        self.dict['crq_global'] = str(data[15])
        self.dict['dtq_global'] = str(data[18])
        
        return self.dict

    def _parse_arp(self, arp_state):
        if(arp_state == 0):
            return "EMPTY"
        elif(arp_state == 1):
            return "COLLISION"
        elif(arp_state == 2):
            return "SUCCESS"
        else:
            return "UNDEFINED"

    def _parse_data(self, data_state):
        if(data_state == 0):
            return "EMPTY"
        elif(data_state == 1):
            return "ERROR"
        elif(data_state == 2):
            return "SUCCESS"
        else:
            return "UNDEFINED"
