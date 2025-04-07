# SCI - Serial Communication Interface
C-Port of the SerialProtocol.

## General
The SCI is a request-response style protocol using a master (or host) - slave (or device) 
relationship. It relies on the presence of a device internal variable structure. 
This variable structure is represented by an array of the following struct variable (descriptor):

```
typedef struct
{
    void        *pVal;
    teVARTYPE   eVartype;
    teDTYPE     eDatatype;

    ACTION_PROCEDURE ap;
}tsSCIVAR;
```
The descriptor holds all basic information related to the data, such as memory location,
data type and storage type. Also, an optional action procedure can be defined that gets
called if the variable has been written by the master.

Although the code is written to be highly portable across platforms (no hardware specific
code), care must be taken about the following aspects:

- The code uses dynamic memory allocation, which can be problematic in MISRA-C compliant 
code.
- The code makes reinterprets variable values by using the union data type which as for ANSI C
causes 'undefined behaviour'. Whether the code works as intended is therefore dependent
on the compiler. It has been proven to work on several platforms though, including STM32 
and Microchip PIC processors, as well as the GNU compiler collection.

## Layered design

The protocol is currently designed based on four different layers:

<img src="doc\layers.png" alt="Layered design of SCI." width="600"/>

The Datalink layer is common to the host and device code.

## Protocol specification

Most of the transferred data is encoded as human-readable ASCII data so the communication
can be debugged easier.

A SCI transfer always consists of a request issued by the SCI Master and the following
response by the SCI Slave.

<img src="doc\latex_img\base_msg_table\extable.svg" alt="Basic message formatting" width="600"/>

In the basic SCI message definition, STX and ETX are the only control symbols used to
identify the start and the end of the transmission, respectively.

SCI supports multiple communication modes:
- Variable access (read/write).
- Direkt commands with the ability to pass multiple parameters at once and get return values.
- Up- and Downstream for bulk data.


### Variable access


