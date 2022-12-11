import sys, os
sys.path.append(os.path.realpath(os.path.join(os.path.dirname(__file__), '..')))
from SCI import *
from time import time

par1 = Parameter(1, Datatype.DTYPE_UINT16)
par2 = Parameter(2, Datatype.DTYPE_F32)

fcn1 = Function(1, requestsUpstream=True)
fcn2 = Function(2, argTypeList=[Datatype.DTYPE_UINT16], returnTypeList=[Datatype.DTYPE_F32]*10)

inst = SCI('COM3', 128, baud = 2000000, numberFormat=NumberFormat.HEX)

start = time()
inst.setvalue(par1, 6)
data = inst.getvalue(par1)
print(data)
data = inst.requestUpstream(function=fcn1)
print(data)
data = inst.command(fcn2, paramList=[4])
print(data)
data = inst.getvalue(par1)
print(data)
print(time() - start)
# packet.insert(0,2)
# packet.append(3)

# # packet = inst._encode(cmd)
# received = inst._decode(packet, SCI.CommandID.SETVAR)
# testint = int(received.responseDesignator, 16)
# testVar = inst._reinterpretDecodedIntToDtype(testint, SCI.Datatype.DTYPE_INT32)