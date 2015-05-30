import threading as threading
import serial as serial

from PyDispatcher import dispatcher as dispatcher

import HDLC as HDLC

class MoteProbe(threading.Thread):
    
    def __init__(self, mote_connector = None, serial_port = None):
        # Call constructor
        threading.Thread.__init__(self)
        
        # Set thread as daemon
        self.setDaemon(True)
        
        # Terminate thread event
        self._stopEvent = threading.Event()
        
        # Serial port name and data rate
        self.serial_name, self.serial_rate = serial_port
        self.serial_port = None
        
        # Module name
        self.moduleName = "moteProbe@" + self.serial_name
        self.from_MoteProbe = 'from_MoteProbe'
        
        # Signals
        self.to_MoteConnector = 'to_MoteConnector'
        self.from_MoteConnector = 'from_MoteConnector'
        self.to_MoteProbe = 'to_MoteProbe'
        self.from_MoteProbe = 'from_MoteProbe'
        
        # Mote connector
        self.mote_connector = mote_connector
        
        # HDLC driver
        self.hdlc = HDLC.HDLC()
        
        # Receive variables
        self.receive_buffer = ''
        self.is_receiving = False
        self.rx_byte = 0
        self.last_rx_byte = 0
        
        # Transmit variables 
        self.transmit_buffer = ''
        
        # Attach the signal from the MoteConnector
        dispatcher.connect(receiver = self._from_MoteConnector, sender = self.mote_connector, signal = self.to_MoteProbe)
    
    # Runs the MoteProbe thread
    def run(self):
        try:
            # Open the serial port
            self.serial_port = serial.Serial(port = self.serial_name, baudrate = self.serial_rate, timeout = 0.1)
            self.serial_port.flush()
        except:
            print("MoteProbe: Error, serial port not found.")
        
        # Execute while thread is alive
        while(not self._stopEvent.isSet()):
            # Try to receive a byte from the serial port (blocking)
            try:
                # Read a byte from the serial port
                self.rx_byte = self.serial_port.read(size = 1)
            except:
                # Print error message
                print("MoteProbe: Error, serial port not ready.")
                
                # Terminate the thread
                self.stop()
                
                # Break the loop
                break
            
            # Start of frame
            if ((not self.is_receiving) and 
                self.last_rx_byte == self.hdlc.HDLC_FLAG and
                self.rx_byte != self.hdlc.HDLC_FLAG):
                # Now we are receiving
                self.is_receiving = True
                
                # Receive the first byte
                self.receive_buffer = self.hdlc.HDLC_FLAG
                self.receive_buffer += self.rx_byte
                
            # Middle of HDLC frame
            elif(self.is_receiving and self.rx_byte != self.hdlc.HDLC_FLAG):
                # Append the current byte
                self.receive_buffer += self.rx_byte
                
            # End of HDLC frame 
            elif(self.is_receiving and self.rx_byte == self.hdlc.HDLC_FLAG):
                # Receive the last byte
                self.receive_buffer += self.rx_byte
                
                try:
                    # Call to HDLC to dehlcify the received buffer
                    output = self.hdlc.dehdlcify(self.receive_buffer)
                except:
                    print("MoteProbe: Error, HDLC parsing unsuccessful.")
                    print(self.receive_buffer.encode("hex"))
                else:
                    # Send message to MoteConnector
                    self._to_MoteConnector(data = output)
                
                # Reset the variables
                self.is_receiving = False
                self.receive_buffer = ''
                self.rx_byte = 0
                self.last_rx_byte = 0
            
            # Otherwise, check if we need to transmit
            else:
                # Check if there is something to transmit (blocking)
                if(self.is_receiving == False and self.transmit_buffer):
                    print("MoteProbe: Writting message to serial_port.") 
                    
                    # Send the message through the serial port
                    self.serial_port.write(self.transmit_buffer)
                    
                    # Empty the transmit buffer
                    self.transmit_buffer = ''
                
            # Always save the last received byte
            self.last_rx_byte = self.rx_byte
        
        print("MoteProbe: Thread finished!")
    
    # Stops the UART thread
    def stop(self):
        print("MoteProbe: Finishing thread...")
        # If the serial port exists
        if (self.serial_port != None):
            # Close the serial port
            self.serial_port.close()
            
        # Terminates the thread
        self._stopEvent.set()

    # Send a message to MoteConnector
    def _to_MoteConnector(self, data = None):
        dispatcher.send(sender = self.moduleName, signal = self.from_MoteProbe, data = data)
        
    # Receive a message from MoteConnector
    def _from_MoteConnector(self, sender = None, data = None):
        print("MoteProbe: _from_MoteConnector")
        
        # Convert the data to HDLC format
        output = self.hdlc.hdlcify(data)
        
        # Point the transmit buffer to the output
        self.transmit_buffer = output
