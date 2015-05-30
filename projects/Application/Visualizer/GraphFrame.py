import numpy as numpy

import GraphCanvas as GraphCanvas

from PyQt4 import QtCore, QtGui

class GraphFrame(QtGui.QWidget):
    
    def __init__(self, title):            
        QtGui.QWidget.__init__(self)
        
        # Create a font for the graph frame title
        font = QtGui.QFont("", 16, QtGui.QFont.Bold, False)
        
        # Create the title of the graph frame
        self.title_label = QtGui.QLabel()
        self.title_label.setText(title)
        self.title_label.setFont(font)
        self.title_label.setAlignment(QtCore.Qt.AlignHCenter)
        
        # Create the canvas that holds the life graphic
        self.canvas = GraphCanvas.GraphCanvas()
        
        # Arrange the widgets vertically
        self.layout = QtGui.QVBoxLayout(self)
        
        # Add the title and the canvas to the layout 
        self.layout.addWidget(self.title_label)
        self.layout.addWidget(self.canvas)
        
    # Updates the graph
    def update_graph(self, time = None, stats = None):
        self.time = time
        self.stats = stats
        
        bars = self._compute_bars()
        stats = self._compute_stats()
        
        self.canvas.plot_figure(bars = bars, stats = stats)
        
    def save_graph(self, mac_type = None):
        self.canvas.save_figure(mac_type)
    
    def _compute_bars(self):
        bars = []
        bars_labels = []
        
        success = []
        keys = sorted(self.stats.success_data_packets.iterkeys())
        for key in keys:
            success.append(self.stats.success_data_packets[key])
        total = sum(success) + self.stats.error_data_packets + self.stats.empty_data_packets
                
        success = 100 * numpy.array(success) / total
        bars.extend(success.tolist())
        
        hex_labels = [hex(int(k)) for k in keys]
        bars_labels.extend(hex_labels)
                
        bars.append(100 * self.stats.empty_data_packets / total)
        bars_labels.append("Empty")
        
        bars.append(100 * self.stats.error_data_packets / total)
        bars_labels.append("Collision")
        
        return (bars, bars_labels)
        
    def _compute_stats(self):
        time = self.time
        
        success = self.stats.success_data_packets 
        error = self.stats.error_data_packets
        empty = self.stats.empty_data_packets
        
        total = error + empty 
        count = 0.0
        for s in success.keys():
            count += success[s]
        success = count
        total += success
        
        stats = time, total, success, error, empty
        
        return stats
