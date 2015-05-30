import numpy as numpy

from PyQt4 import QtGui

from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from matplotlib.ticker import MultipleLocator

class GraphCanvas(FigureCanvas):
    
    def __init__(self, width = 5, height = 4, dpi = 100):
        # Create a figure
        self.figure = Figure(figsize=(width, height), dpi=dpi, facecolor = '#D4D0C8')
        
        # Add the figure to the canvas
        FigureCanvas.__init__(self, self.figure)
        
        # Set the canvas properties on resizing
        FigureCanvas.setSizePolicy(self, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        FigureCanvas.updateGeometry(self)
        
    def plot_figure(self, bars = None, stats = None): 
        data, data_labels = bars 
        time, total, success, error, empty = stats
              
        # Add an axis to the figure
        axis = self.figure.add_subplot(111)
        axis.hold(False)
        
        epsilon = 1e-7
        data = numpy.array(data) + epsilon
        
        n = len(data)
        indices = numpy.arange(n)
        width = 0.35
        axis.bar(indices + width, data, width)
        axis.set_xticks(width + indices + width/2)
        axis.set_xticklabels(data_labels)
        axis.set_ylim([0, 100])
        axis.set_xlabel("Node ID")
        axis.set_ylabel("Percentage (%)")
        axis.grid(which="both")
        
        axis.tick_params(axis='both', labelsize=10)
        
        axis.yaxis.set_major_locator(MultipleLocator(10))
        axis.yaxis.set_minor_locator(MultipleLocator(5))
                
        stats =  "Time: " + str(round(time, 2)) + " seconds" + "\n"
        stats += "Total: " + str(int(total)) + " packets" + "\n"
        stats += "Success: " + str(round((100.0 * success/total),2)) + "%" + "\n"
        stats += "Empty: " + str(round(100.0 * empty/total,2)) + "%" + "\n"
        stats += "Collision: " + str(round(100.0 * error/total,2)) + "%" + "\n"
        stats += "PHY: " +  str(round(total / time,2)) + " pps" + "\n"
        stats += "MAC: " + str(round(success / time,2)) + " pps"
        
        bbox_props = dict(boxstyle="square", fc="grey", ec="0.0", alpha=0.5)
        axis.text(0.90, 0.75, stats, ha="center", va="center", transform=axis.transAxes, size=12, bbox=bbox_props)
        
        self.draw()
    
    def restore_figure(self):
        # Clear the current figure
        self.figure.clear()
        
    def save_figure(self, mac_type = None):
        self.figure.savefig(str(mac_type) + ".pdf", dpi=300, facecolor='w', bbox_inches='tight')
