#include <stdio.h>
#include "SCI.h"
#include "Variables.h"
#include "TestHelpers.h"
#include "TestSCI.h"

bool testTxCb(uint8_t* p_buf, uint8_t size)
{
    p_buf[size] = '\0';
    printf((const char*)p_buf);

    return true;
}

int main (void)
{
    // Test Helpers
    //TestHelpers_hexToStr(0x12345678);
    //TestHelpers_strToHex();
    TestSCI_generateReads();
    return 0;
}