import time as time
import struct as struct

from PyDispatcher import dispatcher as dispatcher

from Parser import ParserDQ, ParserFSA, ParserWriter
from Stats import StatsDQ, StatsFSA

MAC_TYPE_FSA = "FSA"
MAC_TYPE_DQ = "DQ"

class MoteParser(object):
    PARSER_PC2MOTE_START = 'A'
    PARSER_PC2MOTE_STOP = 'O'
    
    PARSER_MOTE2PC_DATA = 'D'
    PARSER_MOTE2PC_RESET = 'R'
    
    PARSER_MAC_NONE = '\x00'
    PARSER_MAC_FSA = '\x01'
    PARSER_MAC_DQ = '\x02'
    
    START_CMD = ['\x41', '\x00', '\x00']
    STOP_CMD = ['\x4F', '\x00', '\x00']
    
    start_time = 0
    stop_time = 0
    current_time = 0
    elapsed_time = 0
    
    mac_type = ""
    mac_slots = 0
    mac_duration = 0
    
    dictionary = None
    parser = None
    parser_writer = None
    stats = None

    def __init__(self, mote_connector = None):
        # Module name
        self.moduleName = 'moteParser'

        # Module signals
        self.from_MoteConnector = 'from_MoteConnector'
        self.to_MoteConnector = 'to_MoteConnector'
        self.to_MoteParser = 'to_MoteParser'
        self.from_MoteParser = 'from_MoteParser'
        self.to_MainFrame = 'to_MainFrame'
        
        # Mote connector
        self.mote_connector = mote_connector

        # Connects the MoteProbe with the MoteConnector
        dispatcher.connect(self._from_MoteConnector, sender = self.mote_connector, signal = self.to_MoteParser)
    
    def set_mac_type(self, mac_type = None, mac_slots = None, mac_duration = None):
        if (mac_type == MAC_TYPE_FSA or mac_type == MAC_TYPE_DQ):
            self.mac_type = mac_type
            self.mac_slots = chr(mac_slots)
            self.mac_duration = struct.pack('>H', mac_duration)
        else:
            print "MoteParser: Error, wrong mac_type parameter."
            
    def get_mac_stats(self):
        return self.stats
    
    def start(self):
        # Reset the start time
        self.start_time = 0
        
        # Copy the START command
        command = self.START_CMD[:]
        
        # Select the appropriate MAC
        if (self.mac_type == MAC_TYPE_DQ):
            print("MoteParser: MAC_TYPE_DQ")
            
            # Create parser and obtain dictionary
            self.parser = ParserDQ.ParserDQ()
            
            # Create an appropriate stats
            self.stats = StatsDQ.StatsDQ()
            
            # Append MAC type to the command
            command.append(self.PARSER_MAC_DQ)
            
            # Append MAC slots to the command
            command.append(str(self.mac_slots))
        elif (self.mac_type == MAC_TYPE_FSA):
            print("MoteParser: MAC_TYPE_FSA")
            
            # Create an appropriate parser
            self.parser = ParserFSA.ParserFSA()
            
            # Create an appropriate stats
            self.stats = StatsFSA.StatsFSA()
            
            # Append MAC type to the command            
            command.append(self.PARSER_MAC_FSA)
            
            # Append MAC slots to the command
            command.append(str(self.mac_slots))
            
        # Append MAC duration to the command
        command.append(str(self.mac_duration))
        
        # Obtain an appropriate dictionary
        self.dictionary = self.parser.get_dictionary()
        
        # Create CSV file
        # self.parser_writer = ParserWriter.ParserWriter()
        
        # Send the start command
        self._to_MoteConnector(command)
    
    def _to_MoteConnector(self, data = None):
        print("MoteParser: _to_MoteConnector")
        dispatcher.send(signal = self.from_MoteParser, sender = self.moduleName, data = data)
    
    def _to_MainFrame(self):
        print("MoteParser: _to_MainFrame")
        dispatcher.send(signal = self.to_MainFrame, sender = self.moduleName)
    
    def _from_MoteConnector(self, signal = None, data = None):
        # Unpack the data        
        command, address, payload = data
        
        # MOTE2PC_RESET
        if (command == self.PARSER_MOTE2PC_RESET):
            # Store the stop time
            self.stop_time = time.time()
            
            # Compute the elapsed time
            self.elapsed_time = self.stop_time - self.start_time
            
            print("MoteParser: PC2MOTE_RESET")
            
            # Send the data to the GUI
            self._to_MainFrame()
            
        # MOTE2PC_DATA    
        elif (command == self.PARSER_MOTE2PC_DATA):
            if (self.start_time == 0):
                # Store the start time
                self.start_time = time.time()
                
            # Store the current time
            current_time = time.time()
            
            # Unpack the payload
            mac_type = payload[0]
            payload = payload[1:]

            # Parse the payload
            data = self.parser.parse_frame(current_time, payload)
            
            # Process the stats
            self.stats.process(data)
                
        # Otherwise 
        else:
            print("MoteParser: PC2MOTE_ERROR")
