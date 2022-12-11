"""
Config file for the Datalogger interface

History:
--------
- Created by Holderried, Roman, 23.08.2022
"""

# Define Command Numbers - 0 if not active ----------------------------
CMD_NUM_GetDataloggerVersion        = 5
CMD_NUM_RegisterLogFromVarStruct    = 7
CMD_NUM_InitializeDatalogger        = 9
CMD_NUM_StartDatalogger             = 10
CMD_NUM_StopDatalogger              = 0
CMD_NUM_GetLogData                  = 12
CMD_NUM_GetChannelInfo              = 0
CMD_NUM_ResetDatalogger             = 14
CMD_NUM_SetOpMode                   = 0

# Define Datalogger Variable numbers ----------------------------------
VAR_NUM_BaseFrequency               = 10