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

class Datatype(Enum):
    DTYPE_UINT8    = ('B',1)
    DTYPE_INT8     = ('b',1)
    DTYPE_UINT16   = ('H',2)
    DTYPE_INT16    = ('h',2)
    DTYPE_UINT32   = ('L',4)
    DTYPE_INT32    = ('l',4)
    DTYPE_F32      = ('f',4)


class Response:
    def __init__(self):
        self.number             : Optional[int]                     = None
        self.responseDesignator : Optional[str]                     = None
        self.dataLength         : Optional[int]                     = None
        self.dataArray          : Optional[List[Union[float, int]]] = None
        self.upstreamData       : Optional[bytearray]               = None

class Command:
    def __init__(self):
        self.number         : Optional[int]                     = None
        self.commandID      : Optional[CommandID]               = None
        self.dataArray      : Optional[Iterable[float, int]]    = None
        self.datatypeArray  : Optional[Iterable[Datatype]]      = None

class Parameter:

    def __init__(self, number : int, type : Datatype, description : Optional[str] = None):
        self.description    : Optional[str] = description
        self.number         : int           = number
        self.type           : Datatype      = type

class Function:

    def __init__(self, number : int,  argTypeList : Iterable[Union[Datatype, None]] = None, returnTypeList : Iterable[Union[Datatype, None]] = None, description : Optional[str] = None):
        self.description    : Optional[str]                     = description
        self.number         : int                               = number
        self.argTypeList    : Optional[Iterable[Datatype]]      = argTypeList
        self.returnTypeList : Optional[Iterable[Datatype]]      = returnTypeList

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

        self.device = serial.Serial(port=port, baudrate=baud, timeout=timeout)
        # serial module needs settling time...
        time.sleep(1)

        self.numberFormat = numberFormat
        self.maxPacketSize = maxPacketSize

    #==============================================================================
    def _decode(self, msg : bytearray, cmdID : CommandID, ongoing : bool = False) -> Response:
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
        if (cmdID.name) != 'UPSTREAM' and ongoing == False:
            rsp.responseDesignator = msgDat[0]
        elif cmdID.name == 'UPSTREAM':
            # Encode data into bytearray
            rsp.upstreamData = bytearray.fromhex(msgDat[0])
            rsp.dataLength = 0

        if cmdID.name == 'COMMAND':
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

            # If there is a message distributed over several packages
            elif ongoing:
                datStrArr = msgDat[0].split(',')
                if self.numberFormat.name == 'HEX':
                    rsp.dataArray = [int(data, 16) for data in datStrArr]
                else: # number format is set to float
                    rsp.dataArray = [float(data) for data in datStrArr]

        elif cmdID.name == 'GETVAR':
            # Data Transfer and Upstream
            if len(msgDat) > 1:
                if self.numberFormat.name == 'HEX':
                    rsp.dataArray = [int(msgDat[1], 16)]
                else: # number format is set to float
                    rsp.dataArray = [float(msgDat[1])]

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

            if command.commandID is not None and command.datatypeArray is not None:

                # if not hasattr(command.dataArray, '__iter__') or not hasattr(command.datatypeArray, '__iter__'):
                #     raise ValueError('command.dataArray and command.datatypeArray must be iterables.')
                
                formatArray = f'{"".join(type.value[0] for type in command.datatypeArray)}'
                byteStringArray = [struct.pack(f'>{formatItem}', dataItem).hex().upper().lstrip('0') for formatItem, dataItem in zip(formatArray, command.dataArray)]

            if byteStringArray is not None:
                packet = f'{num}{command.commandID.value}{",".join(item for item in byteStringArray)}'
            else:
                packet = f'{num}{command.commandID.value}'
            
        else: # number format is set to float
            floatStringArray = None

            num = f'{int(command.number)}'

            if command.commandID is not None and command.datatypeArray is not None:

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
    
    def _reinterpretDecodedIntToDtype (self, decoded : int, type : Datatype) -> Union[float, int]:
        byteLength = type.value[1]
        intArr = struct.pack(f'>I', decoded)
        intArr = intArr[-byteLength:]
        return struct.unpack(f'>{type.value[0]}', intArr)[0]

    
    #==============================================================================
    def command(self, function : Function, paramList : Iterable[Union[float, int, None]] = None) -> List[Union[float, int]]:
        """
        Send a command to the SCI device
        """

        if paramList is not None:
            if len(paramList) != len(function.argTypeList):
                raise Exception('Length of parameter list does not match the length of the type specifier list.')

        # Construct command
        cmd = Command()
        cmd.number      = function.number
        cmd.commandID   = CommandID.COMMAND
        cmd.dataArray   = paramList
        cmd.datatypeArray  = function.argTypeList

        sendOnce = True
        ongoing = False
        data = []
        

        # Query is allowed just once at a time!
        with self.ressourceLock:
            
            while (len(data) < len(function.returnTypeList) or sendOnce):
                packet = self._encode(cmd)
                self.device.flush()
                self._send(packet)
                response = self.device.read_until(b'\x03')
                if len(response) == 0:
                    raise Exception('COMMAND - Timeout occured')
                rsp = self._decode(bytearray(response), cmd.commandID, ongoing)

                # This command does not need further processing
                if rsp.responseDesignator == 'ACK' or rsp.responseDesignator == 'UPS':
                    break
                elif rsp.responseDesignator == 'ERR':
                    raise Exception(f'COMMAND - Error: {rsp.dataArray[0]}')
                elif rsp.responseDesignator == 'NAK':
                    raise Exception('COMMAND - Unknown Command')
                # elif rsp.responseDesignator == 'DAT':
                #     expectedDatalen = rsp.dataLength
                
                # Here we also land if the response designator is None
                data.extend(rsp.dataArray.copy())
                ongoing = True
                sendOnce = False
                # Sleep time necessary for reliable data transmission
                time.sleep(0.001)
        
            # Type conversion
            if len(data) > 0:
                if self.numberFormat.name == 'HEX':
                    data = [self._reinterpretDecodedIntToDtype(dat, type) for dat, type in zip(data, function.returnTypeList)]
                else:
                    data = [dat if function.returnTypeList[i].name == 'DTYPE_F32' else int(dat) for dat, i in zip(data, range(len(function.returnTypeList)))]

        return data

    #==============================================================================
    def setvalue(self, parameter : Parameter, value : Union[float, int]):
        """
        Sets a variable of the variable struct
        """

        # Construct command
        cmd = Command()
        cmd.number      = parameter.number
        cmd.commandID   = CommandID.SETVAR
        cmd.dataArray   = [value]
        cmd.datatypeArray  = [parameter.type]
        response        = None
        
        # Query is allowed just once at a time!
        with self.ressourceLock:
            packet = self._encode(cmd)
            self.device.flush()
            self._send(packet)
            response = self.device.read_until(b'\x03')

        if len(response) == 0:
            raise Exception('COMMAND - Timeout occured')
        rsp = self._decode(bytearray(response), cmd.commandID)

        if rsp.responseDesignator == 'ACK':
            return
        elif rsp.responseDesignator == 'ERR':
            raise Exception(f'SETVAR - Error: {rsp.dataArray[0]}')
        elif rsp.responseDesignator == 'NAK':
            raise Exception('SETVAR - Variable unknown')


    def getvalue(self, parameter : Parameter) -> Union[float,int]:
        """
        gets a variable value of the variable struct
        """
        
        # Construct command
        cmd = Command()
        cmd.number      = parameter.number
        cmd.commandID   = CommandID.GETVAR
        response        = None
        
        # Query is allowed just once at a time!
        with self.ressourceLock:
            packet = self._encode(cmd)
            self.device.flush()
            self._send(packet)
            response = self.device.read_until(b'\x03')

        if len(response) == 0:
            raise Exception('COMMAND - Timeout occured')

        rsp = self._decode(bytearray(response), cmd.commandID)

        if rsp.responseDesignator == 'ACK':
            return self._reinterpretDecodedIntToDtype(rsp.dataArray[0], parameter.type)
        elif rsp.responseDesignator == 'ERR':
            raise Exception(f'SETVAR - Error: {rsp.dataArray[0]}')
        elif rsp.responseDesignator == 'NAK':
            raise Exception('SETVAR - Variable unknown')
        
    


