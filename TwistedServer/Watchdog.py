import netstring2, socket
import struct, random

host = 'localhost'
portCryo = 44444
portHeater = 55555
size = 10
CRLF = "\r\n"




def AskDevice(command, host, port, expectreply = True):
    cmd_netstringed = netstring2.dumps(str(command))
    #print cmd_netstringed
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
    

def WatchdogSweep():
    
    CryoTC = float(AskDevice("CTC",host,portCryo))
    HeaterPower = float(AskDevice("HPW",host,portHeater))
    
    
    print CryoTC, HeaterPower



if __name__ == '__main__':
    WatchdogSweep()
    #print getTemperature()
