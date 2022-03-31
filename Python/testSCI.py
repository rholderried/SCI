from SCI import *
from time import time

par1 = Parameter(1, Datatype.DTYPE_UINT16)
par2 = Parameter(2, Datatype.DTYPE_F32)

fcn1 = Function(1, requestsUpstream=True)

inst = SCI('COM5', 128, baud = 2000000, numberFormat=NumberFormat.HEX)

start = time()
#data = inst.getvalue(par1)
data = inst.requestUpstream(function=fcn1)
print(time() - start)
# packet.insert(0,2)
# packet.append(3)


# # msg = 'A>01020304050607'
# # msg = 'A!DAT;3;567,A7,1'
# # msgArr = bytearray(msg,'ASCII')
# # msgArr.insert(0,2)
# # msgArr.append(3)
# print(packet)

# # packet = inst._encode(cmd)
# received = inst._decode(packet, SCI.CommandID.SETVAR)
# testint = int(received.responseDesignator, 16)
# testVar = inst._reinterpretDecodedIntToDtype(testint, SCI.Datatype.DTYPE_INT32)
print(data)