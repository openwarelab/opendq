import Stats

class StatsFSA(Stats.Stats):
    
    def __init__(self):
        Stats.Stats.__init__(self)
        
        self.success_data_packets = {}
        self.error_data_packets = 0.0
        self.empty_data_packets = 0.0
    
    def process(self, data = None):
        self._process_data(data = data)
    
    def reset(self):
        self.success_data_packets.clear()
        self.error_data_packets = 0.0
        self.empty_data_packets = 0.0
    
    def _process_data(self, data):
        data_state = data['data_state']
        data_address = data['data_address']
      
        key = str(data_address)
        if (data_state == 'SUCCESS'):
            if (key in self.success_data_packets):
                self.success_data_packets[key] += 1
            else:
                self.success_data_packets[key] = 1
        elif (data_state == 'ERROR'):
                self.error_data_packets += 1
        elif (data_state == 'EMPTY'):
            self.empty_data_packets += 1
