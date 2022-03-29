"""
Python driver for the Serial Protocol

History:
--------
- Created by Tim Loh, 27.01.2022
"""

import serial
import time
import threading
from typing import *


class SerialProtocol:
    STX_INT = 2
    ETX_INT = 3
    STX = STX_INT.to_bytes(1,'big')
    ETX = ETX_INT.to_bytes(1,'big')
    # Command Identifier
    EXCLAM      = "!"
    QUESTION    = "?"
    COLON       = ":"

    def __init__(self, port, baud = 115200, timeout = 0.4):

        self.ressourceLock = threading.Lock()

        self.device = serial.Serial(port=port, baudrate=baud, timeout=timeout)

    def _decode_message(self,message,operator):
        responsesplit1 = message.split(operator.encode("ASCII"), -1)
        resp0 = responsesplit1[0]
        resp1 = responsesplit1[1]
        responsesplit2 = resp0.split(self.STX, -1)
        responsesplit3 = resp1.split(self.ETX, -1)
        resp_nbr = responsesplit2[1]
        resp_val = responsesplit3[0]
        return resp_nbr, resp_val
        
    def _sendmessage(self,parameters):
        if len(parameters) == 1:
            data = "{}".format(parameters[0])
        elif len(parameters) == 2:
                data = "{}{}".format(parameters[0],parameters[1])
        else:
            data = "{}{}{}".format(parameters[0], parameters[1], parameters[2])
        message = data.encode("ASCII")
    
    
        self.device.write(self.STX)
        self.device.write(message)
        self.device.write(self.ETX)
        
    def command(self, number, paramList : List):                                                  
        params = [number, self.COLON]

        for parameter in paramList:
            params.append(parameter)

        # Query is allowed just once at a time!
        with self.ressourceLock:
            self.device.flush()
            self._sendmessage(params)                                                     
            response = self.device.read_until(b'\x03')

        _, resp_val = self._decode_message(response, self.COLON)

        result = int(float(resp_val) + 0.5)

        if result == 0:
            return True 
        else:
            return False

    def setvalue(self, number, value):
        params = [number, self.EXCLAM, value]
        
        # Query is allowed just once at a time!
        with self.ressourceLock:
            self.device.flush()
            self._sendmessage(params)
            response = self.device.read_until(b'\x03') 

        _, resp_val = self._decode_message(response,self.EXCLAM)
        
        return float(resp_val)

    def getvalue(self, number):
        params = [number, self.QUESTION]
        
        # Query is allowed just once at a time!
        with self.ressourceLock:
            self.device.flush()
            self._sendmessage(params)
            response = self.device.read_until(b'\x03') 

        _, resp_val = self._decode_message(response,self.QUESTION)

        return float(resp_val)

        
    


