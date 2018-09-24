import netstring2, socket
import struct, random

host = 'localhost'
port = 44444
size = 10
CRLF = "\r\n"
send_command_prefix = "SendToArduino."

def geyserCMD(command, expectreply = True):
    cmd_netstringed = netstring2.dumps(str(command))
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((host,port))
    s.send(cmd_netstringed)
    response = None
    if expectreply:
        data = s.recv(15)
        response = netstring2.loads(data)
    else:
        response = "OK"
    s.close()
    return response
    
def getTemperature(whichTC = 0):
    send_string = "CurrentTemperature.%s" % whichTC
    #print send_string
    #return geyserCMD("CurrentTemperature.0", expectreply=True)
    return geyserCMD(send_string)

def getPressure():
    return float(geyserCMD("CurrentPressure.0"))

def getcFP():
    return float(geyserCMD("ReadcFP.0"))


#We expect the heating controls to be a value between 1 and 100
#like a scale of output heat. So, assuming that, we will go forward
def HeaterControl(set_heating_value):
    #first, we scale the heating for our PWM, that is 0 - 255
    pwm_heating=int(set_heating_value*255.0/100.0)
    #now form the send command!
    send_command = send_command_prefix+"q"+str(pwm_heating)+"a"
    geyserCMD(send_command, expectreply = False)


def ChangePelt1(peltSYSID, set_temp_value):
    #Lets try this shall we?
    #
    StartCmd = "FF"
    #
    s = struct.pack('>f', set_temp_value)  #change > to < for little endian
    floatHex = ''.join('%.2x' % ord(c) for c in s)
    ##
    random_qryID = random.randint(0, 4096)
    random_ID = '%.04X' % random_qryID
    ##
    peltSYSIDfmt = '%.04d' % peltSYSID
    ##
    OpsByteCode = "0001"
    ##
    send_command = send_command_prefix+StartCmd+random_ID+OpsByteCode+peltSYSIDfmt+floatHex+"0A"
    geyserCMD(send_command, expectreply = False)

