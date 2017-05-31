#pragma once

/****************************************************************
 *
 * FILENAME
 *
 *   \file mipsfrontend.h
 *
 * PURPOSE
 *
 *   Skeleton for MIPS disassembly.
 *
 * AUTHOR
 *
 *   \author Markus Gothe, nietzsche@lysator.liu.se
 *
 * REVISION
 *
 *   $Id$
 *
 *****************************************************************/

// Class MIPSFrontEnd: derived from FrontEnd, with source machine specific
// behaviour

#include <set>
#include "include/decoder.h"
#include "include/exp.h"      // Ugh... just for enum OPER
#include "include/frontend.h" // In case included bare, e.g. ProcTest.cpp

class FrontEnd;
class MIPSDecoder;
struct DecodeResult;

class CallStatement;

class MIPSFrontEnd : public FrontEnd
{
public:
	MIPSFrontEnd(QObject *pLoader, Prog *prog, BinaryFileFactory *pbff);
	virtual ~MIPSFrontEnd();

	virtual platform getFrontEndId() { return PLAT_MIPS; }

	virtual bool processProc(ADDRESS uAddr, UserProc *pProc, QTextStream& os, bool frag = false, bool spec = false);

	virtual std::vector<SharedExp>& getDefaultParams();

	virtual std::vector<SharedExp>& getDefaultReturns();

	virtual ADDRESS getMainEntryPoint(bool& gotMain);
};