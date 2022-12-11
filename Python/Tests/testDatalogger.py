import sys, os
sys.path.append(os.path.realpath(os.path.join(os.path.dirname(__file__), '..')))

from SCI import Datatype, SCI, Variable
from Datalogger import Datalogger
from time import sleep
import matplotlib.pyplot as plt

cur_A = Variable(1, Datatype.DTYPE_INT16, 'phase A current, in Q format')
cur_B = Variable(2, Datatype.DTYPE_INT16, 'phase B current, in Q format')
cur_C = Variable(3, Datatype.DTYPE_INT16, 'phase C current, in Q format')

hallSector = Variable(4, Datatype.DTYPE_INT8, 'Hall decoder sector')
hallPos = Variable(5, Datatype.DTYPE_INT32, 'Hall decoder position')

sixStepVector = Variable(6, Datatype.DTYPE_UINT8, 'Vector determined by six step module')
sixStepOut = Variable(7, Datatype.DTYPE_UINT16, 'Output value of the six step module')

sciHdl = SCI('COM27', 128)
dlogHdl = Datalogger(sciHdl, 0, 15000, os.path.join(os.path.realpath(os.path.join(os.path.dirname(__file__))), 'DataloggerCfg.py'))

dlogHdl.reset()
dlogHdl.register(cur_A, 2500, 1)
dlogHdl.register(cur_B, 2500, 1)
dlogHdl.register(cur_C, 2500, 1)
# dlogHdl.register(hallSector, 1250, 16)
# dlogHdl.register(sixStepVector, 1250, 16)


dlogHdl.initializeLogger()

dlogHdl.start()
sleep(2)

data = dlogHdl.getData()

timebaseCh1 = [1/dlogHdl.baseFrequency_Hz * data[0].divider * i for i in range(0,len(data[0].data))]
timebaseCh2 = [1/dlogHdl.baseFrequency_Hz * data[1].divider * i for i in range(0,len(data[1].data))]
timebaseCh3 = [1/dlogHdl.baseFrequency_Hz * data[2].divider * i for i in range(0,len(data[2].data))]

plt.plot(timebaseCh1, data[0].data, timebaseCh2, data[1].data, timebaseCh3, data[2].data)
plt.show()
