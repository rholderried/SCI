#include <gtest/gtest.h>

extern "C" {
#include "SCISlave.h"
#include "SCIMaster.h"
}

extern "C" {
    extern tsSCIVAR varStruct[];
    extern COMMAND_CB cmdStruct;
    extern tsSCI_SLAVE_CALLBACKS sSlaveTestCbs;
    extern char cTxMsgBuf[];
    extern char cRxMsgBuf[];
}

namespace {

class SlaveTest : public ::testing::Test {
protected:
    void SetUp() override {
        SCISlaveInit(sSlaveTestCbs, varStruct, &cmdStruct);
    }
};

// Pumps the slave statemachine, injecting one message byte per tick,
// mirroring the loop shape used by the pre-migration Unity tests.
static void PumpSlave(const uint8_t* msg, uint8_t msgLen, int numLoops = 100) {
    uint8_t j = 0;
    for (int i = 0; i < numLoops; i++) {
        if (j < msgLen) {
            SCISlaveReceiveData(msg[j]);
            j++;
        }
        SCISlaveStatemachine();
    }
}

TEST_F(SlaveTest, PollVarUI8) {
    uint8_t msg[]    = {0x02, '3', '?', 0x03};
    uint8_t expect[] = {0x02, '3', '?', 'A', 'C', 'K', ';', 'F', '5', 0x03};

    PumpSlave(msg, sizeof(msg));

    EXPECT_EQ(0, memcmp(expect, cTxMsgBuf, sizeof(expect)));
}

}  // namespace
