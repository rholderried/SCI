import SCI

cmd = SCI.Command()
cmd.commandID = SCI.CommandID.SETVAR
cmd.number = 1
cmd.dataArray = [23, 2343, 2.43]
cmd.dataFormat = [SCI.Dataformat.DTYPE_UINT16, SCI.Dataformat.DTYPE_UINT32, SCI.Dataformat.DTYPE_F32]

inst = SCI.SCI('COM7', 64, numberFormat=SCI.NumberFormat.FLOAT)
packet = inst._encode(cmd)
print(packet)