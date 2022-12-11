# -*- coding: utf-8 -*-
"""
Created on Thu Jan 27 11:26:44 2022

@author: loh_tm, hold_ro
"""
import sys, os
sys.path.append(os.path.dirname(__file__))

from SerialProtocol import SerialProtocol
#from winreg import SetValue

from typing import *
from time import sleep
from enum import Enum

class varType(Enum):
    RAMTYPE     = 0
    EEPROMTYPE  = 1

class Parameter:

    def __init__(self, number : int, type : varType, value : float, setter : Callable, getter : Callable, description : str = ""):
        self.description = description
        self.number = number
        self.type = type
        self.value = value
        self.setter = setter
        self.getter = getter
    
    def set(self, value):
        try:
            value = self.setter(self.number, value)
        
            self.value = value
        except Exception as e:
            raise Exception(f"Setting variable {self.number} with value {self.value} failed: {e}")

    def get(self):

        try:
            value = self.getter(self.number)

            self.value = value

        except Exception as e:
            raise Exception(f"Getting variable {self.number} failed: {e}")
            
        return value



# Heater Controller Class ####################################################
class HeaterController(SerialProtocol):

    # Variable number starting values
    START_NUMBER_OUTPUTS                = 1
    START_NUMBER_SETPOINTS              = 19
    START_NUMBER_CH1_PT100_CALIB        = 31
    START_NUMBER_MEASURE_CALIB          = 91
    START_NUMBER_CONROL_CALIB           = 103

    NUMBER_OF_CALIB_POINTS  = 5
    NUMBER_OF_CHANNELS      = 6
    
    def __init__(self, port : str, initEEPROMVars : bool = True):
        """
        Initializes the serial port and initializes all variables.

        Parameter:
        ----------

            - port: COM-Port that is used for the device (i.e. "COM1")
        """

        # Connect to the device
        super().__init__(port)
        # Settling time for communication establishment
        sleep(1)

        self.Parameters     = {}
        self.Setpoints      = {}
        self.Outputs        = {}

        # Variables initialization ###########################################
        for j in range(self.NUMBER_OF_CHANNELS):
            self.Outputs.update(    {   f"CH{j+1}TempAct"       : Parameter(self.START_NUMBER_OUTPUTS + 3*j, varType.RAMTYPE, 0, self.setvalue, self.getvalue, f"CH{j+1} - Actual temperature in m°C")})
            self.Outputs.update(    {   f"CH{j+1}CtrlOut"       : Parameter(self.START_NUMBER_OUTPUTS + 3*j + 1, varType.RAMTYPE, 0, self.setvalue, self.getvalue, f"CH{j+1} - Actual temperature controller output [0 ... 255]")})
            self.Outputs.update(    {   f"CH{j+1}SPAct"         : Parameter(self.START_NUMBER_OUTPUTS + 3*j + 2, varType.RAMTYPE, 0, self.setvalue, self.getvalue, f"CH{j+1} - Actual Setpoint of the temperature in m°C")})

        for j in range(self.NUMBER_OF_CHANNELS):
            self.Setpoints.update( {    f"CH{j+1}TempSP"        : Parameter(self.START_NUMBER_SETPOINTS + 2*j, varType.RAMTYPE, 0, self.setvalue, self.getvalue, f"CH{j+1} - Setpoint of the temperature in m°C"),
                                        f"CH{j+1}dTempSP"       : Parameter(self.START_NUMBER_SETPOINTS + 2*j + 1, varType.RAMTYPE, 0, self.setvalue, self.getvalue, f"CH{j+1} - Setpoint of the temperature change rate in m°C/h")})

        for j in range(self.NUMBER_OF_CHANNELS):
            self.Parameters.update( {   f"CH{j+1}Res{i + 1}"    : Parameter(self.START_NUMBER_CH1_PT100_CALIB + i + 2*j*self.NUMBER_OF_CALIB_POINTS, varType.EEPROMTYPE, 0, self.setvalue, self.getvalue, f"CH{j+1} - Resistance value {i + 1} in mOhm") for i in range(self.NUMBER_OF_CALIB_POINTS)})
            self.Parameters.update( {   f"CH{j+1}Temp{i + 1}"   : Parameter(self.START_NUMBER_CH1_PT100_CALIB + i + 2*j*self.NUMBER_OF_CALIB_POINTS + self.NUMBER_OF_CALIB_POINTS, varType.EEPROMTYPE, 0, self.setvalue, self.getvalue, f"CH{j+1} - Temperature value {i + 1} in mdeg") for i in range(self.NUMBER_OF_CALIB_POINTS)})
        
        for j in range(self.NUMBER_OF_CHANNELS):
            self.Parameters.update( {   f"CH{j+1}Curr"          : Parameter(self.START_NUMBER_MEASURE_CALIB + 2*j, varType.EEPROMTYPE, 0, self.setvalue, self.getvalue, f"CH{j+1} - Temperature measurement current in mA"),
                                        f"CH{j+1}AmpGain"       : Parameter(self.START_NUMBER_MEASURE_CALIB + 2*j + 1, varType.EEPROMTYPE, 0, self.setvalue, self.getvalue, f"CH{j+1} - Temperature measurement differential amplifier gain")})

        for j in range(self.NUMBER_OF_CHANNELS):
            self.Parameters.update( {   f"CH{j+1}Gp"            : Parameter(self.START_NUMBER_CONROL_CALIB + 2*j, varType.EEPROMTYPE, 0, self.setvalue, self.getvalue, f"CH{j+1} - Proportional control gain Gp"),
                                        f"CH{j+1}Ti"            : Parameter(self.START_NUMBER_CONROL_CALIB + 2*j + 1, varType.EEPROMTYPE, 0, self.setvalue, self.getvalue, f"CH{j+1} - Integration time Ti")})
                                    
        if initEEPROMVars:
            self.initializeAllCalibrationValues()

    def initializeAllCalibrationValues(self):
        """
        Requests all calibration values stored on the controller EEPROM
        """

        # Get all PT100 calibration values
        for parameter in self.Parameters.values():
            if parameter.type.name == 'EEPROMTYPE':
                parameter.get()


    def startTemperatureControl(self, channels : List) -> bool:
        """
        Start the temperature control of the desired channels.

        Parameters:
        -----------
            - channels: List of the desired channels to switch on.
        """
        
        # Plausibility checker
        for i in range(len(channels)):
            if channels[i] not in range(1,self.NUMBER_OF_CHANNELS + 1):
                raise Exception(f"Channel number {channels[i]} not supported by the Heater Controller.")

        if not self.command(1,channels):
            raise Exception("Command did not succeed.")

    def startTemperatureControlStep(self, channels: List) -> bool:
        """
        Start the temperature control of the desired channels without using
        the setpoint generator.

        Parameters:
        -----------
            - channels: List of the desired channels to switch on.
        """

        # Plausibility checker
        for i in range(len(channels)):
            if channels[i] not in range(1, self.NUMBER_OF_CHANNELS + 1):
                raise Exception(f"Channel number {channels[i]} not supported by the Heater Controller.")

        if not self.command(3, channels):
            raise Exception("Command did not succeed.")

    def stopTemperatureControl(self, channels : List) -> bool:
        """
        Stop the temperature control of the desired channels.

        Parameters:
        -----------
            - channels: List of the desired channels to switch on.
        """

        # Plausibility checker
        for i in range(len(channels)):
            if channels[i] not in range(1,self.NUMBER_OF_CHANNELS + 1):
                raise Exception(f"Channel number {channels[i]} not supported by the Heater Controller.")

        if not self.command(2,channels):
            raise Exception("Command did not succeed.")