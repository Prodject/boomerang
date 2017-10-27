#pragma region License
/*
 * This file is part of the Boomerang Decompiler.
 *
 * See the file "LICENSE.TERMS" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 */
#pragma endregion License
#pragma once


/**
 * \file       proc.h
 * OVERVIEW:   Interface for the procedure classes, which are used to store information about variables in the
 *                procedure such as parameters and locals.
 */

#include "boomerang/util/Address.h"
#include "boomerang/db/Signature.h"

#include <QString>

class Module;
class CallStatement;
class Prog;

/**
 * Interface for the procedure classes, which are used to store information
 * about variables in the procedure such as parameters and locals.
 */
class Function : public Printable
{
    typedef std::map<SharedExp, SharedExp, lessExpStar> ExpExpMap;

public:
    /**
     * \param address   Address of entry point of procedure
     * \param signature The Signature for this Proc
     * \param module    The Module this procedure belongs to
     */
    Function(Address address, Signature *signature, Module *module);
    virtual ~Function();

    /// Get the name of this procedure.
    QString getName() const;

    /// Rename this procedure.
    void setName(const QString& nam);

    /// Get the address of the entry point of this procedure.
    Address getEntryAddress() const;

    /// Set the entry address of this procedure
    void setEntryAddress(Address addr);

    /// Get the program this procedure belongs to.
    Prog *getProg() const { return m_prog; }
    void setProg(Prog *p) { m_prog = p; }

    /// Get the first procedure that calls this procedure (or null for main/start).
    Function *getFirstCaller();

    /// Set the first procedure that calls this procedure (or null for main/start).
    void setFirstCaller(Function *p)
    {
        if (m_firstCaller == nullptr) {
            m_firstCaller = p;
        }
    }

    std::shared_ptr<Signature> getSignature() const { return m_signature; }
    void setSignature(std::shared_ptr<Signature> sig) { m_signature = sig; }

    void eraseFromParent();

    virtual void renameParam(const char *oldName, const char *newName);

    /**
     * Modify actuals so that it is now the list of locations that must
     * be passed to this procedure. The modification will be to either add
     * dummy locations to actuals, delete from actuals, or leave it
     * unchanged.
     * Add "dummy" params: this will be required when there are
     *     less live outs at a call site than the number of parameters
     *     expected by the procedure called. This will be a result of
     *     one of two things:
     *     i) a value returned by a preceeding call is used as a
     *        parameter and as such is not detected as defined by the
     *        procedure. E.g.:
     *
     *           foo(bar(x));
     *
     *        Here, the result of bar(x) is used as the first and only
     *        parameter to foo. On some architectures (such as SPARC),
     *        the location used as the first parameter (e.g. %o0) is
     *        also the location in which a value is returned. So, the
     *        call to bar defines this location implicitly as shown in
     *        the following SPARC assembly that may be generated by from
     *        the above code:
     *
     *            mov      x, %o0
     *            call  bar
     *            nop
     *            call  foo
     *
     *       As can be seen, there is no definition of %o0 after the
     *       call to bar and before the call to foo. Adding the integer
     *       return location is therefore a good guess for the dummy
     *       location to add (but may occasionally be wrong).
     *
     *    ii) uninitialised variables are used as parameters to a call
     *
     *    Note that both of these situations can only occur on
     *    architectures such as SPARC that use registers for parameter
     *    passing. Stack parameters must always be pushed so that the
     *    callee doesn't access the caller's non-parameter portion of
     *    stack.
     *
     * This used to be a virtual function, implemented differenty for
     * LibProcs and for UserProcs. But in fact, both need the exact same
     * treatment; the only difference is how the local member "parameters"
     * is set (from common.hs in the case of LibProc objects, or from analysis
     * in the case of UserProcs).
     * \todo Not implemented nor used yet
     */
    void matchParams(std::list<SharedExp>&, UserProc&);

    /**
     * Get a list of types to cast a given list of actual parameters to
     */
    std::list<Type> *getParamTypeList(const std::list<SharedExp>&);

    virtual bool isLib() const { return false; } ///< Return true if this is a library proc
    virtual bool isNoReturn() const = 0;         ///< Return true if this procedure doesn't return

    /**
     * Output operator for a Proc object.
     */
    friend QTextStream& operator<<(QTextStream& os, const Function& proc);

    /// Get the RHS that is proven for left
    virtual SharedExp getProven(SharedExp left)   = 0; ///< Get the RHS, if any, that is proven for left
    virtual SharedExp getPremised(SharedExp left) = 0; ///< Get the RHS, if any, that is premised for left
    virtual bool isPreserved(SharedExp e)         = 0; ///< Return whether e is preserved by this proc

    /// Set an equation as proven. Useful for some sorts of testing
    void setProvenTrue(SharedExp fact);

    /**
     * Get the callers
     * Note: the callers will be in a random order (determined by memory allocation)
     */
    std::set<CallStatement *>& getCallers() { return m_callerSet; }

    /// Add to the set of callers
    void addCaller(CallStatement *caller) { m_callerSet.insert(caller); }
    void addCallers(std::set<UserProc *>& callers);

    void removeParameter(SharedExp e);
    virtual void removeReturn(SharedExp e);

    virtual void printCallGraphXML(QTextStream& os, int depth, bool = true);
    void printDetailsXML();

    void clearVisited() { m_visited = false; }
    bool isVisited() const { return m_visited; }

    Module *getParent() { return m_module; }
    const Module *getParent() const { return m_module; }

    void setParent(Module *c);
    void removeFromParent();

protected:
    /// Delete the CFG from this function.
    virtual void deleteCFG() {}

protected:
    bool m_visited; ///< For printCallGraphXML
    Prog *m_prog;   ///< Program containing this procedure.

    /**
     * The formal signature of this procedure.
     * This information is determined either by the common.hs file (for a library function) or by analysis.
     * \note This belongs in the CALL, because the same procedure can have different
     * signatures if it happens to have varargs. Temporarily here till it can be permanently
     * moved.
     */
    std::shared_ptr<Signature> m_signature;

    ///////////////////////////////////////////////////
    // Persistent state
    ///////////////////////////////////////////////////
    Address m_entryAddress;    ///< Entry address of this procedure.
    Function *m_firstCaller;   ///< first procedure to call this procedure.
    Address m_firstCallerAddr; ///< can only be used once.

    /**
     * All the expressions that have been proven true.
     * (Could perhaps do with a list of some that are proven false)
     * Proof the form r28 = r28 + 4 is stored as map from "r28" to "r28+4" (NOTE: no subscripts)
     * FIXME: shouldn't provenTrue be in UserProc, with logic associated with the signature doing the equivalent thing
     * for LibProcs?
     */
    ExpExpMap m_provenTrue;
    // Cache of queries proven false (to save time)
    // mExpExp provenFalse;

    /**
     * Premises for recursion group analysis. This is a preservation
     * that is assumed true only for definitions by calls reached in the proof. It also
     * prevents infinite looping of this proof logic.
     */
    ExpExpMap m_recurPremises;
    std::set<CallStatement *> m_callerSet; ///< Set of callers (CallStatements that call this procedure).
    Module *m_module;
};
