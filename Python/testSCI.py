import SCI

cmd = SCI.Command()
cmd.commandID = SCI.CommandID.SETVAR
cmd.number = 1
cmd.dataArray = [-2343]
cmd.datatypeArray = [SCI.Datatype.DTYPE_INT32]

inst = SCI.SCI('COM7', 64, numberFormat=SCI.NumberFormat.HEX)

packet = inst._encode(cmd)
packet.insert(0,2)
packet.append(3)


# msg = 'A>01020304050607'
# msg = 'A!DAT;3;567,A7,1'
# msgArr = bytearray(msg,'ASCII')
# msgArr.insert(0,2)
# msgArr.append(3)
print(packet)

# packet = inst._encode(cmd)
received = inst._decode(packet, SCI.CommandID.SETVAR)
testint = int(received.responseDesignator, 16)
testVar = inst._reinterpretDecodedIntToDtype(testint, SCI.Datatype.DTYPE_INT32)
print(testVar)