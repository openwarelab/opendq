import MoteProbe as MoteProbe
import MoteParser as MoteParser

from PyDispatcher import dispatcher

class MoteConnector(object):

    def __init__(self, serial_ports = None):
        
        # Module name
        self.moduleName = 'moteConnector'
        
        # Module signals
        self.to_MoteProbe    = 'to_MoteProbe'
        self.from_MoteProbe  = 'from_MoteProbe'
        self.to_MoteParser   = 'to_MoteParser'
        self.from_MoteParser = 'from_MoteParser'
        
        # Store the serial ports
        self.serial_ports = serial_ports
        
        # List to store all moteProbe objects
        self.moteProbes = []
        
        for serial_port in self.serial_ports:
            # Creates the MoteProbe
            moteProbe = MoteProbe.MoteProbe(mote_connector = self.moduleName, serial_port = serial_port)
            
            # Appends the MoteProbe to the MoteProbe list
            self.moteProbes.append(moteProbe)
            
            # Connects the MoteProbe with the MoteConnector
            dispatcher.connect(self._from_MoteProbe, sender = moteProbe.moduleName, signal = moteProbe.from_MoteProbe)
            
            # Starts the current MoteProbe
            moteProbe.start()
        
        # Create the MoteParser object
        self.moteParser = MoteParser.MoteParser(mote_connector = self.moduleName)
        
        # Connects the MoteParser with the MoteConnector
        dispatcher.connect(self._from_MoteParser, sender = self.moteParser.moduleName, signal = self.from_MoteParser)
    
    def get_parser(self):
        return self.moteParser
    
    def send(self, data):
        self._to_MoteProbe(data)
    
    def _to_MoteParser(self, data = None):
        print("MoteConnector: _to_MoteParser")
        dispatcher.send(sender = self.moduleName, signal = self.to_MoteParser, data = data)
        
    def _to_MoteProbe(self, data = None):
        print("MoteConnector: _to_MoteProbe")
        dispatcher.send(sender = self.moduleName, signal = self.to_MoteProbe, data = data)
        
    def _from_MoteParser(self, sender = None, data = None):
        print("MoteConnector: _from_MoteParser")
        self.send(data)
    
    def _from_MoteProbe(self, sender = None, data = None):
        # Parse the payload 
        command = data[0]
        address = data[1:2]
        payload = data[3:]
        
        # Create the new data
        data = (command, address, payload)
        
        # Send to MoteParser
        self._to_MoteParser(data)
        
