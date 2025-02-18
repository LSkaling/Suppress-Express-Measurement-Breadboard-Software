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
default_font.configure(family='Courier', size=16)
text_font = tkFont.nametofont("TkTextFont")
text_font.configure(family='Courier', size=16)
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
    sel = event.widget.curselection()
    if sel is None:
        return
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
n_nodes = 2
node_zeroes = []
node_calibs = []
node_zeroes.append(0)
node_calibs.append(1)

# Sub-panels for individual nodes: ID label, CL display, calibrate switch
node_panels = []
node_labels = []
node_cls = []
node_switches_calib = []

def cup_pts(scale, height):
    base_size = 50
    ratio = 0.1
    full_height = base_size * scale
    full_width = base_size * scale
    pad = base_size * ratio
    top = base_size * ratio + (1 - height) * full_height
    left = base_size * ratio
    taper = base_size * ratio
    return (
        (left + pad + taper * (1 - height), top + pad),
        (left + pad + taper, full_height - pad),
        (left + full_width - pad - taper, full_height - pad),
        (left + full_width - pad - taper * (1 - height), top + pad)
    )

def draw_cup_setup(canvas, cl, scale):
    canvas.delete("all")
    cup_outline = cup_pts(scale, 1)
    water = cup_pts(scale, 0.0)
    canvas.create_polygon(*cup_outline, fill='', outline='blue', width = 2 * scale, tags="static")
    canvas.create_polygon(*water, fill='blue', outline='', tags="temp")
    canvas.create_text(80, 30, text = str(cl), fill = "blue", font = f"TkDefaultFont {24 * scale}", tags="temp")

def draw_cup(canvas, cl, scale):
    canvas.delete("temp")
    water = cup_pts(scale, cl / 10)
    canvas.create_polygon(*water, fill='blue', outline='')
    canvas.create_text(80, 30, text = str(cl), fill = "blue", font = f"TkDefaultFont {24 * scale}")

# Set up a panel for each node
for i in range(n_nodes):
    node_panels.append(Frame(readout))
    node_labels.append(Label(node_panels[-1], text = f"Sensor {i}"))
    node_cls.append(Canvas(node_panels[-1], width = 100, height = 50, bg='white'))
    draw_cup_setup(node_cls[i], 0, 1)
    node_switches_calib.append(Button(node_panels[-1], text = "Calibrate"))
    node_panels[i].pack()
    node_labels[i].pack()
    node_cls[i].pack()
    node_switches_calib[i].pack()

def weight_to_cl(weight):
    return weight # TODO: implement

# TODO: Allow for disconnection
def update_measurement():
    T = int(1000 / 80)
    # Wait for updates from each node (not necessarily in order)
    for i in range(n_nodes):
        id = int.from_bytes(ser.read(4))
        reading = int.from_bytes(ser.read(4))
        weight = (reading * node_calibs[id]) - node_zeroes[id]

        # Update canvas to show new coverage level
        draw_cup(node_cls[id], cl=weight_to_cl(weight), scale=1)
        root.after(T, update_measurement)

readout.pack_forget()
root.mainloop()