#include <iostream>
#include <cstdint>
#include <cstdbool>
#include "SerialProtocol.h"
#include "Helpers.h"
#include "Variables.h"

using namespace std;

extern VAR varStruct[];
extern COMMAND_CB cmdStruct[];
uint8_t EESimulationBuffer[20];

void testFtoA (void)
{
    uint8_t retVal[40] = {0};
    uint8_t size = 0;
    float testVal = 5.131666;

    for (int i = 0; i < 20; i++ )
    {
        size = ftoa(retVal, testVal - static_cast<float>(i)/2, 5, true);

        retVal[size] = '\0';

        cout << retVal << endl;
    }

    testVal = 5;

    for (int i = 0; i < 10; i++ )
    {
        size = ftoa(retVal, testVal - static_cast<float>(i), 0, true);

        retVal[size] = '\0';

        cout << retVal << endl;
    }

}

bool testWriteEE (uint32_t ui32_val, uint16_t ui16_address)
{
    uint8_t val = ui32_val & 0xFF;
    EESimulationBuffer[ui16_address] = val;
    return true;
}

bool testReadEE (uint32_t *ui32_val, uint16_t ui16_address)
{
    uint8_t val = EESimulationBuffer[ui16_address];
    *ui32_val = val;
    return true;
}


bool testTxCb(uint8_t* p_buf, uint8_t size)
{
    p_buf[size] = '\0';
    cout << p_buf << endl;

    return true;
}

int main (void)
{
    float testvar = -0.567;
    SerialProtocol prot = SerialProtocol();
    prot.setupCallbacks(testTxCb, testReadEE, testWriteEE);
    prot.setupVariableStructure(varStruct,5);
    prot.setupCommandStructure(cmdStruct, 1);

    // Simulate a write to the EEPROM
    prot.receive(2);
    prot.receive('1');
    prot.receive('!');
    prot.receive('3');
    prot.receive('.');
    prot.receive('1');
    prot.receive('4');
    prot.receive(',');
    prot.receive('2');
    prot.receive(',');
    prot.receive('-');
    prot.receive('3');
    prot.receive('1');
    prot.receive(3);

    prot.statemachine();
    prot.statemachine();
    prot.statemachine();

    // // Simulate a read from the EEPROM
    // prot.receive(2);
    // prot.receive('4');
    // prot.receive('?');
    // prot.receive(3);

    // prot.statemachine();
    // prot.statemachine();
    // prot.statemachine();



    return 0;
}