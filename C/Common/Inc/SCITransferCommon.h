/**************************************************************************//**
 * \file SCITransferCommon.h
 * \author Roman Holderried
 *
 * \brief Common defines and type definitions of an SCI Transfer.
 *
 * <b> History </b>
 * 	- 2022-12-11 - File creation
 *  - 2026-09-10 - tsREQUEST.uValArr changed from a bare tuREQUESTVALUE*
 *                 pointer to a fixed-size tuREQUESTVALUE[MAX_NUM_REQUEST_VALUES]
 *                 array member, so every tsREQUEST owns its own backing
 *                 storage (matches the existing tsTRANSFER_DATA.puRespVals
 *                 pattern below). Previously the pointer defaulted to NULL
 *                 via tsREQUEST_DEFAULTS and nothing on the Slave's request-
 *                 parsing path ever pointed it at real storage, causing a
 *                 NULL-pointer write on any SetVar/Command-with-arguments
 *                 request; callers that build a tsREQUEST with values now
 *                 copy into uValArr instead of aliasing their own array
 *                 (see SCIMasterTransfer.c's SCITransferStart()). Also
 *                 fixed tsREQUEST_DEFAULTS, whose brace-initializer field
 *                 order did not match tsREQUEST's actual member order.
 *****************************************************************************/

#ifndef _SCITRANSFERCOMMON_H_
#define _SCITRANSFERCOMMON_H_
/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "SCIconfig.h"

/******************************************************************************
 * defines
 *****************************************************************************/
#define UNKNOWN_IDENTIFIER      '#'
#define GETVAR_IDENTIFIER       '?'
#define SETVAR_IDENTIFIER       '!'
#define COMMAND_IDENTIFIER      ':'
#define UPSTREAM_IDENTIFIER     '>'
#define DOWNSTREAM_IDENTIFIER   '<'
/******************************************************************************
 * Type definitions
 *****************************************************************************/


/** \brief Request acknowledge enumeration */
typedef enum
{
    eREQUEST_ACK_STATUS_SUCCESS             = 0,
    eREQUEST_ACK_STATUS_SUCCESS_DATA        = 1,
    eREQUEST_ACK_STATUS_SUCCESS_UPSTREAM    = 2,
    eREQUEST_ACK_STATUS_ERROR               = 3,
    eREQUEST_ACK_STATUS_UNKNOWN             = 4
}teREQUEST_ACKNOWLEDGE;

/** \brief Return value of the Transfer callbacks*/
typedef enum
{
    eTRANSFER_ACK_SUCCESS = 0,
    eTRANSFER_ACK_REPEAT_REQUEST,
    eTRANSFER_ACK_ABORT
}teTRANSFER_ACK;

/** \brief Request type enumeration.*/
typedef enum 
{
    eREQUEST_TYPE_NONE          = 0,
    eREQUEST_TYPE_GETVAR        = 1,
    eREQUEST_TYPE_SETVAR        = 2,
    eREQUEST_TYPE_COMMAND       = 3,
    eREQUEST_TYPE_UPSTREAM      = 4,
    eREQUEST_TYPE_DOWNSTREAM    = 5
}teREQUEST_TYPE;

typedef union
{
    float           f_float;
    uint32_t        ui32_hex;
}tuREQUESTVALUE;

typedef tuREQUESTVALUE tuRESPONSEVALUE;

typedef struct
{
    union
    {
        struct
        {
            // uint8_t dataBufDynamic      : 1;
            uint8_t upstreamBufDynamic  : 1;
            uint8_t reserved            : 7;
        }ui8InfoFlagBits;

        uint8_t ui8InfoFlagByte;
    };
    uint8_t         *pui8UpStreamBuf;
    tuRESPONSEVALUE puRespVals[MAX_NUM_RESPONSE_VALUES];
    uint32_t        ui32DatLen;
    uint16_t        ui16Error;
}tsTRANSFER_DATA;

#define tsTRANSFER_DATA_DEFAULTS {{.ui8InfoFlagByte = 0}, NULL, {{.ui32_hex = 0}}, 0, 0}

/** \brief REQUEST structure declaration.*/
typedef struct
{
    int16_t         i16Num;                            /*!< ID Number.*/
    teREQUEST_TYPE  eReqType;                          /*!< REQUEST Type.*/
    tuREQUESTVALUE  uValArr[MAX_NUM_REQUEST_VALUES];   /*!< Value array backing storage (fixed size, not a bare pointer - every tsREQUEST owns its own storage, so callers/parsers can always write into it directly).*/
    uint8_t         ui8ValArrLen;                      /*!< Length of the value Array.*/
}tsREQUEST;

#define tsREQUEST_DEFAULTS         {0, eREQUEST_TYPE_NONE, {{.ui32_hex = 0}}, 0}

/** \brief Response structure declaration.*/
typedef struct
{
    int16_t                 i16Num;                             /*!< ID Number. (Reflects REQUEST ID number).*/
    teREQUEST_TYPE          eReqType;                           /*!< Response type inherited from REQUEST type.*/
    teREQUEST_ACKNOWLEDGE   eReqAck;                            /*!< Acknowledge returned by the REQUEST callback.*/
    tsTRANSFER_DATA         sTransferData;                      /*!< Structure holding info regarding the transfer data*/
    // uint8_t                 ui8ResponseDataLength;              /*!< Data length within actual response */
    // tuRESPONSEVALUE         uValArr[MAX_NUM_RESPONSE_VALUES];   /*!< Pointer to the value array.*/                           /*!< Response value.*/
    // uint8_t                 *pui8Raw;                           /*!< Raw data of the response dataframe */
    // uint16_t                ui16ErrNum;                         /*!< Returned error number */
    // uint32_t                ui32DataLength;                     /*!< Whole length of the data to follow */
}tsRESPONSE;

#define tsRESPONSE_DEFAULTS {  0,\
                            eREQUEST_TYPE_NONE,\
                            eREQUEST_ACK_STATUS_UNKNOWN,\
                            tsTRANSFER_DATA_DEFAULTS}

#endif //_SCITRANSFERCOMMON_H_