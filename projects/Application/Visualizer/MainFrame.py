import sys as sys

from PyQt4 import QtCore, QtGui

from PyDispatcher import dispatcher as dispatcher

import GraphFrame as GraphFrame 

from Probe import MoteConnector, MoteParser, MoteUtils

class MainFrame(QtGui.QWidget):
    
    def __init__(self):
        # Call constructor     
        QtGui.QWidget.__init__(self)
        
        # Module name
        self.moduleName = 'mainFrame'
        
        # Module signals
        self.to_MainFrame = 'to_MainFrame'
        
        # Detect serial ports
        self.serial_ports = None
        while(not self.serial_ports):
            # Create the MoteProbe list, one for each serial port available
            self.serial_ports = MoteUtils.find_serial_ports()
            
            # If no serial ports are available
            if(not self.serial_ports):
                error_type = "Warning"
                error_msg = "No serial port available is available in your system."
                error_box = QtGui.QMessageBox.warning(self, error_type, error_msg, buttons = QtGui.QMessageBox.Retry | QtGui.QMessageBox.Close)
                
                if(error_box == QtGui.QMessageBox.Retry):
                    pass
                elif(error_box == QtGui.QMessageBox.Close):
                    sys.exit()
        
        # Create a MoteConector
        self.connector = MoteConnector.MoteConnector(serial_ports = self.serial_ports)
                
        # Obtain the MoteParser from the MoteConnector
        self.mote_parser = self.connector.get_parser()
        
        # Connects the MainFrame with the MoteParser
        dispatcher.connect(self._to_MainFrame, sender = self.mote_parser.moduleName, signal = self.to_MainFrame)
        
        # Create the widget that contains the frame
        self.graph = GraphFrame.GraphFrame("")
        
        # Arrange the frame horizontally
        self.layout = QtGui.QHBoxLayout(self)
        
        # Add the frame into the layout
        self.layout.addWidget(self.graph)
    
    # Starts plotting
    def start(self, config):
        # Obtain the configuration
        self.mac_type, self.mac_slots, self.mac_duration = config
        
        # Configure the MoteParser
        self.mote_parser.set_mac_type(mac_type = self.mac_type, mac_slots = self.mac_slots, mac_duration = self.mac_duration)
        
        # Start the MoteParser
        self.mote_parser.start()
    
    # Stops plotting
    def stop(self):
        pass
    
    # Resets plotting
    def reset(self):
        pass
    
    def save(self):
        self.graph.save_graph(mac_type = self.mac_type)
    
    def _to_MainFrame(self):
        self.graph.update_graph(self.mote_parser.elapsed_time, self.mote_parser.stats)
