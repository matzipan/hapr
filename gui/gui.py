#!/usr/bin/env python

# Author: Zisu Andrei
# It uses the pyGTK library, and pySerial library. It uses a GTK Glade XML file to describe the layout

from gi.repository import Gtk
import serial
import os.path
import io

# Serial communication API
class Api:
	connector = None
	running = False

	def sendMessage(self, message):
		#@improvement: print debug messages 
		print("Sending "+ repr(message.ljust(16,'0')))

		buf = ""

		self.connector.write(message.ljust(16,'0'))
		self.connector.flush() 

		recieved = self.connector.readline()

		print("Recieved "+ repr(recieved))


		return recieved
		
	def connect(self):
		if self.connector == None: 
			self.connector = serial.Serial(port='/dev/ttyACM0', baudrate=9600, timeout=0.1)
			
		print("Initialized")

		self.connector.open()

		if self.connector.isOpen() and self.sendMessage("0") == "Noop": #send noop command
			print("Connected")
			return True
		else:
			print("Error")
			return False

	def halt(self):
		if not self.isConnected(): 
			return False

		if self.sendMessage("h") == "Halt":
			print("Halted")
			self.running = False
			return True
		else:
			print("Error")
			return False

	def disconnect(self):
		if not self.isConnected(): 
			return False

		self.connector.close()

		if self.connector.isOpen():
			print("Error")
			return False
		else: 
			print("Disconnected")
			return True

		return 

	def setFilters(self, filters):
		if not self.isConnected():
			return False

		if self.sendMessage("a"+chr(len(filters))) != "Apply":
			print("Error")
			return False

		for filter in filters:
			paramsBuf = ""

			# generate the string of chars simbolizing the parameters 
			if filter[1] != "":
				split = filter[1].split(",")[0:4] # only use the first 4
				for element in split:
					paramsBuf += chr(int(element))

			outputBuf = ""

			# generate the string of chars simbolizing the outputs 
			if filter[2] != "":
				split = filter[2].split(",")[0:2] # only use the first 2
				for element in split:
					outputBuf += chr(int(element))
				outputBuf = outputBuf.ljust(2, "0")

			if self.sendMessage("f"+chr(filter[0])+chr(filter[3])+outputBuf+paramsBuf) != "Filter":
				print("Error")
				return False

		if self.sendMessage("a") == "End filters":
			print("Filters set")
			self.running = True
			return True
		else:
			print("Error")
			return False

	def setFrequency(self, frequency):
		if not self.isConnected():
			return False

		if frequency < 1 or frequency > 44000:
			print("Error")
			return False

		if self.sendMessage("s"+str(frequency)+chr(0)) == "Set":
			print("Frequency set")
			return True
		else:
			print("Error")
			return False

	def getFrequency(self):
		if not self.isConnected():
			return False

		value = self.sendMessage("g")

		if value.startswith("Get:"):
			print("Frequency fetched "+value[4:]) #skip the "get:" prefix
			return int(value[4:]) #skip the "get:" prefix
		else: 
			print("Error")
			return False

	def download(self):
		if not self.isConnected():
			return False

		value = self.sendMessage("d")

		if value.startswith("Download:"):
			filters= []
			print("Filters count "+value[9:]) #skip the "download:" prefix
			count = int(value[9:])  #skip the "download:" prefix
			for i in range(count):
				x = []

				value = self.sendMessage("d")
				if value.startswith("Download:"):
					print("Filter recieved: "+ value[9:]) #skip the "download:" prefix
					
				for j, val in enumerate(value[9:]): #skip the "download:" prefix
					x.append(ord(val)-32) #decrease by 32 because pyhton doesn't like special characters (0-31) and we're avoiding them

				filters.append(x)

			return filters
		else:
			print("Error")
			return False

	def isConnected(self): 
		if self.connector != None and self.connector.isOpen():
			return True
		else:
			print("Not connected")
			self.running = False
			return False

	def isRunning(self):
		return self.running

	def load(self, block):
		if not self.isConnected():
			return False

		if self.sendMessage("z"+chr(block)) == "Loaded":
			return True

		return False

	def save(self, block):
		if not self.isConnected():
			return False

		if self.sendMessage("x"+chr(block)) == "Saved":
			return True

		return False


#@improvement continuously check for connection state

