"""
Python driver for the Serial Communication Interface

History:
--------
- Created by Tim Loh, 27.01.2022
- Updated by Holderried Roman for SCI functionality, 29.03.2022
"""

import serial
import struct
import time
from enum import Enum
import threading
from typing import *

class NumberFormat(Enum):
    HEX = 1
    FLOAT = 2

class CommandID(Enum):
    REJECTED    = '#'
    GETVAR      = '?'
    SETVAR      = '!'
    COMMAND     = ':'
    UPSTREAM    = '>'
    DOWNSTREAM  = '<'

class Dataformat(Enum):
    DTYPE_UINT8    = 'B'
    DTYPE_INT8     = 'b'
    DTYPE_UINT16   = 'H'
    DTYPE_INT16    = 'h'
    DTYPE_UINT32   = 'L'
    DTYPE_INT32    = 'l'
    DTYPE_F32      = 'f'

class Response:
    def __init__(self):
        self.number = None
        self.responseDesignator = None
        self.dataLength = None
        self.dataArray = None
        self.upstreamData = None

class Command:
    def __init__(self):
        self.number     = None
        self.commandID  = None
        self.dataArray  = None
        self.dataFormat = None

class SCI:
    STX_INT = 2
    ETX_INT = 3
    STX = STX_INT.to_bytes(1,'big')
    ETX = ETX_INT.to_bytes(1,'big')

    # Command Identifier
    #COMMAND_ID = {'REJECTED' : '#', 'GETVAR' : '?', 'SETVAR' : '!', 'COMMAND' : ':', 'UPSTREAM' : '>', 'DOWNSTREAM' : '<'}
    # EXCLAM      = "!"
    # QUESTION    = "?"
    # COLON       = ":"
    #==============================================================================
    def __init__(self, port, maxPacketSize, baud : int = 115200, timeout : float = 0.4, numberFormat : NumberFormat = NumberFormat.HEX):

        self.ressourceLock = threading.Lock()

        #self.device = serial.Serial(port=port, baudrate=baud, timeout=timeout)

        self.numberFormat = numberFormat
        self.maxPacketSize = maxPacketSize

    #==============================================================================
    def _decode(self, msg : bytearray, cmdID : CommandID) -> Response:
        """
        Message decoder

        Parameters:
        -----------
        - msg   : Received data line (STX -> ETX)
        - cmdID : Expected command identifier
        
        Returns:
        --------
        - Response details
        """

        rsp = Response()

        if (msg[0] != self.STX or msg[-1] != self.ETX):
            raise Exception(f'MESSAGE DECODE: STX / ETX error.')

        # Remove STX and ETX
        msg.pop(0)
        msg.pop(-1)

        msgStr = msg.decode()

        try:
            splitted = msgStr.split(cmdID.value())
        except Exception as e:
            raise ValueError(f'MESSAGE DECODE: Wrong message format - {e}')

        if self.numberFormat.name() == 'HEX':
            rsp.number = int(splitted[0], 16)
        else: # number format is set to float
            rsp.number = int(float(splitted[0]) + 0.5)

        # Data section
        msgDat = splitted[1]

        # Special case: Message is an upstream, so there is no response designator
        if (cmdID.name) != 'UPSTREAM':
            rsp.responseDesignator = msgDat.split(';')[0]
        else:
            # Encode data into bytearray
            rsp.upstreamData = bytearray.fromhex(msgDat)

        # Data transfer
        if len(msgDat) > 2:
            datStrArr = msgDat[2].split(',')
            if self.numberFormat.name() == 'HEX':
                rsp.dataArray = [int(data, 16) for data in datStrArr]
            else: # number format is set to float
                rsp.dataArray = [float(data) for data in datStrArr]
        
        # Data Transfer and Upstream
        if len(msgDat) > 1:
            if self.numberFormat.name() == 'HEX':
                rsp.dataLength = int(msgDat[1], 16)
            else: # number format is set to float
                rsp.dataLength = int(float(msgDat[1]) + 0.5)

        return rsp
    
    #==============================================================================
    def _encode(self, command : Command) -> bytearray:
        """
        Encodes the message into a bytearray SCI packet (ready to send)

        Parameters:
        -----------
        - command: Command details

        Returns:
        - data bytearray to be sent
        """
        
        num = None
        dataStr = None

        if self.numberFormat.name == 'HEX':
            byteStringArray = None

            num = struct.pack(f'>L', command.number).hex().upper().lstrip('0')

            if not isinstance(command.commandID, CommandID):
                raise ValueError('command.commandID must be an instance of the CommandID class.')

            if command.commandID is not None and command.dataFormat is not None:

                if not hasattr(command.dataArray, '__iter__') or not hasattr(command.dataFormat, '__iter__'):
                    raise ValueError('command.dataArray and command.dataFormat must be iterables.')
                
                formatArray = f'{"".join(form.value for form in command.dataFormat)}'
                byteStringArray = [struct.pack(f'>{formatItem}', dataItem).hex().upper().lstrip('0') for formatItem, dataItem in zip(formatArray, command.dataArray)]

            if byteStringArray is not None:
                dataStr = f'{num}{command.commandID.value}{",".join(item for item in byteStringArray)}'
            else:
                dataStr = f'{num}{command.commandID.value}'
            
        else: # number format is set to float
            floatStringArray = None

            num = f'{int(command.number)}'

            if command.commandID is not None and command.dataFormat is not None:

                if not hasattr(command.dataArray, '__iter__'):
                    raise ValueError('command.dataArray must be iterable.')

                floatStringArray = [f'{str(float(item)).rstrip("0")}' if isinstance(item, float) else f'{str(int(item))}' for item in command.dataArray]

            if floatStringArray is not None:
                dataStr = f'{num}{command.commandID.value}{",".join(item for item in floatStringArray)}'
            else:
                dataStr = f'{num}{command.commandID.value}'


        return dataStr.encode("ASCII")
    
        # self.device.write(self.STX)
        # self.device.write(message)
        # self.device.write(self.ETX)

    #==============================================================================
    def _send(self, packet : bytearray):
        
        if len(packet) > self.maxPacketSize:
            raise Exception(f'Size of packet too big: Packet size: {len(packet)}; Max size: {len(self.maxPacketSize)}.')
        
        self.device.write(self.STX)
        self.device.write(packet)
        self.device.write(self.ETX)
    
    #==============================================================================
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

    #==============================================================================
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

        
    


