import serial
import serial.tools.list_ports
from tkinter import *
from tkinter.simpledialog import askfloat
import tkinter.font as tkFont

pad_x = 10
pad_y = 10

# Initial interface setup
root = Tk()
root.geometry("400x600")
root.title("Measurement Station")
default_font = tkFont.nametofont("TkDefaultFont")
default_font.configure(family='Courier', size=18)
text_font = tkFont.nametofont("TkTextFont")
text_font.configure(family='Courier', size=18)
frame = Frame(root)
frame.pack()

# Connection form
connect_panel = Frame(root)
connect_panel.pack(side=TOP)
ports = []
ser = serial.Serial()
ser.baudrate = 115200

def port_scan():
    global ports
    ports_list.delete(0, END)
    ports = serial.tools.list_ports.comports()
    for i in range(len(ports)):
        ports_list.insert("end", ports[i].name)

def port_select(event):
    global ser
    index = event.widget.curselection()[0]
    ser.port = ports[index].name
    readout.pack()
    connect_panel.pack_forget()

connect_label = Label(connect_panel, text="Not connected")
ports_list = Listbox(connect_panel)
ports_list.bind('<<ListboxSelect>>', port_select)
scan_button = Button(connect_panel, text="Scan ports", command=port_scan)
connect_label.pack()
ports_list.pack()
scan_button.pack()

# Data readout display
readout = Frame(root)
readout.pack()

zero_all_button = Button(readout, text = "Zero all")
zero_all_button.pack()

# TODO: Learn node IDs and calibration from output node
node_zeroes = []
node_calibs = []

# Sub-panels for individual nodes: ID label, CL display, calibrate switch
node_panels = []
node_labels = []
node_cls = []
node_switches_calib = []

node_panels.append(Frame(readout))
node_labels.append(Label(node_panels[-1], text = "Sensor 1"))
node_cls.append(Canvas(node_panels[-1], width = 50, height = 50))
node_switches_calib.append(Button(node_panels[-1], text = "Calibrate"))

for i in range(len(node_panels)):
    node_panels[i].pack()
    node_labels[i].pack()
    node_cls[i].pack()
    node_switches_calib[i].pack()

readout.pack_forget()
root.mainloop()

#    def update_measurement(self):
#        bytes = ser.readline()
#        bytes = bytes.decode("utf-8")
#        self.reading = float(str(bytes[0:-2])) # strip newlines (CRLF) and convert
#        self.weight = (self.reading * self.calib) - self.zero
#        txt = "Weight: " + "{:.1f}".format(self.weight) + "g"
#        measurement.config(text = txt)
#        root.after(self.T, self.update_measurement)

