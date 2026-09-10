# SCI - Serial Communication Interface
C-Port of the SerialProtocol.

## General
The SCI is a request-response style protocol using a master (or host) - slave (or device) 
relationship. It relies on the presence of at least a device internal variable structure (var struct). 
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
called if the variable has been written by the host. For more complex applications the 
host could also make use of the remote function call capability of SCI.

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

## Behavioural Description (Protocol specification)

Most of the transferred data is encoded as human-readable ASCII data so the communication
can be debugged easier.

A SCI transfer always consists of a request issued by the SCI Master and the following
response by the SCI Slave.

A SCI request provides access to either a variable of the var struct or a device internal
callback function, now referred to as remote function call (RFC) that are identified by
their respective addresses. Currently, the addresses are directly related to the item
position within their respective arrays. That means, for example, an access to the variable
with the address 1 means that the variable of the first descriptor entry in the descriptor
array gets read or written. This simple method minimizes the access delay, since the
data dictionary (array of items) do not have to be searched. 

The basic messaging format is as follows:

<img src="doc\latex_img\base_msg_table\extable.svg" alt="Basic message formatting" width="600"/>

STX and ETX are the control symbols are used to identify the start and the end of the transmission, respectively. After STX, the address of the item is transmitted, followed by the Transmission
Identifier. By this Identifier, the purpose of the transmission is determined.

<img src="doc\latex_img\transmission_id\transmission_id.svg" alt="Message identifier" width="180"/>

GetVar and SetVar requests are used to read and write a variable of the var struct,
respectively, while the Command Identifier is used for remote function calls. The Upstream 
and Downstream Identifiers indicate a message carrying binary data in the Data Field of the
message. These are used for transferring huge amount of data. The Unknown Identifier is 
only used in response messages, if the requested transmission identifier could not be
determined.

In a request message, the Transmission Identifier is followed by the Data Field. Multiple
data units can be transmitted, these need to be separated by a comma. The current 
implementation of SCI limits the amount of data that can be sent down to the device such
that the message length does not exceed the configured maximum request message length.
The Data Field of a request message is not mandatory. For example, in a GetVar transmission, 
only  the Address Field and the Transmission Identifier need to be present.

In a response message, the Acknowledge follows on the Transmission Identifier if the
transmission is not of type Upstream. 
It consists of one of the following strings:

<img src="doc\latex_img\acknowledge\acknowledge.svg" alt="Acknowledge type" width="210"/>

The most general acknowledgement type is ACK. It indicates that the requested operation
has been successfully executed. The DAT acknowledgement is sent if a remote function call
response contains one or multiple data units. The third positive acknowledgement type is
the UPS acknowlegment, which is explained further below. A NAK is send if the request 
type could not be identified by the SCI slave stack, while an ERR acknowledgement indicates
that an error appeared while processing the request. In this case, the response contains
an error identifier in its Data Field.

The response Data Field, which follows on the Acknowledge Field is seqarated by a ';' 
character. If multiple data units are to be transferred, they are separated by a comma.
Unlike the data field of a request, an arbitrary amount of data can be transferred by
a response. If the amount of data exeeds the maximum response message length, the data
needs to be split into multiple transmissions. Each transmission has to be requested by
the SCI Host until no more data remains to be transferred. While ongoing data is transferred
within multiple transmissions, the request message of each transmission needs to be always 
the same.

As indicated by the transmission identifier table, SCI supports multiple transmission
types that are discussed in more detail in the chapters that follow.

### Variable access (SetVar and GetVar)


