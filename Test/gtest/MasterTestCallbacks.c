#include "MasterTestCallbacks.h"
#include "SCISlave.h"

// Mirrors SlaveTxCbBlocking in TestCallbacks.c: forwards every byte the
// Master transmits directly into the Slave's receive function, in-process.
// Valid only because both sides run in the same test binary/address space.
void MasterTxCbBlocking(uint8_t* pui8Data, uint8_t ui8Size)
{
    for (uint8_t i = 0; i < ui8Size; i++)
        SCISlaveReceiveData(pui8Data[i]);
}
