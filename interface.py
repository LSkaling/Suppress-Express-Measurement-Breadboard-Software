"""
Tkinter interface to manage measurement system
Ben Lees, 2024-2025

Implemented:
 - Select serial port in GUI
 - Read serial data from receiver
 - Display current measurement level in GUI
 - Zero and calibrate nodes in GUI
 - Comprehensive data logging
Not yet implemented:
 - Monitor and troubleshoot connection status
 - Dynamically manage node layout (hardcoded for now)
 - "Snapshot" data logging
"""

import serial
import serial.tools.list_ports
from tkinter import *
from tkinter.simpledialog import askfloat
from tkinter.messagebox import showinfo
import tkinter.font as tkFont
import pickle
import os
import time

# Maps sensor IDs to indices
sensor_map = {1: 0, 2: 1, 3: 2,
              4: 3, 5: 4, 10: 5,
              11: 6, 12: 7, 13: 8}
# Indices take following row-column indexing (hardcoded for 3x3 prototype):
# 0 1 2
# 3 4 5
# 6 7 8

# Initial interface setup
root = Tk()
root.geometry("1000x600")
root.title("Measurement Station")
default_font = tkFont.nametofont("TkDefaultFont")
default_font.configure(family='Courier', size=16)
text_font = tkFont.nametofont("TkTextFont")
text_font.configure(family='Courier', size=16)

def port_scan():
    global ports
    # Clear list and populate with current serial ports
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
    #T = int(1000 / 80)
    T = 100
    #status = "Received node: weight "

    # Wait for updates from each node (not necessarily in order)
    while ser.in_waiting:
        assert ser.in_waiting > 11, "Synchronization error (<12 bytes)"
        with open("log.txt", "a") as logfile:
            #print("Found " + str(ser.in_waiting) + " in buffer")
            packet = ser.read(12)

            # Log entire packet in hex
            logfile.write(packet.hex())
            assert int.from_bytes(packet[0:4], 'little') == (2**32 - 1), "Synchronization error (missed sync bytes)"

            id = int.from_bytes(packet[4:8], byteorder='little')
            #status = status + str(id) + ", "
            #print(f"ID {id_byte}: {id}")
            logfile.write("\nID" + str(id))
            if id in sensor_map:
                calib = 0.142
                zero = 0
                unit = ""
                if id in node_calibs:
                    #logfile.write(" -c- ")
                    calib = node_calibs[id]
                    unit = "g"
                if id in node_zeroes:
                    zero = node_zeroes[id]

                reading = int.from_bytes(packet[8:12], byteorder='little')
                last_readings[id] = reading
                weight = (reading - zero) * calib
                last_weights[id] = weight

                # Update canvas to show new coverage level
                logfile.write(" r" + str(reading) + " t" + str(round(time.time() - t0, 2)) + "\n")
                draw_cup(node_cls[sensor_map[id]], cl=weight, scale=0.5, unit=unit)
            else:
                logfile.write("ERROR")

    #status = status + "at time T + " + str(time.time() - t0) + "\n"
    #print(status)
    root.after(T, update_measurement)

def sync_serial():
    bytes = b'\00'
    print("Syncing")
    t = time.time()
    while bytes != b'\xff\xff\xff\xff':
        while not ser.in_waiting:
            if (time.time() - t) > 1:
                print("Nothing on that port")
                ser.close()
                return False

        bytes = ser.read(4)
        #print(bytes)
    print("Synced")
    # Throw out the rest of this packet
    #print(ser.read(8))
    ser.read(8)
    # Start measurements
    global t0
    t0 = time.time()
    with open("log.txt", "a") as logfile:
        logfile.write("\nBeginning session at time " + str(round(t0, 2)) + "\n")
    root.after(50, update_measurement)
    #print("After function call")
    return True

def port_select(event):
    global ser
    sel = event.widget.curselection()
    if sel is None:
        return
    index = event.widget.curselection()[0]
    ser.port = ports[index].device
    ser.open()
    #sync_serial()
    if sync_serial():
        connect_panel.pack_forget()
        readout.pack()

def zero_all():
    for id in last_readings:
        node_zeroes[id] = last_readings[id]

def calibrate(id):
    print(id)
    showinfo(title="Begin calibration", message="Empty this node, then press OK.")
    # Zero first
    zero = last_readings[id]
    actual_weight = askfloat(f"Calibrate node ${id}", "Add something, then enter its weight:")
    node_calibs[id] = actual_weight / (last_readings[id] - zero)
    with open("calib.pkl", "wb") as calib_file:
        pickle.dump(node_calibs, calib_file)
    showinfo(message="Calibrated & saved")

# Serial connection interface
connect_panel = Frame(root)
connect_panel.pack(side=TOP)
ports = []
ser = serial.Serial()
ser.baudrate = 115200

connect_label = Label(connect_panel, text="Not connected")
ports_list = Listbox(connect_panel)
ports_list.bind('<<ListboxSelect>>', port_select)
scan_button = Button(connect_panel, text="Scan ports", command=port_scan)
connect_label.pack()
ports_list.pack()
scan_button.pack()

# Data readout display
readout = Frame(root)

zero_all_button = Button(readout, text = "Zero all", command=zero_all)
zero_all_button.grid(row=0, column=0, columnspan=100)

# TODO: Learn node IDs and calibration from output node
#n_rows = 3
#n_cols = 3
#n_nodes = n_rows * n_cols
n_nodes = 9
node_zeroes = {}
node_calibs = {}
last_readings = {}
last_weights = {}
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

def draw_cup(canvas, cl, scale, unit):
    canvas.delete("all")
    cup_outline = cup_pts(scale, 0.6)
    water = cup_pts(scale, 0.9)
    canvas.create_polygon(*water, fill='blue', outline='', tags="temp")
    canvas.create_polygon(*cup_outline, fill='', outline='black', width = 2 * scale, tags="static")
    canvas.create_text(100 * scale, 60 * scale, text = str(round(cl, 1)) + unit, fill = "black", font = f"TkDefaultFont {int(24 * scale)}", tags="temp")

# Set up a panel for each node
i = 0
n_rows = 3
n_cols = 3
for r in range(n_rows):
    for c in range(n_cols):
        node_panels.append(Frame(readout))
        node_labels.append(Label(node_panels[-1], text = f"Sensor {i}"))
        node_cls.append(Canvas(node_panels[-1], width = 100, height = 45, bg='white'))
        draw_cup(node_cls[i], 0, 0.5, "")
        node_switches_calib.append(Button(node_panels[-1], text = "Calibrate", command=lambda: calibrate(i+1)))
        node_panels[i].grid(row=r+1, column=c)
        node_labels[i].pack()
        node_cls[i].pack()
        node_switches_calib[i].pack()
        i = i + 1

# Read calibration data from file
calibs_file_path = 'calib.pkl'
if os.path.exists(calibs_file_path):
    with open(calibs_file_path, 'rb') as calib_file:
        #print("Loading calibration")
        node_calibs = pickle.load(calib_file)
        #print("Calibration: ")
        #print(node_calibs)
else:
    with open(calibs_file_path, 'wb') as calib_file:
        pickle.dump(node_calibs, calib_file)

t0 = 0

#readout.pack_forget()
root.mainloop()