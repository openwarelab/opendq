from collections import OrderedDict

def uint2int(x):
    return -(256 + (~(x)+1)) 

class Parser(object):
    
    dict = OrderedDict()
    
    def __init__(self):
        pass  
    
    def parse_frame(self, time, payload):
        return self.dict
    
    def get_dictionary(self):
        return self.dict