# Event handler class
class Handler:
	api = None
	builder = None
	availableIter = None # selection from the available filters view
	chosenIter = None # selection from the chosen filters view
	chosenMaxId = 2
	chosenModel = None
	availableModel = None

	def __init__(self, builder, api): 
		self.api = api

		self.chosenModel = builder.get_object("chosenfilterstore")
		self.availableModel = builder.get_object("availablefilterstore")

		self.builder = builder

	def deleteWindow(self, *args):
		self.api.disconnect()

		Gtk.main_quit(*args)

	def addFilter(self, *args):
		if self.availableIter != None: 
			#name, type id, properties, output, unique id, editable
			self.chosenModel.append([self.availableModel[self.availableIter][0], self.availableModel[self.availableIter][1], "0,0,0,0", "0,0", self.chosenMaxId, True])
			self.chosenMaxId += 1

	def removeFilter(self, *args):
		if self.chosenIter != None and self.chosenModel[self.chosenIter][1] != 0 and self.chosenModel[self.chosenIter][1] != 1:
			self.chosenModel.remove(self.chosenIter)

	def connect(self, button):
		if self.api.connect():
			button.set_sensitive(False)
			self.builder.get_object("buttondisconnect").set_sensitive(True)
			self.builder.get_object("buttonsetfrequency").set_sensitive(True)
			self.builder.get_object("buttonapplybegin").set_sensitive(True)
			self.builder.get_object("buttonstop").set_sensitive(True)
			self.builder.get_object("buttondownload").set_sensitive(True)
			self.builder.get_object("savebutton1").set_sensitive(True)
			self.builder.get_object("loadbutton1").set_sensitive(True)

	def disconnect(self, button):
		if self.api.disconnect():
			self.builder.get_object("buttonconnect").set_sensitive(True)
			button.set_sensitive(False)
			self.builder.get_object("buttonsetfrequency").set_sensitive(False)
			self.builder.get_object("buttonapplybegin").set_sensitive(False)
			self.builder.get_object("buttonstop").set_sensitive(False)
			self.builder.get_object("buttondownload").set_sensitive(False)
			self.builder.get_object("savebutton1").set_sensitive(False)
			self.builder.get_object("loadbutton1").set_sensitive(False)

	def stop(self, *args):
		self.api.halt()

	def applyFilters(self, *args):
		#@improvement: check if there is a path from input to output 
		processedList = []

		for filter in self.chosenModel:
			# filter type, filter properties, filter output, filter id
			processedList.append([filter[1], filter[2], filter[3], filter[4]])

		self.api.setFilters(processedList)

	def availableSelectionChanged(self,selection): 
		(model, treeiter) = selection.get_selected()

		self.availableIter = treeiter
    	

	def chosenSelectionChanged(self,selection):
		(model, treeiter) = selection.get_selected()

		self.chosenIter = treeiter

	def propertiesEdited(self, widget, path, new_text):
		self.chosenModel[path][2] = new_text


	def outputEdited(self, widget, path, new_text):
		self.chosenModel[path][3] = str(new_text)

	def setFrequencyClicked(self, *args):
		frequency = self.api.getFrequency()

		if frequency != False: 
			self.builder.get_object("frequencyinput").set_text(str(frequency))
		
			self.builder.get_object("frequencyBox").set_visible(True)


	def deleteFrequencyWindow(self, *args):
		self.builder.get_object("frequencyBox").set_visible(False)

		# this inhibits the propagation of the delete event,
		# thus not deleting the elements inside the window
		# which, otherwise, seems to be the default behaviour of GTK3
		return True

	def applyFrequency(self, *args):
		value = self.builder.get_object("frequencyinput").get_text().strip()
		if value != "":
			value = int(value)

			if self.api.setFrequency(value):
				self.deleteFrequencyWindow()


	def download(self, *args):
		self.chosenModel.clear()

		downloadedFilters = self.api.download()
		if downloadedFilters == False:
			return

		for filter in downloadedFilters:
			#name, type id, properties, output, unique id, editable
			self.chosenModel.append([
				filters[filter[0]],

				filter[0],

				str(filter[4])+","+
				str(filter[5])+","+
				str(filter[6])+","+
				str(filter[7]),

				str(filter[2])+","+
				str(filter[3]),

				filter[1],

				True
			])

	def loadBlock(self, *args):
		value = self.builder.get_object("blockloadinput").get_text().strip()

		if value != "":
			value = int(value)

			if self.api.load(value):
				self.builder.get_object("loadBox").set_visible(False)

				

	def deleteBlockLoadWindow(self, *args):
		self.builder.get_object("loadBox").set_visible(False)

		# this inhibits the propagation of the delete event,
		# thus not deleting the elements inside the window
		# which, otherwise, seems to be the default behaviour of GTK3
		return True

	def saveBlock(self, *args):
		value = self.builder.get_object("blocksaveinput").get_text().strip()

		if value != "":
			value = int(value)

			if self.api.save(value):
				self.builder.get_object("saveBox").set_visible(False)

	def deleteBlockSaveWindow(self, *args): 
		self.builder.get_object("saveBox").set_visible(False)

		# this inhibits the propagation of the delete event,
		# thus not deleting the elements inside the window
		# which, otherwise, seems to be the default behaviour of GTK3
		return True

	def openSaveBlockWindow(self, *args):
		self.builder.get_object("saveBox").set_visible(True)

	def openLoadBlockWindow(self, *args): 
		self.builder.get_object("loadBox").set_visible(True)

builder = Gtk.Builder()
builder.add_from_file("gui.glade")

handler = Handler(builder,Api())
builder.connect_signals(handler)

availableFilters = builder.get_object("availablefilters")
#the selection changed event cannot be registered from within Glade, so register it programatically
availableFilters.get_selection().connect("changed", handler.availableSelectionChanged)

chosenFilters = builder.get_object("chosenfilters")
#the selection changed event cannot be registered from within Glade, so register it programatically
chosenFilters.get_selection().connect("changed", handler.chosenSelectionChanged)

filters = [
	"Input",
	"Output",
	"Passthrough",
	"Zero",
	"Max",
	"Min",
	"Sine Generator",
	"Reverb",
	"Delay",
	"Mix",
	"Tremolo",
	"Flange",
	"Upward Compressor",
	"Downward Compressor",
	"N-bits Quantizer",
	"Distortion",
	"Triangle Generator",
	"Noise Reduction",
	"AEC Low Pass",
	"AEC High Pass",
	"AEC All Pass"
]

availableModel = builder.get_object("availablefilterstore")

for index, value in enumerate(filters[2:]): #skip input and output filters
	availableModel.append([value, index+2])

chosenModel = builder.get_object("chosenfilterstore")
#name, typeid, properties, output, uniqueid, editable
chosenModel.append(["Input", 0, "0,0,0,0", "1,0", 0, True])
chosenModel.append(["Output", 1, "0,0,0,0", "0,0", 1, True])

window = builder.get_object("mainWindow")
window.show_all()

Gtk.main()
