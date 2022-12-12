/**************************************************************************//**
 * \file SCIMasterTransfer.h
 * \author Roman Holderried
 *
 * \brief SCI transfer related declarations / definitions.
 *
 * <b> History </b>
 * 	- 2022-11-17 - File creation -
 *  - 2022-12-12 - Adapted code for unified master/slave repo structure.
 *****************************************************************************/


#ifndef _SCIMASTERTRANSFER_H_
#define _SCIMASTERTRANSFER_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "SCIMasterConfig.h"
#include "SCITransferCommon.h"

/******************************************************************************
 * Defines
 *****************************************************************************/



/******************************************************************************
 * Type definitions
 *****************************************************************************/
typedef enum
{
    eSCI_REQUEST_ERROR_NONE = 0,
}teREQUEST_ERROR;

// /** \brief REQUEST structure declaration.*/
// typedef struct
// {
//     int16_t         i16Num;                            /*!< ID Number.*/
//     teREQUEST_TYPE  eReqType;                          /*!< REQUEST Type.*/
//     tuREQUESTVALUE  *uValArr;                          /*!< Pointer to the value array.*/
//     uint8_t         ui8ValArrLen;                      /*!< Length of the value Array.*/
// }tsREQUEST;

// #define tsREQUEST_DEFAULTS         {0, 0, NULL, eREQUEST_TYPE_NONE}



// #define tsRESPONSE_DEFAULTS         {0, eREQUEST_TYPE_NONE, eREQUEST_ACK_STATUS_UNKNOWN, 0, {{.ui32_hex = 0}}, NULL, 0, 0}

typedef struct
{
    tsREQUEST       sReq;
    uint8_t         ui8MessageDataCnt;
    uint32_t        ui32ExpectedDataCnt;
    uint32_t        ui32ReceivedDataCnt;
    uint32_t        ui32TransferCnt;
    tuRESPONSEVALUE *uTransferResults;
    uint8_t         *pui8UpstreamBuffer;
}tsTRANSFER_INFO;

#define tsTRANSFER_INFO_DEFAULTS {tsREQUEST_DEFAULTS, 0, 0, 0, 0, NULL, NULL}

typedef struct
{
    tsTRANSFER_INFO     sTransferInfo;

    struct
    {
        teTRANSFER_ACK  (*SetVarCB)(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint16_t ui16ErrNum);
        teTRANSFER_ACK  (*GetVarCB)(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint32_t ui32Data, uint16_t ui16ErrNum);
        teTRANSFER_ACK  (*CommandCB)(teREQUEST_ACKNOWLEDGE eAck, int16_t i16Num, uint32_t *pui32Data, uint8_t ui8DataCnt, uint16_t ui16ErrNum);
        teTRANSFER_ACK  (*UpstreamCB)(int16_t i16Num, uint8_t *pui8Data, uint32_t ui32ByteCnt);

        bool        (*RequestCB)(tsREQUEST sReq);
        void        (*InitiateStreamCB)(uint32_t ui32ByteCount);
        void        (*FinishStreamCB)(void);
        void        (*ReleaseProtocolCB)(void);
    }sCallbacks;
}tsSCI_TRANSFER;

#define tsSCI_TRANSFER_DEFAULTS {tsTRANSFER_INFO_DEFAULTS, {NULL}}

/******************************************************************************
 * Function declarations
 *****************************************************************************/

/** \brief Builds the request and starts the transmission.
 * 
 * @param psSciTransfer Pointer to the transfer data
 * @param eReqType      Request type of the transfer
 * @param i16CmdNum     Request number of the transfer
 * @param uVal          Pointer to the array of parameters to transmit
 * @param ui8Argnum     Number of parameters to transmit
 * 
 * @returns Error indicator
 * */
bool SCITransferStart (tsSCI_TRANSFER *psSciTransfer, teREQUEST_TYPE eReqType, int16_t i16CmdNum, tuREQUESTVALUE *uVal, uint8_t ui8ArgNum);

/** \brief Handles the transfer responses according to the protocol mechanisms.
 * 
 * TODO:
 * - What is going to be done if the device returns "UNKNOWN" ?
 * 
 * @param psSciTransfer Pointer to the transfer data
 * @param sRsp          Response data that has just been arrived
 * 
 * @returns Error indicator
 * */
bool SCITransferControl (tsSCI_TRANSFER *psSciTransfer, tsRESPONSE sRsp);



#endif //_SCIMASTERTRANSFER_H_