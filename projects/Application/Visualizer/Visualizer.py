from PyQt4 import QtCore, QtGui

import ControlsFrame, MainFrame

class Visualizer(QtGui.QMainWindow):
    
    window_title = "FSA and DQ Demonstrator for OpenMote-CC2538"
    
    def __init__(self):
        QtGui.QMainWindow.__init__(self)
        
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        
        self.setWindowTitle(self.window_title)
        self.showMaximized()

        self.file_menu = QtGui.QMenu('&File', self)
        self.file_menu.addAction('&Quit', self.fileQuit, QtCore.Qt.CTRL + QtCore.Qt.Key_Q)
        self.menuBar().addMenu(self.file_menu)

        self.help_menu = QtGui.QMenu('&Help', self)
        self.menuBar().addSeparator()
        self.menuBar().addMenu(self.help_menu)

        self.help_menu.addAction('&About', self.about)

        # Create the main widget that contains all the UI elements
        self.main_widget = QtGui.QWidget()
        self.main_layout = QtGui.QVBoxLayout(self.main_widget)
        
        # Create and add the main frame 
        self.main_frame = MainFrame.MainFrame()
        self.main_layout.addWidget(self.main_frame)
        
        # Create and add the buttons frame
        self.controls_frame = ControlsFrame.ControlsFrame()
        self.main_layout.addWidget(self.controls_frame)
        
        # Set focus on the main widget
        self.main_widget.setFocus()
        self.setCentralWidget(self.main_widget)
        
        # Connect the button clicked events with their respective event handlers 
        QtCore.QObject.connect(self.controls_frame.controls.start_button, QtCore.SIGNAL("clicked()"), self.start)
        QtCore.QObject.connect(self.controls_frame.controls.stop_button, QtCore.SIGNAL("clicked()"), self.stop)
        QtCore.QObject.connect(self.controls_frame.controls.reset_button, QtCore.SIGNAL("clicked()"), self.reset)
        QtCore.QObject.connect(self.controls_frame.controls.save_button, QtCore.SIGNAL("clicked()"), self.save)

    def fileQuit(self):
        self.close()

    def closeEvent(self, ce):
        self.fileQuit()

    def about(self):
        QtGui.QMessageBox.about(self, "About", """""")
    
    # Start an experiment    
    def start(self):
        # Obtain the configuration from the interface
        config = self.controls_frame.get_values()
        
        # Start a test with the current configuration
        self.main_frame.start(config)
    
    # Stop plotting
    def stop(self):
        self.controls_frame.stop()
        self.main_frame.stop()
    
    # Reset the plots
    def reset(self):
        self.controls_frame.reset()
        self.main_frame.reset()
        
    def save(self):
        self.main_frame.save()
