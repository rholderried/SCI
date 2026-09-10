#ifndef _MASTERTESTCALLBACKS_H_
#define _MASTERTESTCALLBACKS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void MasterTxCbBlocking(uint8_t* pui8Data, uint8_t ui8Size);

#ifdef __cplusplus
}
#endif

#endif // _MASTERTESTCALLBACKS_H_
