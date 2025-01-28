import serial
from tkinter import *
from tkinter.simpledialog import askfloat
import tkinter.font as tkFont

# Initial interface setup
root = Tk()
root.geometry("400x200")
root.title("Measurement Station")
default_font = tkFont.nametofont("TkDefaultFont")
default_font.configure(family='Courier', size=18)
text_font = tkFont.nametofont("TkTextFont")
text_font.configure(family='Courier', size=18)
frame = Frame(root)
frame.pack()

tray = Frame(root)
tray.pack(side=TOP)

pad_x = 10
pad_y = 10

# Serial start (TODO: take address as program argument)
ser = serial.Serial('/dev/tty.usbmodem155895701')

# Sensor stuff
class Sensor:
    zero = 0
    calib = 1
    reading = 0
    weight = 0
    T = int(1000 / 80) # sampling interval

    def tare(self):
        if self.calib != 1: # Zero in grams
            self.zero = self.weight

        if self.calib == 1: # Zero in arbitrary units
            self.zero = self.reading

    def calibrate(self):
        self.calib = askfloat("Calibrate sensor", "Current weight on sensor") / self.weight
        self.zero = 0

    def update_measurement(self):
        bytes = ser.readline()
        bytes = bytes.decode("utf-8")
        self.reading = float(str(bytes[0:-2])) # strip newlines (CRLF) and convert
        self.weight = (self.reading * self.calib) - self.zero
        txt = "Weight: " + "{:.1f}".format(self.weight) + "g"
        measurement.config(text = txt)
        root.after(self.T, self.update_measurement)

# Single sensor display
sens0 = Sensor()
button1 = Button(tray, text = "Zero", command = sens0.tare)
button1.pack(padx = pad_x, pady = pad_y)
button2 = Button(tray, text = "Calibrate", command = sens0.calibrate)
button2.pack(padx = pad_x, pady = pad_y)

measurement = Label(frame)
measurement.pack()

# Start loop and update callbacks
sens0.update_measurement()
root.mainloop()