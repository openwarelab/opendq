# Generic imports
import sys

# PyQT imports
from PyQt4 import QtGui

# Custom imports
from Visualizer import Visualizer

class Application():
    def __init__(self):
        # Create QT application
        self.app = QtGui.QApplication(sys.argv)

        # Fill the QT application 
        self.gui = Visualizer.Visualizer()
    
    def start(self):       
        # Show the QT application
        self.gui.show()
        
        # Run the QT application
        self.app.exec_()
    
def main():
    application = Application()
    application.start()
        
if __name__ == "__main__":
    main()
