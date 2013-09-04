/*
 * Copyright (C) 2008, 2011, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "VM.h"

#include "ArgList.h"
#include "CallFrameInlines.h"
#include "CodeBlock.h"
#include "CodeCache.h"
#include "CommonIdentifiers.h"
#include "DFGLongLivedState.h"
#include "DFGWorklist.h"
#include "DebuggerActivation.h"
#include "ErrorInstance.h"
#include "FunctionConstructor.h"
#include "GCActivityCallback.h"
#include "GetterSetter.h"
#include "Heap.h"
#include "HostCallReturnValue.h"
#include "Identifier.h"
#include "IncrementalSweeper.h"
#include "Interpreter.h"
#include "JSActivation.h"
#include "JSAPIValueWrapper.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSGlobalObjectFunctions.h"
#include "JSLock.h"
#include "JSNameScope.h"
#include "JSNotAnObject.h"
#include "JSPropertyNameIterator.h"
#include "JSWithScope.h"
#include "Lexer.h"
#include "Lookup.h"
#include "Nodes.h"
#include "ParserArena.h"
#include "RegExpCache.h"
#include "RegExpObject.h"
#include "SimpleTypedArrayController.h"
#include "SourceProviderCache.h"
#include "StrictEvalActivation.h"
#include "StrongInlines.h"
#include "UnlinkedCodeBlock.h"
#include <wtf/ProcessID.h>
#include <wtf/RetainPtr.h>
#include <wtf/StringPrintStream.h>
#include <wtf/Threading.h>
#include <wtf/WTFThreadData.h>

#if ENABLE(DFG_JIT)
#include "ConservativeRoots.h"
#endif

#if ENABLE(REGEXP_TRACING)
#include "RegExp.h"
#endif

#if USE(CF)
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace WTF;

namespace JSC {

extern const HashTable arrayConstructorTable;
extern const HashTable arrayPrototypeTable;
extern const HashTable booleanPrototypeTable;
extern const HashTable jsonTable;
extern const HashTable dataViewTable;
extern const HashTable dateTable;
extern const HashTable dateConstructorTable;
extern const HashTable errorPrototypeTable;
extern const HashTable globalObjectTable;
extern const HashTable numberConstructorTable;
extern const HashTable numberPrototypeTable;
JS_EXPORTDATA extern const HashTable objectConstructorTable;
extern const HashTable privateNamePrototypeTable;
extern const HashTable regExpTable;
extern const HashTable regExpConstructorTable;
extern const HashTable regExpPrototypeTable;
extern const HashTable stringConstructorTable;
extern const HashTable promisePrototypeTable;
extern const HashTable promiseConstructorTable;
extern const HashTable promiseResolverPrototypeTable;

// Note: Platform.h will enforce that ENABLE(ASSEMBLER) is true if either
// ENABLE(JIT) or ENABLE(YARR_JIT) or both are enabled. The code below
// just checks for ENABLE(JIT) or ENABLE(YARR_JIT) with this premise in mind.

#if ENABLE(ASSEMBLER)
static bool enableAssembler(ExecutableAllocator& executableAllocator)
{
    if (!executableAllocator.isValid() || (!Options::useJIT() && !Options::useRegExpJIT()))
        return false;

#if USE(CF)
#if COMPILER(GCC) && !COMPILER(CLANG)
    // FIXME: remove this once the EWS have been upgraded to LLVM.
    // Work around a bug of GCC with strict-aliasing.
    RetainPtr<CFStringRef> canUseJITKeyRetain = adoptCF(CFStringCreateWithCString(0 , "JavaScriptCoreUseJIT", kCFStringEncodingMacRoman));
    CFStringRef canUseJITKey = canUseJITKeyRetain.get();
#else
    CFStringRef canUseJITKey = CFSTR("JavaScriptCoreUseJIT");
#endif // COMPILER(GCC) && !COMPILER(CLANG)
    RetainPtr<CFTypeRef> canUseJIT = adoptCF(CFPreferencesCopyAppValue(canUseJITKey, kCFPreferencesCurrentApplication));
    if (canUseJIT)
        return kCFBooleanTrue == canUseJIT.get();
#endif

#if USE(CF) || OS(UNIX)
    char* canUseJITString = getenv("JavaScriptCoreUseJIT");
    return !canUseJITString || atoi(canUseJITString);
#else
    return true;
#endif
}
#endif // ENABLE(!ASSEMBLER)

VM::VM(VMType vmType, HeapType heapType)
    : m_apiLock(adoptRef(new JSLock(this)))
#if ENABLE(ASSEMBLER)
    , executableAllocator(*this)
#endif
    , heap(this, heapType)
    , vmType(vmType)
    , clientData(0)
    , topCallFrame(CallFrame::noCaller()->removeHostCallFrameFlag())
    , arrayConstructorTable(fastNew<HashTable>(JSC::arrayConstructorTable))
    , arrayPrototypeTable(fastNew<HashTable>(JSC::arrayPrototypeTable))
    , booleanPrototypeTable(fastNew<HashTable>(JSC::booleanPrototypeTable))
    , dataViewTable(fastNew<HashTable>(JSC::dataViewTable))
    , dateTable(fastNew<HashTable>(JSC::dateTable))
    , dateConstructorTable(fastNew<HashTable>(JSC::dateConstructorTable))
    , errorPrototypeTable(fastNew<HashTable>(JSC::errorPrototypeTable))
    , globalObjectTable(fastNew<HashTable>(JSC::globalObjectTable))
    , jsonTable(fastNew<HashTable>(JSC::jsonTable))
    , numberConstructorTable(fastNew<HashTable>(JSC::numberConstructorTable))
    , numberPrototypeTable(fastNew<HashTable>(JSC::numberPrototypeTable))
    , objectConstructorTable(fastNew<HashTable>(JSC::objectConstructorTable))
    , privateNamePrototypeTable(fastNew<HashTable>(JSC::privateNamePrototypeTable))
    , regExpTable(fastNew<HashTable>(JSC::regExpTable))
    , regExpConstructorTable(fastNew<HashTable>(JSC::regExpConstructorTable))
    , regExpPrototypeTable(fastNew<HashTable>(JSC::regExpPrototypeTable))
    , stringConstructorTable(fastNew<HashTable>(JSC::stringConstructorTable))
    , promisePrototypeTable(fastNew<HashTable>(JSC::promisePrototypeTable))
    , promiseConstructorTable(fastNew<HashTable>(JSC::promiseConstructorTable))
    , promiseResolverPrototypeTable(fastNew<HashTable>(JSC::promiseResolverPrototypeTable))
    , identifierTable(vmType == Default ? wtfThreadData().currentIdentifierTable() : createIdentifierTable())
    , propertyNames(new CommonIdentifiers(this))
    , emptyList(new MarkedArgumentBuffer)
    , parserArena(adoptPtr(new ParserArena))
    , keywords(adoptPtr(new Keywords(this)))
    , interpreter(0)
    , jsArrayClassInfo(JSArray::info())
    , jsFinalObjectClassInfo(JSFinalObject::info())
#if ENABLE(DFG_JIT)
    , sizeOfLastScratchBuffer(0)
#endif
    , dynamicGlobalObject(0)
    , m_enabledProfiler(0)
    , m_regExpCache(new RegExpCache(this))
#if ENABLE(REGEXP_TRACING)
    , m_rtTraceList(new RTTraceList())
#endif
    , exclusiveThread(0)
    , m_newStringsSinceLastHashCons(0)
#if ENABLE(ASSEMBLER)
    , m_canUseAssembler(enableAssembler(executableAllocator))
#endif
#if ENABLE(JIT)
    , m_canUseJIT(m_canUseAssembler && Options::useJIT())
#endif
#if ENABLE(YARR_JIT)
    , m_canUseRegExpJIT(m_canUseAssembler && Options::useRegExpJIT())
#endif
#if ENABLE(GC_VALIDATION)
    , m_initializingObjectClass(0)
#endif
    , m_inDefineOwnProperty(false)
    , m_codeCache(CodeCache::create())
{
    interpreter = new Interpreter(*this);

    // Need to be careful to keep everything consistent here
    JSLockHolder lock(this);
    IdentifierTable* existingEntryIdentifierTable = wtfThreadData().setCurrentIdentifierTable(identifierTable);
    structureStructure.set(*this, Structure::createStructure(*this));
    structureRareDataStructure.set(*this, StructureRareData::createStructure(*this, 0, jsNull()));
    debuggerActivationStructure.set(*this, DebuggerActivation::createStructure(*this, 0, jsNull()));
    terminatedExecutionErrorStructure.set(*this, TerminatedExecutionError::createStructure(*this, 0, jsNull()));
    stringStructure.set(*this, JSString::createStructure(*this, 0, jsNull()));
    notAnObjectStructure.set(*this, JSNotAnObject::createStructure(*this, 0, jsNull()));
    propertyNameIteratorStructure.set(*this, JSPropertyNameIterator::createStructure(*this, 0, jsNull()));
    getterSetterStructure.set(*this, GetterSetter::createStructure(*this, 0, jsNull()));
    apiWrapperStructure.set(*this, JSAPIValueWrapper::createStructure(*this, 0, jsNull()));
    JSScopeStructure.set(*this, JSScope::createStructure(*this, 0, jsNull()));
    executableStructure.set(*this, ExecutableBase::createStructure(*this, 0, jsNull()));
    nativeExecutableStructure.set(*this, NativeExecutable::createStructure(*this, 0, jsNull()));
    evalExecutableStructure.set(*this, EvalExecutable::createStructure(*this, 0, jsNull()));
    programExecutableStructure.set(*this, ProgramExecutable::createStructure(*this, 0, jsNull()));
    functionExecutableStructure.set(*this, FunctionExecutable::createStructure(*this, 0, jsNull()));
    regExpStructure.set(*this, RegExp::createStructure(*this, 0, jsNull()));
    sharedSymbolTableStructure.set(*this, SharedSymbolTable::createStructure(*this, 0, jsNull()));
    structureChainStructure.set(*this, StructureChain::createStructure(*this, 0, jsNull()));
    sparseArrayValueMapStructure.set(*this, SparseArrayValueMap::createStructure(*this, 0, jsNull()));
    withScopeStructure.set(*this, JSWithScope::createStructure(*this, 0, jsNull()));
    unlinkedFunctionExecutableStructure.set(*this, UnlinkedFunctionExecutable::createStructure(*this, 0, jsNull()));
    unlinkedProgramCodeBlockStructure.set(*this, UnlinkedProgramCodeBlock::createStructure(*this, 0, jsNull()));
    unlinkedEvalCodeBlockStructure.set(*this, UnlinkedEvalCodeBlock::createStructure(*this, 0, jsNull()));
    unlinkedFunctionCodeBlockStructure.set(*this, UnlinkedFunctionCodeBlock::createStructure(*this, 0, jsNull()));
    propertyTableStructure.set(*this, PropertyTable::createStructure(*this, 0, jsNull()));
    smallStrings.initializeCommonStrings(*this);

    wtfThreadData().setCurrentIdentifierTable(existingEntryIdentifierTable);

#if ENABLE(JIT)
    jitStubs = adoptPtr(new JITThunks());
    performPlatformSpecificJITAssertions(this);
#endif
    
    interpreter->initialize(this->canUseJIT());
    
#if ENABLE(JIT)
    initializeHostCallReturnValue(); // This is needed to convince the linker not to drop host call return support.
#endif

    heap.notifyIsSafeToCollect();

    LLInt::Data::performAssertions(*this);
    
    if (Options::enableProfiler()) {
        m_perBytecodeProfiler = adoptPtr(new Profiler::Database(*this));

        StringPrintStream pathOut;
#if !OS(WINCE)
        const char* profilerPath = getenv("JSC_PROFILER_PATH");
        if (profilerPath)
            pathOut.print(profilerPath, "/");
#endif
        pathOut.print("JSCProfile-", getCurrentProcessID(), "-", m_perBytecodeProfiler->databaseID(), ".json");
        m_perBytecodeProfiler->registerToSaveAtExit(pathOut.toCString().data());
    }

#if ENABLE(DFG_JIT)
    if (canUseJIT())
        dfgState = adoptPtr(new DFG::LongLivedState());
#endif
    
    // Initialize this last, as a free way of asserting that VM initialization itself
    // won't use this.
    m_typedArrayController = adoptRef(new SimpleTypedArrayController());
}

VM::~VM()
{
    // Never GC, ever again.
    heap.incrementDeferralDepth();
    
#if ENABLE(DFG_JIT)
    // Make sure concurrent compilations are done, but don't install them, since there is
    // no point to doing so.
    if (worklist) {
        worklist->waitUntilAllPlansForVMAreReady(*this);
        worklist->removeAllReadyPlansForVM(*this);
    }
#endif // ENABLE(DFG_JIT)
    
    // Clear this first to ensure that nobody tries to remove themselves from it.
    m_perBytecodeProfiler.clear();
    
    ASSERT(m_apiLock->currentThreadIsHoldingLock());
    m_apiLock->willDestroyVM(this);
    heap.lastChanceToFinalize();

    delete interpreter;
#ifndef NDEBUG
    interpreter = reinterpret_cast<Interpreter*>(0xbbadbeef);
#endif

    arrayPrototypeTable->deleteTable();
    arrayConstructorTable->deleteTable();
    booleanPrototypeTable->deleteTable();
    dataViewTable->deleteTable();
    dateTable->deleteTable();
    dateConstructorTable->deleteTable();
    errorPrototypeTable->deleteTable();
    globalObjectTable->deleteTable();
    jsonTable->deleteTable();
    numberConstructorTable->deleteTable();
    numberPrototypeTable->deleteTable();
    objectConstructorTable->deleteTable();
    privateNamePrototypeTable->deleteTable();
    regExpTable->deleteTable();
    regExpConstructorTable->deleteTable();
    regExpPrototypeTable->deleteTable();
    stringConstructorTable->deleteTable();
    promisePrototypeTable->deleteTable();
    promiseConstructorTable->deleteTable();
    promiseResolverPrototypeTable->deleteTable();

    fastDelete(const_cast<HashTable*>(arrayConstructorTable));
    fastDelete(const_cast<HashTable*>(arrayPrototypeTable));
    fastDelete(const_cast<HashTable*>(booleanPrototypeTable));
    fastDelete(const_cast<HashTable*>(dataViewTable));
    fastDelete(const_cast<HashTable*>(dateTable));
    fastDelete(const_cast<HashTable*>(dateConstructorTable));
    fastDelete(const_cast<HashTable*>(errorPrototypeTable));
    fastDelete(const_cast<HashTable*>(globalObjectTable));
    fastDelete(const_cast<HashTable*>(jsonTable));
    fastDelete(const_cast<HashTable*>(numberConstructorTable));
    fastDelete(const_cast<HashTable*>(numberPrototypeTable));
    fastDelete(const_cast<HashTable*>(objectConstructorTable));
    fastDelete(const_cast<HashTable*>(privateNamePrototypeTable));
    fastDelete(const_cast<HashTable*>(regExpTable));
    fastDelete(const_cast<HashTable*>(regExpConstructorTable));
    fastDelete(const_cast<HashTable*>(regExpPrototypeTable));
    fastDelete(const_cast<HashTable*>(stringConstructorTable));
    fastDelete(const_cast<HashTable*>(promisePrototypeTable));
    fastDelete(const_cast<HashTable*>(promiseConstructorTable));
    fastDelete(const_cast<HashTable*>(promiseResolverPrototypeTable));

    delete emptyList;

    delete propertyNames;
    if (vmType != Default)
        deleteIdentifierTable(identifierTable);

    delete clientData;
    delete m_regExpCache;
#if ENABLE(REGEXP_TRACING)
    delete m_rtTraceList;
#endif

#if ENABLE(DFG_JIT)
    for (unsigned i = 0; i < scratchBuffers.size(); ++i)
        fastFree(scratchBuffers[i]);
#endif
}

PassRefPtr<VM> VM::createContextGroup(HeapType heapType)
{
    return adoptRef(new VM(APIContextGroup, heapType));
}

PassRefPtr<VM> VM::create(HeapType heapType)
{
    return adoptRef(new VM(Default, heapType));
}

PassRefPtr<VM> VM::createLeaked(HeapType heapType)
{
    return create(heapType);
}

bool VM::sharedInstanceExists()
{
    return sharedInstanceInternal();
}

VM& VM::sharedInstance()
{
    GlobalJSLock globalLock;
    VM*& instance = sharedInstanceInternal();
    if (!instance) {
        instance = adoptRef(new VM(APIShared, SmallHeap)).leakRef();
        instance->makeUsableFromMultipleThreads();
    }
    return *instance;
}

VM*& VM::sharedInstanceInternal()
{
    static VM* sharedInstance;
    return sharedInstance;
}

#if ENABLE(JIT)
static ThunkGenerator thunkGeneratorForIntrinsic(Intrinsic intrinsic)
{
    switch (intrinsic) {
    case CharCodeAtIntrinsic:
        return charCodeAtThunkGenerator;
    case CharAtIntrinsic:
        return charAtThunkGenerator;
    case FromCharCodeIntrinsic:
        return fromCharCodeThunkGenerator;
    case SqrtIntrinsic:
        return sqrtThunkGenerator;
    case PowIntrinsic:
        return powThunkGenerator;
    case AbsIntrinsic:
        return absThunkGenerator;
    case FloorIntrinsic:
        return floorThunkGenerator;
    case CeilIntrinsic:
        return ceilThunkGenerator;
    case RoundIntrinsic:
        return roundThunkGenerator;
    case ExpIntrinsic:
        return expThunkGenerator;
    case LogIntrinsic:
        return logThunkGenerator;
    case IMulIntrinsic:
        return imulThunkGenerator;
    default:
        return 0;
    }
}

NativeExecutable* VM::getHostFunction(NativeFunction function, NativeFunction constructor)
{
    return jitStubs->hostFunctionStub(this, function, constructor);
}
NativeExecutable* VM::getHostFunction(NativeFunction function, Intrinsic intrinsic)
{
    ASSERT(canUseJIT());
    return jitStubs->hostFunctionStub(this, function, intrinsic != NoIntrinsic ? thunkGeneratorForIntrinsic(intrinsic) : 0, intrinsic);
}

#else // !ENABLE(JIT)
NativeExecutable* VM::getHostFunction(NativeFunction function, NativeFunction constructor)
{
    return NativeExecutable::create(*this, function, constructor);
}
#endif // !ENABLE(JIT)

VM::ClientData::~ClientData()
{
}

void VM::resetDateCache()
{
    localTimeOffsetCache.reset();
    cachedDateString = String();
    cachedDateStringValue = QNaN;
    dateInstanceCache.reset();
}

void VM::startSampling()
{
    interpreter->startSampling();
}

void VM::stopSampling()
{
    interpreter->stopSampling();
}

void VM::prepareToDiscardCode()
{
#if ENABLE(DFG_JIT)
    if (!worklist)
        return;
    
    worklist->completeAllPlansForVM(*this);
#endif
}

void VM::discardAllCode()
{
    prepareToDiscardCode();
    m_codeCache->clear();
    heap.deleteAllCompiledCode();
    heap.reportAbandonedObjectGraph();
}

void VM::dumpSampleData(ExecState* exec)
{
    interpreter->dumpSampleData(exec);
#if ENABLE(ASSEMBLER)
    ExecutableAllocator::dumpProfile();
#endif
}

SourceProviderCache* VM::addSourceProviderCache(SourceProvider* sourceProvider)
{
    SourceProviderCacheMap::AddResult addResult = sourceProviderCacheMap.add(sourceProvider, 0);
    if (addResult.isNewEntry)
        addResult.iterator->value = adoptRef(new SourceProviderCache);
    return addResult.iterator->value.get();
}

void VM::clearSourceProviderCaches()
{
    sourceProviderCacheMap.clear();
}

struct StackPreservingRecompiler : public MarkedBlock::VoidFunctor {
    HashSet<FunctionExecutable*> currentlyExecutingFunctions;
    void operator()(JSCell* cell)
    {
        if (!cell->inherits(FunctionExecutable::info()))
            return;
        FunctionExecutable* executable = jsCast<FunctionExecutable*>(cell);
        if (currentlyExecutingFunctions.contains(executable))
            return;
        executable->clearCodeIfNotCompiling();
    }
};

void VM::releaseExecutableMemory()
{
    prepareToDiscardCode();
    
    if (dynamicGlobalObject) {
        StackPreservingRecompiler recompiler;
        HashSet<JSCell*> roots;
        heap.canonicalizeCellLivenessData();
        heap.getConservativeRegisterRoots(roots);
        HashSet<JSCell*>::iterator end = roots.end();
        for (HashSet<JSCell*>::iterator ptr = roots.begin(); ptr != end; ++ptr) {
            ScriptExecutable* executable = 0;
            JSCell* cell = *ptr;
            if (cell->inherits(ScriptExecutable::info()))
                executable = static_cast<ScriptExecutable*>(*ptr);
            else if (cell->inherits(JSFunction::info())) {
                JSFunction* function = jsCast<JSFunction*>(*ptr);
                if (function->isHostFunction())
                    continue;
                executable = function->jsExecutable();
            } else
                continue;
            ASSERT(executable->inherits(ScriptExecutable::info()));
            executable->unlinkCalls();
            if (executable->inherits(FunctionExecutable::info()))
                recompiler.currentlyExecutingFunctions.add(static_cast<FunctionExecutable*>(executable));
                
        }
        heap.objectSpace().forEachLiveCell<StackPreservingRecompiler>(recompiler);
    }
    m_regExpCache->invalidateCode();
    heap.collectAllGarbage();
}

static void appendSourceToError(CallFrame* callFrame, ErrorInstance* exception, unsigned bytecodeOffset)
{
    exception->clearAppendSourceToMessage();
    
    if (!callFrame->codeBlock()->hasExpressionInfo())
        return;
    
    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    unsigned line = 0;
    unsigned column = 0;
    
    CodeBlock* codeBlock = callFrame->codeBlock();
    codeBlock->expressionRangeForBytecodeOffset(bytecodeOffset, divotPoint, startOffset, endOffset, line, column);
    
    int expressionStart = divotPoint - startOffset;
    int expressionStop = divotPoint + endOffset;
    
    const String& sourceString = codeBlock->source()->source();
    if (!expressionStop || expressionStart > static_cast<int>(sourceString.length()))
        return;
    
    VM* vm = &callFrame->vm();
    JSValue jsMessage = exception->getDirect(*vm, vm->propertyNames->message);
    if (!jsMessage || !jsMessage.isString())
        return;
    
    String message = asString(jsMessage)->value(callFrame);
    
    if (expressionStart < expressionStop)
        message =  makeString(message, " (evaluating '", codeBlock->source()->getRange(expressionStart, expressionStop), "')");
    else {
        // No range information, so give a few characters of context.
        const StringImpl* data = sourceString.impl();
        int dataLength = sourceString.length();
        int start = expressionStart;
        int stop = expressionStart;
        // Get up to 20 characters of context to the left and right of the divot, clamping to the line.
        // Then strip whitespace.
        while (start > 0 && (expressionStart - start < 20) && (*data)[start - 1] != '\n')
            start--;
        while (start < (expressionStart - 1) && isStrWhiteSpace((*data)[start]))
            start++;
        while (stop < dataLength && (stop - expressionStart < 20) && (*data)[stop] != '\n')
            stop++;
        while (stop > expressionStart && isStrWhiteSpace((*data)[stop - 1]))
            stop--;
        message = makeString(message, " (near '...", codeBlock->source()->getRange(start, stop), "...')");
    }
    
    exception->putDirect(*vm, vm->propertyNames->message, jsString(vm, message));
}
    
JSValue VM::throwException(ExecState* exec, JSValue error)
{
    ASSERT(exec == topCallFrame || exec == exec->lexicalGlobalObject()->globalExec() || exec == exec->dynamicGlobalObject()->globalExec());
    
    Vector<StackFrame> stackTrace;
    interpreter->getStackTrace(stackTrace);
    m_exceptionStack = RefCountedArray<StackFrame>(stackTrace);
    m_exception = error;
    
    if (stackTrace.isEmpty() || !error.isObject())
        return error;
    JSObject* exception = asObject(error);
    
    StackFrame stackFrame;
    for (unsigned i = 0 ; i < stackTrace.size(); ++i) {
        stackFrame = stackTrace.at(i);
        if (stackFrame.bytecodeOffset)
            break;
    }
    unsigned bytecodeOffset = stackFrame.bytecodeOffset;
    if (!hasErrorInfo(exec, exception)) {
        // FIXME: We should only really be adding these properties to VM generated exceptions,
        // but the inspector currently requires these for all thrown objects.
        unsigned line;
        unsigned column;
        stackFrame.computeLineAndColumn(line, column);
        exception->putDirect(*this, Identifier(this, "line"), jsNumber(line), ReadOnly | DontDelete);
        exception->putDirect(*this, Identifier(this, "column"), jsNumber(column), ReadOnly | DontDelete);
        if (!stackFrame.sourceURL.isEmpty())
            exception->putDirect(*this, Identifier(this, "sourceURL"), jsString(this, stackFrame.sourceURL), ReadOnly | DontDelete);
    }
    if (exception->isErrorInstance() && static_cast<ErrorInstance*>(exception)->appendSourceToMessage()) {
        unsigned stackIndex = 0;
        CallFrame* callFrame;
        for (callFrame = exec; callFrame && !callFrame->codeBlock(); callFrame = callFrame->callerFrame()->removeHostCallFrameFlag())
            stackIndex++;
        stackFrame = stackTrace.at(stackIndex);
        bytecodeOffset = stackFrame.bytecodeOffset;
        appendSourceToError(callFrame, static_cast<ErrorInstance*>(exception), bytecodeOffset);
    }

    if (exception->hasProperty(exec, this->propertyNames->stack))
        return error;
    
    exception->putDirect(*this, propertyNames->stack, interpreter->stackTraceAsString(topCallFrame, stackTrace), DontEnum);
    return error;
}
    
JSObject* VM::throwException(ExecState* exec, JSObject* error)
{
    return asObject(throwException(exec, JSValue(error)));
}
void VM::getExceptionInfo(JSValue& exception, RefCountedArray<StackFrame>& exceptionStack)
{
    exception = m_exception;
    exceptionStack = m_exceptionStack;
}
void VM::setExceptionInfo(JSValue& exception, RefCountedArray<StackFrame>& exceptionStack)
{
    m_exception = exception;
    m_exceptionStack = exceptionStack;
}

void VM::clearException()
{
    m_exception = JSValue();
}
void VM:: clearExceptionStack()
{
    m_exceptionStack = RefCountedArray<StackFrame>();
}

void releaseExecutableMemory(VM& vm)
{
    vm.releaseExecutableMemory();
}

#if ENABLE(DFG_JIT)
void VM::gatherConservativeRoots(ConservativeRoots& conservativeRoots)
{
    for (size_t i = 0; i < scratchBuffers.size(); i++) {
        ScratchBuffer* scratchBuffer = scratchBuffers[i];
        if (scratchBuffer->activeLength()) {
            void* bufferStart = scratchBuffer->dataBuffer();
            conservativeRoots.add(bufferStart, static_cast<void*>(static_cast<char*>(bufferStart) + scratchBuffer->activeLength()));
        }
    }
}
#endif

#if ENABLE(REGEXP_TRACING)
void VM::addRegExpToTrace(RegExp* regExp)
{
    m_rtTraceList->add(regExp);
}

void VM::dumpRegExpTrace()
{
    // The first RegExp object is ignored.  It is create by the RegExpPrototype ctor and not used.
    RTTraceList::iterator iter = ++m_rtTraceList->begin();
    
    if (iter != m_rtTraceList->end()) {
        dataLogF("\nRegExp Tracing\n");
        dataLogF("                                                            match()    matches\n");
        dataLogF("Regular Expression                          JIT Address      calls      found\n");
        dataLogF("----------------------------------------+----------------+----------+----------\n");
    
        unsigned reCount = 0;
    
        for (; iter != m_rtTraceList->end(); ++iter, ++reCount)
            (*iter)->printTraceData();

        dataLogF("%d Regular Expressions\n", reCount);
    }
    
    m_rtTraceList->clear();
}
#else
void VM::dumpRegExpTrace()
{
}
#endif

} // namespace JSC
