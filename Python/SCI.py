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
        self.number             : Optional[int]                     = None
        self.responseDesignator : Optional[str]                     = None
        self.dataLength         : Optional[int]                     = None
        self.dataArray          : Optional[List[Union[float, int]]] = None
        self.upstreamData       : Optional[bytearray]               = None

class Command:
    def __init__(self):
        self.number     : Optional[int]                     = None
        self.commandID  : Optional[CommandID]               = None
        self.dataArray  : Optional[Iterable[float, int]]    = None
        self.dataFormat : Optional[Iterable[Dataformat]]    = None

class SCI:
    STX = 2
    ETX = 3
    # STX = STX_INT.to_bytes(1,'big')
    # ETX = ETX_INT.to_bytes(1,'big')

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

        splitted = msgStr.split(cmdID.value)
        
        try:
            if self.numberFormat.name == 'HEX':
                rsp.number = int(splitted[0], 16)
            else: # number format is set to float
                rsp.number = int(float(splitted[0]) + 0.5)
        except Exception as e:
            raise ValueError(f'MESSAGE DECODE: Wrong message format - {e}')

        # Data section
        msgDat = splitted[1].split(';')

        # Special case: Message is an upstream, so there is no response designator
        if (cmdID.name) != 'UPSTREAM':
            rsp.responseDesignator = msgDat[0]
        else:
            # Encode data into bytearray
            rsp.upstreamData = bytearray.fromhex(msgDat[0])
            rsp.dataLength = 0

        # Data transfer
        if len(msgDat) > 2:
            datStrArr = msgDat[2].split(',')
            if self.numberFormat.name == 'HEX':
                rsp.dataArray = [int(data, 16) for data in datStrArr]
            else: # number format is set to float
                rsp.dataArray = [float(data) for data in datStrArr]
        
        # Data Transfer and Upstream
        if len(msgDat) > 1:
            if self.numberFormat.name == 'HEX':
                rsp.dataLength = int(msgDat[1], 16)
            else: # number format is set to float
                rsp.dataLength = int(float(msgDat[1]) + 0.5)
        else:
            rsp.dataLength = 0

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
        packet = None

        if self.numberFormat.name == 'HEX':
            byteStringArray = None

            num = struct.pack(f'>L', command.number).hex().upper().lstrip('0')

            # if not isinstance(command.commandID, CommandID):
            #     raise ValueError('command.commandID must be an instance of the CommandID class.')

            if command.commandID is not None and command.dataFormat is not None:

                # if not hasattr(command.dataArray, '__iter__') or not hasattr(command.dataFormat, '__iter__'):
                #     raise ValueError('command.dataArray and command.dataFormat must be iterables.')
                
                formatArray = f'{"".join(form.value for form in command.dataFormat)}'
                byteStringArray = [struct.pack(f'>{formatItem}', dataItem).hex().upper().lstrip('0') for formatItem, dataItem in zip(formatArray, command.dataArray)]

            if byteStringArray is not None:
                packet = f'{num}{command.commandID.value}{",".join(item for item in byteStringArray)}'
            else:
                packet = f'{num}{command.commandID.value}'
            
        else: # number format is set to float
            floatStringArray = None

            num = f'{int(command.number)}'

            if command.commandID is not None and command.dataFormat is not None:

                # if not hasattr(command.dataArray, '__iter__'):
                #     raise ValueError('command.dataArray must be iterable.')

                floatStringArray = [f'{str(float(item)).rstrip("0")}' if isinstance(item, float) else f'{str(int(item))}' for item in command.dataArray]

            if floatStringArray is not None:
                packet = f'{num}{command.commandID.value}{",".join(item for item in floatStringArray)}'
            else:
                packet = f'{num}{command.commandID.value}'


        return bytearray(packet,'ASCII')

    #==============================================================================
    def _send(self, packet : bytearray):
        
        if len(packet) > self.maxPacketSize:
            raise Exception(f'Size of packet too big: Packet size: {len(packet)}; Max size: {len(self.maxPacketSize)}.')
        
        packet.insert(0, self.STX)
        packet.append(self.ETX)
        self.device.write(packet)
    
    #==============================================================================
    def command(self, number : int, paramList : Iterable[Union[float, int, None]] = None, typeList : Iterable[Union[Dataformat, None]] = None) -> List[Union[float, int]]:
        """
        Send a command to the SCI device
        """

        # Construct command
        cmd = Command()
        cmd.number      = number
        cmd.commandID   = CommandID.SETVAR
        cmd.dataArray   = paramList
        cmd.dataFormat  = typeList

        firstCycleHandled = False
        expectedDatalen = 1
        data = []
        

        # Query is allowed just once at a time!
        with self.ressourceLock:
            
            while (len(data) < expectedDatalen):
                packet = self._encode(cmd)
                self.device.flush()
                self._send(packet)
                response = self.device.read_until(b'\x03')
                rsp = self._decode(response, cmd.commandID)

                # This command does not need further processing
                if rsp.responseDesignator == 'ACK' or rsp.responseDesignator == 'UPS':
                    break
                elif rsp.responseDesignator == 'ERR':
                    raise Exception(f'Command error: {rsp.dataArray[0]}')
                elif rsp.responseDesignator == 'NAK':
                    raise Exception('Command unknown')
                elif rsp.responseDesignator == 'DAT':
                    expectedDatalen = rsp.dataLength
                
                # Here we also land if the response designator is None
                data.extend(rsp.dataArray.copy())
        
        return data

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

        
    


