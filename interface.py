import serial
import serial.tools.list_ports
from tkinter import *
from tkinter.simpledialog import askfloat
from tkinter.messagebox import showinfo
import tkinter.font as tkFont
import pickle
import os
import time

pad_x = 10
pad_y = 10

# Sensor physical arrangement
# UI elements are in this order:
# 0 1 2
# 3 4 5
# 6 7 8
sensor_map = {1: 1}

# Initial interface setup
root = Tk()
root.geometry("500x500")
root.title("Measurement Station")
default_font = tkFont.nametofont("TkDefaultFont")
default_font.configure(family='Courier', size=16)
text_font = tkFont.nametofont("TkTextFont")
text_font.configure(family='Courier', size=16)

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

def weight_to_cl(weight):
    D_cup = 8.7 # cm
    density = 1 # g/cm^3
    A_top = 3.1415926 * (D_cup / 2) ** 2
    coverage_cm = weight / density
    # * cm^3/gal, / cm^2/(100 ft^2)
    return coverage_cm * 3785 / 92900

# TODO: Allow for disconnection
def update_measurement():
    #print("Measuring")
    T = int(1000 / 80)
    #n_read = 0
    timeout = int(T / 10)
    start = time.time()

    # Wait for updates from each node (not necessarily in order)
    for i in range(n_nodes):
        # Wait only for a bit
        while not ser.in_waiting:
            if (time.time() - start) > timeout:
                print("Read timed out")
                break

        if ser.read(1) != b'\00':
            print("Error! Lost synchronization")

        id_byte = ser.read(4)
        id = int.from_bytes(id_byte, byteorder='little')
        #print(f"ID {id_byte}: {id}")

        calib = 1
        zero = 0
        if id in node_calibs:
            calib = node_calibs[id]
        if id in node_zeroes:
            zero = node_zeroes[id]

        reading_bytes = ser.read(4)
        reading = int.from_bytes(reading_bytes, byteorder='little')
        last_readings[id] = reading
        #print(f"Reading: {reading_bytes}, or {reading}")
        last_readings[id] = reading
        weight = (reading - zero) * calib
        #print(f"Reads {reading} for weight {weight}")

        # Update canvas to show new coverage level
        draw_cup(node_cls[sensor_map[id]], cl=weight, scale=0.5)
        root.after(T, update_measurement)

def sync_serial():
    bytes = b'\01'
    print("Syncing")
    t = time.time()
    while bytes != b'\00':
        while not ser.in_waiting:
            if (time.time() - t) > 0.01:
                print("Nothing on that port")
                ser.close()
                return False

        bytes = ser.read(1)
        #print(bytes)
    print("Synced")
    # Throw out the rest of this packet
    #print(ser.read(8))
    ser.read(8)
    update_measurement()

def port_select(event):
    global ser
    sel = event.widget.curselection()
    if sel is None:
        return
    index = event.widget.curselection()[0]
    ser.port = ports[index].device
    ser.open()
    if sync_serial():
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
#readout.grid(row=2, column=2)
readout.pack()

def zero_all():
    for id in node_zeroes:
        if id in last_readings:
            node_zeroes[id] = last_readings[id]

def calibrate(id):
    print(id)
    showinfo(title="Begin calibration", message="Empty this node, then press OK.")
    # Zero first
    zero = last_readings[id]
    actual_weight = askfloat(f"Calibrate node ${id}", "Add something, then enter its weight:")
    node_calibs[id] = actual_weight / (last_readings[id] - zero)
    pickle.dump(node_calibs, calib_file)

zero_all_button = Button(readout, text = "Zero all", command=zero_all)
zero_all_button.pack()

# TODO: Learn node IDs and calibration from output node
#n_rows = 3
#n_cols = 3
#n_nodes = n_rows * n_cols
n_nodes = 9
node_zeroes = {}
node_calibs = {}
last_readings = {}
node_cls = []

# Sub-panels for individual nodes: ID label, CL display, calibrate switch
node_panels = []
node_labels = []
node_switches_calib = []

def cup_pts(scale, height):
    base_size = 50
    full_height = 1.4 * base_size * scale
    full_width = base_size * scale
    top = 10
    bottom = top + full_height
    top = bottom - height * full_height
    left = 10
    taper = base_size * 0.15
    return (
        (left + taper * (1 - height), top),
        (left + taper, bottom),
        (left + full_width - taper, bottom),
        (left + full_width - taper * (1 - height), top)
    )

def draw_cup(canvas, cl, scale):
    canvas.delete("all")
    cup_outline = cup_pts(scale, 0.6)
    water = cup_pts(scale, 0.9)
    canvas.create_polygon(*water, fill='blue', outline='', tags="temp")
    canvas.create_polygon(*cup_outline, fill='', outline='black', width = 2 * scale, tags="static")
    canvas.create_text(50, 30, text = str(round(cl)), fill = "black", font = f"TkDefaultFont {int(24 * scale)}", tags="temp")

# Set up a panel for each node
i = 0
#for r in range(n_rows):
#    for c in range(n_cols):
for i in range(n_nodes):
        node_panels.append(Frame(readout))
        node_labels.append(Label(node_panels[-1], text = f"Sensor {i}"))
        node_cls.append(Canvas(node_panels[-1], width = 100, height = 45, bg='white'))
        draw_cup(node_cls[i], 0, 0.5)
        node_switches_calib.append(Button(node_panels[-1], text = "Calibrate", command=lambda: calibrate(i+1)))
        node_panels[i].pack(side=LEFT)
        #node_panels[i].grid(row=r, column=c)
        node_labels[i].pack()
        node_cls[i].pack()
        node_switches_calib[i].pack()
        i = i + 1

# Read calibration data from file
calibs_file_path = 'calib.pkl'
if os.path.exists(calibs_file_path):
    with open(calibs_file_path, 'rb') as calib_file:
        node_calibs = pickle.load(calib_file)
else:
    with open(calibs_file_path, 'wb') as calib_file:
        pickle.dump(node_calibs, calib_file)

readout.pack_forget()
root.mainloop()