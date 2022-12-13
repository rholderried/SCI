/**************************************************************************//**
 * \file TestCallbacks.c
 * \author Roman Holderried
 *
 * \brief Dummy callbacks for testing purposes.
 *
 * <b> History </b>
 * 	- 2022-12-13 - File creation
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "SCISlave.h"
#include "SCIMaster.h"
#include "SCIconfig.h"

/******************************************************************************
 * Global variable definition
 *****************************************************************************/
char cTxMsgBuf[TX_PACKET_LENGTH + 2] = {0};
char cRxMsgBuf[RX_PACKET_LENGTH + 2] = {0};
uint8_t ui8EEPROMByteAddressable [MAX_NUMBER_OF_EEPROM_VARS * 4] = {0};
uint16_t ui16EEPROMWordAddressable [MAX_NUMBER_OF_EEPROM_VARS * 2] = {0};
uint32_t ui32EEPROMDWordAddressable [MAX_NUMBER_OF_EEPROM_VARS] = {0};
/******************************************************************************
 * Function definitions
 *****************************************************************************/
bool SlaveGetBusyState (void)
{
    static uint8_t ui8BsyCnt = 0;

    if (++ui8BsyCnt > 3)
    {
        ui8BsyCnt = 0;
        return false;
    }
    else 
        return true;
}

uint8_t SlaveTxCbNonBlocking(uint8_t* pui8Data, uint8_t ui8Size)
{
    static uint8_t ui8Idx = 0;

    if ((ui8Size == 1) && pui8Data[0] == STX)
    {
        ui8Idx = 1;
        cTxMsgBuf[0] = STX;
    }
    else
    {
        memcpy(&cTxMsgBuf[ui8Idx], pui8Data, ui8Size);
    }

    for(uint8_t i = 0; i < ui8Size; i++)
        SCIMasterReceiveData(&pui8Data[i], ui8Size);
    
    return ui8Size;
}

void SlaveTxCbBlocking(uint8_t* pui8Data, uint8_t ui8Size)
{
    static uint8_t ui8Idx = 0;

    if ((ui8Size == 1) && pui8Data[0] == STX)
    {
        ui8Idx = 1;
        cTxMsgBuf[0] = STX;
    }
    else
    {
        memcpy(&cTxMsgBuf[ui8Idx], pui8Data, ui8Size);
        ui8Idx += ui8Size;
    }

    for(uint8_t i = 0; i < ui8Size; i++)
        SCIMasterReceiveData(&pui8Data[i], ui8Size);   
}

bool SlaveReadEEROM (uint32_t *ui32Val, uint16_t ui16Address)
{
    #if EEPROM_ADDRESSTYPE == EEPROM_BYTE_ADDRESSABLE
    *ui32Val = ui8EEPROMByteAddressable[ui16Address];
    #elif EEPROM_ADDRESSTYPE == EEPROM_WORD_ADDRESSABLE
    *ui32Val = ui16EEPROMWordAddressable[ui16Address];
    #else
    *ui32Val = ui32EEPROMDWordAddressable[ui16Address];
    #endif

    return true;
}

bool SlaveWriteEEROM (uint32_t ui32Val, uint16_t ui16Address)
{
    #if EEPROM_ADDRESSTYPE == EEPROM_BYTE_ADDRESSABLE
    ui8EEPROMByteAddressable[ui16Address] = ui32Val;
    #elif EEPROM_ADDRESSTYPE == EEPROM_WORD_ADDRESSABLE
    ui16EEPROMWordAddressable[ui16Address] = ui32Val;
    #else
    ui32EEPROMDWordAddressable[ui16Address] = ui32Val;
    #endif

    return true;
}

/******************************************************************************
 * Callback structure definition
 *****************************************************************************/
tsSCI_SLAVE_CALLBACKS sSlaveTestCbs =   {   .cbGetTxBusyState = SlaveGetBusyState,
                                            .cbTransmitBlocking = SlaveTxCbBlocking,
                                            .cbTransmitNonBlocking = SlaveTxCbNonBlocking,
                                            .cbReadEEPROM = SlaveReadEEROM,
                                            .cbWriteEEPROM = SlaveWriteEEROM};