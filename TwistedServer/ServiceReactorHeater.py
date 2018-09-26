#!/usr/bin/env python

#This will need tons of documentation, but we will try to address all of it
#as we go and in documents we write later.
#
#This is the Service Module / Server for the geyser hardware control.
#It is written in TwistedMatrix for Python, a framework for ASYNC programming.
#
#CAUTION: DO NOT USING BLOCKING METHODS IN THIS! The AsyncReactor expects
#NON-BLOCKING commands STRICTLY. Refrain from using things like:
#
#time.sleep(t)
#
#These commands will block the reactor, and the async program will malfunction!

#we need the reactor from twisted.internet, and the protocol
#factory for our server.
from twisted.internet import reactor, protocol, task, threads, defer
from twisted.python.failure import Failure

#serial port implementation
from twisted.internet.serialport import SerialPort

#Twisted basic protocol implementations
from twisted.protocols.basic import LineReceiver, NetstringReceiver


#module needed to convert between hex and ascii for me to see whats going on.
import serial



#serial port definition
SerialPortAddress = '/dev/ttyUSB1'

#This is the protocol which defines how to "talk" to the cryocooler control board
#running the cryocooler. It is very simple, and constructed
#from twisted.protocols.basic
class CryoProtocol(LineReceiver):
    
    #first, since this is the thing that sits right behind the transport,
    #we would want it to get a callback associated with it, so when it gets
    #data, it "calls home" with it.
    def __init__(self, callback):
        self.callback = callback
        self.delimiter = '\r\n'
        
    #This is what is does when connection is established. Right now,
    #sitting here for debugging reasons
    def connectionMade(self):
        print 'Connected to LakeShore325'
        #If Raw mode is needed, uncomment:
        #self.setRawMode()
        
    #this defines what to do, when you want to send a line.
    #it just sends the line ot the transport WITH delimiter "\r"
    def sendLine(self, line):
        #print "Transport Start, sending: ", line
        self.transport.write(line)
    
    #this defines what to do when transport gives some data which was received
    #it essentially "calls home" to tell about it!
    def lineReceived(self, line):
        self.callback(line)
        
    #this defines what to do when transport gives some raw data which was received
    #it essentially "calls home" to tell about it!
    #def rawDataReceived(self, rawData):
    #    self.callback(rawData)
    
        
    #What to to when connection is lost or closed. If in future, you want to
    #issue a recovery method for serial disconnection, use this
    def connectionLost(self, reason):
        print "Connection lost, reason: ", repr(reason)
########################################################

#This is the protocol, which talks over the internet
#to other threads, requesting info. We use netstrings, as they are a viable
#way to communicate between the server and client. Our protocol is simple,
#instead of using AMP or Perspective Broker, we can use this simple but
#genius RPC like method from krondo. (Hell, one day for giggles I want to write
#this whole thing in OSCAR bwahaha)


class OutboundProtocol(NetstringReceiver):
    
    #this is what we do when we receive strings.
    def stringReceived(self, request):
        self.processRequest(request)
        #check for the "." in te string
        #if '.' not in request:
        #    self.transport.loseConnection("Bad Request")
        #    return
        #separate the "." and the 2 parts and send it for processing
        #request_name , request_value = request.split('.', 1)
        #self.processRequest(request_name, request_value)
    
    #You might be wondering, why we are doing this redundant step. We could
    #have sent the request directly to the factory! Yes, we could. But
    #the work of the protocol is to RESPOND as well. 
    def processRequest(self, request):
        responseFromProcess = self.factory.processRequest(request)
        #if there is a response, send it to transport.
        if responseFromProcess is not None:
            self.sendString(responseFromProcess)
        #and close the connection
        self.transport.loseConnection()
    
        
    #def connectionMade(self):
    #    self.transport.write(str(self.factory.temperature))
    #    self.transport.loseConnection()

#This is the service which writes data to the board
class GeyserService(object):
    
    def WriteToDevice(self, Device, what):
        reactor.callWhenRunning(self.Device.sendLine, what)

