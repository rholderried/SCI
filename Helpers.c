/**************************************************************************//**
 * \file Helpers.c
 * \author Roman Holderried
 *
 * \brief Definitions of the Helpers modules.
 *
 * <b> History </b>
 * 	- 2022-01-17 - File creation
 *  - 2022-03-17 - Port to C (Originally from SerialProtocol)
 *****************************************************************************/
/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include "Helpers.h"

/******************************************************************************
 * Global variables definitions
 *****************************************************************************/
const uint32_t ui32_pow10[10] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
const uint8_t hexNibbleConv[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};


/******************************************************************************
 * Function definitions
 *****************************************************************************/
uint8_t ftoa (uint8_t *pui8_resBuf, float val, bool b_round)
{
    float signum            = (val < 0) * -1 + (val > 0);
    float rval              = val + b_round * signum * 0.5f / ui32_pow10[FTOA_MAX_AFTERPOINT]; 
    int32_t i32_tmp         = (int32_t)(rval);
    int32_t i32_tmp2        = 0;
    uint32_t ui32_decimator = 1;
    uint8_t ui8_size        = 0;
    int8_t i8_exp           = -1;
    uint8_t ui8_digit       = 0;
    uint32_t ui32_afterPoint= (uint32_t)(signum * (rval - i32_tmp) * ui32_pow10[FTOA_MAX_AFTERPOINT]);


    // Sign evaluation - Add the sign if necessary
    if (signum < 0)
    {
        *pui8_resBuf++ = '-';
        ui8_size++;
        i32_tmp = -1 * i32_tmp;
    }

    // Determine decimator -> Determine how big the number is
    i32_tmp2 = i32_tmp;
    while (i32_tmp2 > 0)
    {
        i8_exp++;
        
        if (i8_exp > 0)
            ui32_decimator *= 10;

        i32_tmp2 /= 10;
    }

    // If 1 > rval > -1, write a '0' into the buffer place
    if (i8_exp < 0)
    {
        *pui8_resBuf++ = '0';
        ui8_size++;
    }
    
    else
    {
        ui8_size += i8_exp + 1;

        // Write digits into buffer
        while(i8_exp >= 0)
        {
            // Determine next digit
            ui8_digit = i32_tmp/ui32_decimator;
            // Write ASCII digit into the buffer
            *pui8_resBuf++ = ui8_digit + '0';
            
            i32_tmp -= ui8_digit * ui32_decimator;
            ui32_decimator /= 10;
            i8_exp--;
        }
    }

    // After point digits
    if(ui32_afterPoint > 0)
    {
        int8_t  i = 0;
        bool    b_trailingZero = true;
        uint8_t ui8_tmp[FTOA_MAX_AFTERPOINT] = {0};
        uint8_t ui8_sizeTmp = 0;

        ui32_decimator = ui32_pow10[FTOA_MAX_AFTERPOINT - 1];

        // Keep the space for the decimal point free
        pui8_resBuf++;

        // Convert all digits after the decimal point
        while(i < FTOA_MAX_AFTERPOINT)
        {
            ui8_digit = (uint8_t)(ui32_afterPoint / ui32_decimator);
            ui32_afterPoint -= ui8_digit * ui32_decimator;
            ui32_decimator /= 10;
            ui8_tmp[i] = (ui8_digit + '0');
            i++;
        }
        
        // prepare the output buffer pointer
        pui8_resBuf += --i;

        // Fill the output buffer from behind to get rid of trailing zeros
        while (i >= 0)
        {
            b_trailingZero = !b_trailingZero ? b_trailingZero : !(ui8_tmp[i] > '0');

            if (!b_trailingZero)
            {
                *pui8_resBuf = ui8_tmp[i];
                ui8_sizeTmp++;
            }
            pui8_resBuf--;
            i--;
        }

        // Add the decimal point if there are any afterpoint digits
        if (ui8_sizeTmp > 0)
        {
            *pui8_resBuf = '.';
            ui8_size += ui8_sizeTmp + 1;
        }
    }
    return ui8_size;
}

//=============================================================================
bool strToHex (uint8_t *pui8_strBuf, uint32_t *pui32_val)
{
    int8_t i = 0;
    int8_t j = 0;
    uint32_t ui32_hexVal = 0;
    bool valid = true;

    *pui32_val = 0;

    // Determine number of passed digits
    while (*pui8_strBuf++ != '\0')
        i++;
    
    // Hex number cannot be greater than 8 nibbles
    if (i>8)
    {
        valid = false;
        goto terminate;
    }

    j=i;

    // Set the buffer pointer on the least significant nibble
    if (i > 0)
        pui8_strBuf -= 2;
    // If there was passed a string that was just holding a '\0', interpret as 0
    else
    {
        *pui32_val = 0;
        goto terminate;
    }

    while (i > 0)
    {
        if(*pui8_strBuf >='0' && *pui8_strBuf <= '9')
            *pui32_val |= (*pui8_strBuf - '0') << ((j - i) * 4);
        else if (*pui8_strBuf >= 'A' && *pui8_strBuf <= 'F')
            *pui32_val |= (*pui8_strBuf - 'A' + 10) << ((j - i) * 4);
        else
        {
            valid = false;
            break;
        }
        i--;
        pui8_strBuf--;
    }

    terminate: return valid;
}

//=============================================================================
int8_t hexToStr (uint8_t *pui8_strBuf, uint32_t *pui32_val)
{
    int8_t j = 7;
    int8_t numberOfDigits = 0;

    // Determine number of digits to pass (j holds this number afterwards)
    while (j >= 0)
    {
        if((*pui32_val & (uint32_t)(0xF << (j * 4))) > 0)
            break;
        else
            j--;
    }

    if (j < 0)
    {
        *pui8_strBuf = '0';
        numberOfDigits = 1;
        goto terminate;
    }

    numberOfDigits = j;

    while (j >= 0)
    {
        pui8_strBuf[numberOfDigits - j] = hexNibbleConv[(*pui32_val & (uint32_t)(0xF << (j * 4))) >> (j * 4)];
        j--;
    }

    numberOfDigits++;

    terminate: return numberOfDigits;
}