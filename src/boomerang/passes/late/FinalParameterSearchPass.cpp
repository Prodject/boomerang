#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License
#include "FinalParameterSearchPass.h"

#include "boomerang/db/proc/UserProc.h"
#include "boomerang/db/signature/Signature.h"
#include "boomerang/ssl/exp/Location.h"
#include "boomerang/ssl/exp/RefExp.h"
#include "boomerang/ssl/statements/ImplicitAssign.h"
#include "boomerang/util/log/Log.h"
#include "boomerang/visitor/expmodifier/ImplicitConverter.h"

#include <QtAlgorithms>


FinalParameterSearchPass::FinalParameterSearchPass()
    : IPass("FinalParameterUpdate", PassID::FinalParameterSearch)
{
}


bool FinalParameterSearchPass::execute(UserProc *proc)
{
    qDeleteAll(proc->getParameters());
    proc->getParameters().clear();

    if (proc->getSignature()->isForced()) {
        // Copy from signature
        int n = proc->getSignature()->getNumParams();
        ImplicitConverter ic(proc->getCFG());

        for (int i = 0; i < n; ++i) {
            SharedExp paramLoc = proc->getSignature()->getParamExp(i)->clone(); // E.g. m[r28 + 4]
            LocationSet components;
            paramLoc->addUsedLocs(components);

            for (const SharedExp &component : components) {
                if (component != paramLoc) {                       // Don't subscript outer level
                    paramLoc->expSubscriptVar(component, nullptr); // E.g. r28 -> r28{-}
                    paramLoc->acceptModifier(&ic);                 // E.g. r28{-} -> r28{0}
                }
            }

            proc->getParameters().append(
                new ImplicitAssign(proc->getSignature()->getParamType(i), paramLoc));
            QString name         = proc->getSignature()->getParamName(i);
            SharedExp param      = Location::param(name, proc);
            SharedExp reParamLoc = RefExp::get(
                paramLoc, proc->getCFG()->findOrCreateImplicitAssign(paramLoc));
            proc->mapSymbolTo(reParamLoc, param); // Update name map
        }

        return true;
    }

    if (DEBUG_PARAMS) {
        LOG_VERBOSE("Finding final parameters for %1", getName());
    }

    //    int sp = signature->getStackRegister();
    proc->getSignature()->setNumParams(0); // Clear any old ideas
    StatementList stmts;
    proc->getStatements(stmts);

    for (Statement *s : stmts) {
        // Assume that all parameters will be m[]{0} or r[]{0}, and in the implicit definitions at
        // the start of the program
        if (!s->isImplicit()) {
            // Note: phis can get converted to assignments, but I hope that this is only later on:
            // check this!
            break; // Stop after reading all implicit assignments
        }

        SharedExp e = static_cast<ImplicitAssign *>(s)->getLeft();

        if (proc->getSignature()->findParam(e) == -1) {
            if (DEBUG_PARAMS) {
                LOG_VERBOSE("Potential param %1", e);
            }

            // I believe that the only true parameters will be registers or memofs that look like
            // locals (stack pararameters)
            if (!(e->isRegOf() || proc->isLocalOrParamPattern(e))) {
                continue;
            }

            if (DEBUG_PARAMS) {
                LOG_VERBOSE("Found new parameter %1", e);
            }

            SharedType ty = static_cast<ImplicitAssign *>(s)->getType();
            // Add this parameter to the signature (for now; creates parameter names)
            proc->addParameterToSignature(e, ty);
            // Insert it into the parameters StatementList, in sensible order
            proc->insertParameter(e, ty);
        }
    }

    return true;
}
