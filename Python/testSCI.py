import SCI

cmd = SCI.Command()
cmd.commandID = SCI.CommandID.SETVAR
cmd.number = 1
cmd.dataArray = [23, 2343, 2.43]
cmd.dataFormat = [SCI.Dataformat.DTYPE_UINT16, SCI.Dataformat.DTYPE_UINT32, SCI.Dataformat.DTYPE_F32]

# msg = 'A>01020304050607'
msg = 'A!DAT;3;567,A7,1'
msgArr = bytearray(msg,'ASCII')
msgArr.insert(0,2)
msgArr.append(3)
print(msgArr[0])

inst = SCI.SCI('COM7', 64, numberFormat=SCI.NumberFormat.HEX)
# packet = inst._encode(cmd)
received = inst._decode(msgArr, SCI.CommandID.SETVAR)
print(received)