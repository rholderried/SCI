"""
Datalogger SCI interface

History:
--------
- Created by Holderried, Roman, 22.08.2022
"""

from xmlrpc.client import boolean
from SCI import SCI, Function, Variable, Datatype
from typing import *
from Common import Version
import importlib.util, sys
import struct


class Channel:

    def __init__(self, channel : int, variable : Variable, divider : int, recLen : int):
        self.channel    : int = channel
        self.variable   : Variable = variable
        self.divider    : int = divider
        self.recLen     : int = recLen
        self.data       : Union[int, float] = []


class Datalogger:

    MAX_NUM_LOGS = 8

    def __init__(self, SCI: SCI, index: int, maxNumBytes : int, cfgFilePath : str):
        """
        Datalogger API initializer

        Parameters:
        -----------
        - SCI           : Handle to the SCI Master
        - index         : Index of the datalogger
        - maxNumBytes   : Maximum number of bytes that the on-board buffer can hold
        - cfgFilePath   : Full Path to the datalogger configuration file (Must be named DataloggerCfg.py)
        """
        self.sciHdl         : SCI = SCI
        self.index          : int = index
        self.maxNumBytes    : int = maxNumBytes
        self.numBytesLeft   : int = maxNumBytes

        # Load the config file
        spec = importlib.util.spec_from_file_location('DataloggerCfg', cfgFilePath)
        cfg = importlib.util.module_from_spec(spec)
        sys.modules['DataloggerCfg'] = cfg
        spec.loader.exec_module(cfg)

        self.cfg = cfg

        self.functions : Dict[str, Function] = \
                         {  'GetDataloggerVersion'      : Function(cfg.CMD_NUM_GetDataloggerVersion, [Datatype.DTYPE_UINT8], [Datatype.DTYPE_UINT8, Datatype.DTYPE_UINT8, Datatype.DTYPE_UINT8], 'Get the Datalogger version in Format V{Version_Major}_{Version_Minor}_{Revision}.', False),
                            'RegisterLogFromVarStruct'  : Function(cfg.CMD_NUM_RegisterLogFromVarStruct, [Datatype.DTYPE_UINT8, Datatype.DTYPE_UINT8, Datatype.DTYPE_UINT16, Datatype.DTYPE_UINT16, Datatype.DTYPE_UINT32], [], 'Register a log channel with a variable from the variable structure.', False),
                            'InitializeDatalogger'      : Function(cfg.CMD_NUM_InitializeDatalogger, [Datatype.DTYPE_UINT8], [], 'Initialize the datalogger with the registered logger channels.', False),
                            'StartDatalogger'           : Function(cfg.CMD_NUM_StartDatalogger, [Datatype.DTYPE_UINT8], [], 'Start the initialized datalogger.',False),
                            'StopDatalogger'            : Function(cfg.CMD_NUM_StopDatalogger, [Datatype.DTYPE_UINT8], [], 'Stop the datalogger.', False),
                            'GetLogData'                : Function(cfg.CMD_NUM_GetLogData, [Datatype.DTYPE_UINT8], [], 'Initiate the data upstream.', True),
                            'GetChannelInfo'            : Function(cfg.CMD_NUM_GetChannelInfo, [Datatype.DTYPE_UINT8, Datatype.DTYPE_UINT8], [Datatype.DTYPE_UINT32, Datatype.DTYPE_UINT16, Datatype.DTYPE_UINT32, Datatype.DTYPE_UINT32, Datatype.DTYPE_UINT32], 'Return the relevant channel data.', False),
                            'ResetDatalogger'           : Function(cfg.CMD_NUM_ResetDatalogger, [Datatype.DTYPE_UINT8], [], 'Reset the data structure.', False),
                            'SetOpMode'                 : Function(cfg.CMD_NUM_SetOpMode, [Datatype.DTYPE_UINT8, Datatype.DTYPE_UINT32], [], 'Set the operation mode of the data logger.', False)}
                        
        self.baseFrequency_Hz : int     = self.sciHdl.getvalue(Variable(cfg.VAR_NUM_BaseFrequency, Datatype.DTYPE_UINT32, ''))
        self.version          : Version = Version()
        self.getVersion()


        self.chRegistered  : List[Optional[Channel]] = [None] * self.MAX_NUM_LOGS
        self.loggerInit     : boolean = False
    
    def getVersion(self):
        """
        Queries the version numbers from the datalogger.
        """
        
        resp = self.sciHdl.command(self.functions['GetDataloggerVersion'], [self.index])

        self.version.versionMajor = resp[0]
        self.version.versionMinor = resp[1]
        self.version.revision = resp[2]

    def register(self, variable : Variable, recLen : int, divider : int, channel : int = 0):
        """
        Register a channel into the logger. After registering all desired channels,
        the function initializeLogger() must be called in order to prepare the on board
        data logger.

        Parameters:
        -----------
        - variable: SCI variable (instance of class Variable) that shall get logged.
        - recLen:   Number of items to record
        - divider:  Frequency divider for this channel
        - channel:  Log channel to use (defaults to the next free channel)
        """

        if (self.functions['RegisterLogFromVarStruct'].number <= 0):
            raise Exception(f'No valid channel number for the callback RegisterLogFromVarStruct assigned.')

        freeChannels : List = list(range(1,self.MAX_NUM_LOGS + 1))

        # if len(self.chRegistered) == self.MAX_NUM_LOGS:
        #     raise Exception(f'Maximum number of channels is {self.MAX_NUM_LOGS}.')

        # Variable checks for plausibility ----------------------------------------------
        if divider <= 0:
            divider = 1

        if recLen < 0:
            recLen = 0
        # -------------------------------------------------------------------------------

        for ch in reversed(self.chRegistered):
            if ch is not None:
                freeChannels.pop(ch.channel - 1)

        if len(freeChannels) == 0:
            raise Exception('Maximum numbers of registered channels reached.')

        if channel <= 0:
            # Choose the next free channel for this log
            channel = freeChannels[0]

        elif channel not in freeChannels:
            raise Exception(f'Channel {channel} is already assigned.')

        recByteLen = variable.type.value[1] * recLen

        if recByteLen > self.numBytesLeft:
            raise Exception(f'Requested size of log {recLen} cannot be hold by the onboard buffer, which has {int(self.numBytesLeft / variable.type.value[1])} bytes left.')

        self.sciHdl.command(self.functions['RegisterLogFromVarStruct'], [self.index, channel, variable.number, divider, recLen])

        # We get here: Command was successful
        self.numBytesLeft = self.numBytesLeft - recByteLen
        self.chRegistered[channel - 1] = Channel(channel, variable, divider, recLen)
        self.loggerInit = False

    def initializeLogger(self):
        """
        Commands the onboard datalogger to initialize the registered log channels.
        Must be called prior to start().
        """

        if (self.functions['InitializeDatalogger'].number <= 0):
            raise Exception(f'No valid channel number for the callback InitializeDatalogger assigned.')

        if len(self.chRegistered) == 0:
            raise Exception(f'Channels must be registered before they can be initialized.')

        self.sciHdl.command(self.functions['InitializeDatalogger'], [self.index])

        self.loggerInit = True


    def start(self):
        """
        Starts the onboard datalogger.
        """

        if (self.functions['StartDatalogger'].number <= 0):
            raise Exception(f'No valid channel number for the callback StartDatalogger assigned.')

        if not self.loggerInit:
            raise Exception(f'Onboard logger must be initialized befor starting the logger.')

        self.sciHdl.command(self.functions['StartDatalogger'], [self.index])

    def stop(self):
        """
        Stops the onboard datalogger if it is running
        """

        if (self.functions['StopDatalogger'].number <= 0):
            raise Exception(f'No valid channel number for the callback StopDatalogger assigned.')

        self.sciHdl.command(self.functions['StopDatalogger'], [self.index])

    def getData(self) -> List[Channel]:
        """
        Retrieves channel data from the onboard data logger.

        Returns:
        --------
        - List of Channel-Instances containing log information and data.
        """

    
        ret = []
        
        data = self.sciHdl.requestUpstream(self.functions['GetLogData'], [self.index])

        chNums = [ch.channel for ch in self.chRegistered if ch is not None]

        offset = 0

        # Decompose channel data
        for num in chNums:
            self.chRegistered[num - 1].data = []
            
            # Slice to get the channel data bytearray
            chDat = data[offset : (self.chRegistered[num - 1].recLen * self.chRegistered[num - 1].variable.type.value[1]) + offset].copy()
            
            offset += self.chRegistered[num - 1].recLen * self.chRegistered[num - 1].variable.type.value[1]

            # Covert data and save into struct
            self.chRegistered[num - 1].data = [dat[0] for dat in struct.iter_unpack(f'>{self.chRegistered[num - 1].variable.type.value[0]}', chDat)]

            ret.append(self.chRegistered[num - 1])

        return ret


    def reset(self):
        """
        Resets the onboard datalogger.
        """
        self.sciHdl.command(self.functions['ResetDatalogger'],[self.index])

        self.chRegistered = [None] * self.MAX_NUM_LOGS
        self.loggerInit = False










