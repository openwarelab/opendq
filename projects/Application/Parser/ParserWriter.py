import csv as csv
import os as os

class ParserWriter(object):
    
    ext = ".txt"
    
    file = None
    file_path = None
    csv_file = None
    
    def __init__(self, dir = None, name = None, dict = None):
        # Check if the directory exists and create it otherwise
        if not os.path.exists(dir):
            os.makedirs(dir)
        
        # Compile the file path
        self.file_path = os.path.join(dir, name, self.ext)
        
        # Open new file
        self.file = open(self.file_path, 'w')
        
        # Create a new csv writer
        self.csv_file = csv.DictWriter(self.file, self.dict, lineterminator='\n')
    
    def write(self, data):
        self.csv_file.writerow(self.dict)
        
    def close(self):
        # Close current file
        self.file.close()
