/**************************************************************************//**
 * \file RoundTripFixture.h
 * \author Roman Holderried
 *
 * \brief Shared GoogleTest scaffolding for Master/Slave round-trip tests.
 *
 * Provides the common SetUp() (Slave init) and the PumpUntilIdle() helper
 * used by every round-trip test file. Each test file still declares its
 * own callback-capture statics and installs its own
 * tsSCI_MASTER_CALLBACKS (via SCIMasterInit()) in its own SetUp(), since
 * different request types need different captured payloads - that part is
 * deliberately not generalized here.
 *
 * <b> History </b>
 * 	- 2026-09-10 - File creation (extracted from RoundTripTests.cpp)
 *****************************************************************************/
#ifndef _ROUNDTRIPFIXTURE_H_
#define _ROUNDTRIPFIXTURE_H_

#include <gtest/gtest.h>

extern "C" {
#include "SCISlave.h"
#include "SCIMaster.h"
#include "MasterTestCallbacks.h"
}

extern "C" {
    extern tsSCIVAR varStruct[];
    extern COMMAND_CB cmdStruct[];
    extern tsSCI_SLAVE_CALLBACKS sSlaveTestCbs;
}

/** \brief Base fixture for Master/Slave round-trip tests.
 *
 * Initializes the Slave only. Derived fixtures must call
 * RoundTripTest::SetUp() first, then call SCIMasterInit() themselves with
 * whichever external callbacks their test file needs.
 */
class RoundTripTest : public ::testing::Test {
protected:
    void SetUp() override {
        SCISlaveInit(sSlaveTestCbs, varStruct, cmdStruct);
    }
};

/** \brief Pumps both statemachines in lockstep until the Master returns to
 *  IDLE (transaction complete) or the iteration budget runs out.
 *
 * @param maxIterations Iteration budget. Multi-packet transfers (large
 *                       upstream payloads, chunked command data) need a
 *                       higher budget than the default - raise it per call
 *                       site rather than globally.
 * @returns true if the Master reached IDLE within the budget, false if the
 *          budget was exhausted first.
 */
static inline bool PumpUntilIdle(int maxIterations = 500) {
    for (int i = 0; i < maxIterations; i++) {
        SCIMasterSM();
        SCISlaveStatemachine();
        if (SCIGetProtocolState() == ePROTOCOL_IDLE && i > 0)
            return true;
    }
    return false;
}

#endif //_ROUNDTRIPFIXTURE_H_