#This is the protocol factory, which constructs the outbound service.
#Note, the hardware bound service "calls back" here to report
#on new variables coming in, so this is where the data is stored.

class OutboundFactory(protocol.ServerFactory):
    
    #specify the internet side protocol
    protocol = OutboundProtocol
    
    #initializer, manages memory and hardware device callbacks
    def __init__(self, SerialPortAddress, reactor):
        
        #allocate space for temperature   
        self.PW = None
        
        self._SPECIFIC = False
        self._SPCMD = None

        
        #Assign a device i.e. the hardware
        self.Device = CryoProtocol(self.AssignFromCallback)
        
        #assign a transport to the hardware i.e. serial
        self.transport = SerialPort(self.Device, SerialPortAddress, reactor, baudrate=9600, bytesize=serial.SEVENBITS, parity=serial.PARITY_ODD, stopbits=serial.STOPBITS_ONE, xonxoff=1, rtscts=0)
        
        
        #Dictionary of handlers based on dispatcher output
        self.ExpectedCB = ""
        self.ExpectingReply = False
        self.dispatchDictionary = {"PW": self._handlerPW, "PID": self._handlerPI}
        
        
        
        
        #####################
        #Looping calls. This is here so in future we can implement a safety
        #feature, where we can "poll" the hardware to tell it that we
        #are still talking and something didnt go wrong
        l = task.LoopingCall(self.PrintDataOnScreen)
        l.start(1.0)
        r = task.LoopingCall(self.CurrentPW)
        r.start(2.0)
        
        #####################
    
    
    #this function used to assign values from callback to memory
    #Now this function prepars the data and launches helper dispatchers
    #and dispatches based on the PacketIdentifier.
    def AssignFromCallback(self, data):
        _msgReply=self._dispatcherHelper(data)
        _msgReply.addCallback(self._PassFailState, True)
        _msgReply.addErrback(self._PassFailState, False, data)
        
       
    #What to do in case there is a failure mode? (Let the user know
    #and continue as normal)
    def _PassFailState(self, state, CBEB, data=None):
        if CBEB == False:
            print "The message was of unknown type and was not handled"
            if data != None:
                print data
                
            
    #Dispatcher helper. Identifies incoming packets
    #based on the packet identifying bit
    #and launches handlers based on what kind of packet was received.
    def _dispatcherHelper(self, data):
        
        D = defer.Deferred()
        
        
        if self.ExpectingReply==True:
            _messageIdent=self.ExpectedCB
            self.dispatchDictionary[_messageIdent](data)
            D.callback(True)
            
        else:
            _messageIdent=""
            D.errback(False)
            
        return D
    
    
    #Handler for the temperature data.
    #Basically, just split the data and store the bytes.
    def _handlerPW(self, PWData):
        #print "PW handler called"
        
        self.PW = PWData
        self.ExpectingReply=False
        
        #print self.PW
       
        
    def PrintDataOnScreen(self):
        temp_string = "Last PW "+self.PW+"Reply expected: "+self.ExpectingReply
        sys.stdout.flush() 
        sys.stdout.write('\n' + temp_string)
    
    def _handlerPI(self, PIdata):
        print PIdata
    
    #This probably handles the outbound protocol. What a mess,
    #this needs a rewrite.
    def processRequest(self, request):
        
        #RequestIdentifier - First 4 chars.
        #First char C for CryoCooler
        RDictionary = {"HPW": self.PW, "PID": self._handlerPI}
        
        
        RequestID = request[:3]
        return self.PW
        #try:
        #    return thunk(request_value)
        #except:
        #    return None # transform failed
        
    def CurrentPW(self):
        self.ExpectingReply = True
        self.ExpectedCB="PW"
        self.SendToPW("HTR? 1")
        #return self.TC
    
    
    
    def SendToPW(self, what):
        toBeSent = what+"\r\n"
        reactor.callWhenRunning(self.Device.sendLine, toBeSent)
        
    
reactor.listenTCP(55555, OutboundFactory(SerialPortAddress, reactor))
reactor.run()
    
    


