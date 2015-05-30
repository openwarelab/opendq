import Stats

class StatsDQ(Stats.Stats):
        
    def __init__(self):
        Stats.Stats.__init__(self)
        
        self.success_data_packets = {}
        self.error_data_packets = 0.0
        self.empty_data_packets = 0.0
        
        self.success_arp_packets = {}
        self.error_arp_packets = 0.0
        self.empty_arp_packets = 0.0
        
        self.crq_global = []
        self.dtq_global = []
    
    def process(self, data = None):
        self._process_arp(data = data)
        self._process_data(data = data)
        self._process_queue(data = data)
    
    def reset(self):
        self.success_data_packets.clear()
        self.error_data_packets = 0.0
        self.empty_arp_packets = 0.0
        
        self.success_arp_packets.clear()
        self.error_arp_packets = 0.0
        self.empty_arp_packets = 0.0
        
        self.crq_global[:] = []
        self.dtq_global[:] = []
    
    def _process_arp(self, data):
        arp1_state = data['arp1_state']
        arp1_random = data['arp1_random']
        arp1_rssi = data['arp1_rssi']
        arp2_state = data['arp2_state']
        arp2_random = data['arp2_random']
        arp2_rssi = data['arp2_rssi']
        arp3_state = data['arp3_state']
        arp3_random = data['arp3_random']
        arp3_rssi = data['arp3_rssi']
        
        arp1 = (arp1_state, arp1_random, arp1_rssi)
        arp2 = (arp2_state, arp2_random, arp2_rssi)
        arp3 = (arp3_state, arp3_random, arp3_rssi)
        
        arps = (arp1, arp2, arp3)
        
        for arp in arps:
            arp_state, arp_random, arp_rssi = arp
            
            key = str(arp_random)
            if (arp_state == 'SUCCESS'):
                if (key in self.success_arp_packets):
                    self.success_arp_packets[key] += 1
                else:
                    self.success_arp_packets[key] = 1
            elif (arp_state == 'COLLISION'):
                self.error_arp_packets += 1
            elif (arp_state == 'EMPTY'):
                self.empty_arp_packets += 1
      
    def _process_queue(self, data):
        self.crq_global.append(int(data['crq_global']))
        self.dtq_global.append(int(data['dtq_global']))
    
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
