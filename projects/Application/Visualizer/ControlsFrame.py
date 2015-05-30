from Probe import MoteParser

from PyQt4 import QtGui
     
class Config(QtGui.QWidget):
     
    MAC_TYPE_DQ = "Distributed Queuing"
    MAC_TYPE_FSA = "Frame-Slotted ALOHA"
    
    def __init__(self):
        QtGui.QWidget.__init__(self)
    
        font_text = QtGui.QFont()
        font_text.setPointSize(11)
        
        font_title = QtGui.QFont()
        font_title.setPointSize(11)
        font_title.setBold(True)
        
        label_type = QtGui.QLabel("Select MAC type:", self)
        label_type.setFont(font_title)
    
        self.combo_type = QtGui.QComboBox(self)
        self.combo_type.addItem(self.MAC_TYPE_FSA)
        self.combo_type.addItem(self.MAC_TYPE_DQ)
        self.combo_type.setFont(font_text)
        self.combo_type.setFixedWidth(250)
        
        label_nodes = QtGui.QLabel("Number of nodes (1-100):", self)
        label_nodes.setFont(font_title)
    
        self.line_nodes = QtGui.QLineEdit(self)
        self.line_nodes.setFont(font_text)
        self.line_nodes.setFixedWidth(275)
        
        label_duration = QtGui.QLabel("Experiment duration (1000-65000 ms):", self)
        label_duration.setFont(font_title)
    
        self.line_duration = QtGui.QLineEdit(self)
        self.line_duration.setFont(font_text)
        self.line_duration.setFixedWidth(300)
        
        layout = QtGui.QVBoxLayout(self)
        layout.addWidget(label_type)
        layout.addWidget(self.combo_type)
        layout.addWidget(label_nodes)
        layout.addWidget(self.line_nodes)
        layout.addWidget(label_duration)
        layout.addWidget(self.line_duration)
    
    def stop(self):
        pass
    
    def reset(self):
        self.line_duration.clear()
        self.line_nodes.clear()

class Controls(QtGui.QWidget):
    
    start_button = None
    stop_button = None
    reset_button = None
    
    def __init__(self):
        QtGui.QWidget.__init__(self)
        
        font_text = QtGui.QFont()
        font_text.setPointSize(11)
        
        # Create the start button
        self.start_button = QtGui.QPushButton()
        self.start_button.setText("Start")
        self.start_button.setFixedWidth(250)
        self.start_button.setFont(font_text)
        
        # Create the stop button
        self.stop_button = QtGui.QPushButton()
        self.stop_button.setText("Stop")
        self.stop_button.setFixedWidth(250)
        self.stop_button.setFont(font_text)
        
        # Create the reset button
        self.reset_button = QtGui.QPushButton()
        self.reset_button.setText("Reset")
        self.reset_button.setFixedWidth(250)
        self.reset_button.setFont(font_text)
        
        # Create the save button
        self.save_button = QtGui.QPushButton()
        self.save_button.setText("Save")
        self.save_button.setFixedWidth(250)
        self.save_button.setFont(font_text)
        
        layout = QtGui.QVBoxLayout(self)
        layout.addWidget(self.start_button)
        layout.addWidget(self.stop_button)
        layout.addWidget(self.reset_button)
        layout.addWidget(self.save_button)

class ControlsFrame(QtGui.QWidget):
    
    def __init__(self):
        QtGui.QWidget.__init__(self)
        
        self.config = Config()
        self.config.setFixedWidth(325)
        self.controls = Controls()
        self.controls.setFixedWidth(325)

        # Create the button layout
        self.layout = QtGui.QHBoxLayout(self)
        
        # Add the buttons to the layout
        self.layout.addWidget(self.config)
        self.layout.addWidget(self.controls)
    
    def get_values(self):
        mac_type = str(self.config.combo_type.currentText())
        
        if (mac_type == self.config.MAC_TYPE_DQ):
            mac_type = MoteParser.MAC_TYPE_DQ
        elif (mac_type == self.config.MAC_TYPE_FSA):
            mac_type = MoteParser.MAC_TYPE_FSA
        
        mac_nodes = int(self.config.line_nodes.text())
        mac_duration = int(self.config.line_duration.text())
        
        return (mac_type, mac_nodes, mac_duration)
    
    def stop(self):
        self.config.stop()
    
    def reset(self):
        self.config.reset()
