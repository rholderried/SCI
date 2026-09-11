/**************************************************************************//**
 * \file TestCallbacks.h
 * \author Roman Holderried
 *
 * \brief Dummy callbacks for testing purposes.
 *
 * <b> History </b>
 * 	- 2022-12-13 - File creation
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/******************************************************************************
 * Global variable definition
 *****************************************************************************/

/** \brief Set to true from a test to force SlaveWriteEEROM (TestCallbacks.c)
 *  to fail, exercising the EEPROM-write-failure rollback path. Reset to
 *  false by tests in SetUp()/TearDown() to avoid cross-test leakage. */
extern bool g_forceEEPROMWriteFailure;

/******************************************************************************
 * Function declarations
 *****************************************************************************/
