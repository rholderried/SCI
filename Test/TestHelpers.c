#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "Helpers.h"

void TestHelpers_hexToStr(uint32_t ui32_testval)
{
    uint8_t ui8_valBuf[4] = {0};
    uint8_t ui8_testBuf[10] ={0};
    int8_t size = 0;

    fillByteBufBigEndian(ui8_valBuf, (uint8_t*)&ui32_testval, 4);

    for (uint8_t i = 0; i < 4; i++)
    {
        size += hexToStrByte(&ui8_testBuf[i << 1], &ui8_valBuf[i], false);
    }
    
    ui8_testBuf[size] = '\0';

    printf("Conversion Result: %s", (char*)ui8_testBuf);

}

void TestHelpers_strToHex(void)
{
    bool b_valid = false;
    uint8_t ui8_testBuf[9] = {0};

    union 
    {
        uint8_t     ui8_test;
        int8_t      i8_test;
        uint16_t    ui16_test;
        uint16_t    i16_test;
        uint32_t    ui32_test;
        int32_t     i32_test;
        float       f_test;
    }u_testVal;

    u_testVal.ui32_test = 0;


    b_valid = strToHex(ui8_testBuf, &u_testVal.ui32_test);

    printf("Conversion Result: %x", u_testVal.ui32_test);

}