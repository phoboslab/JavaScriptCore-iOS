# -------------------------------------------------------------------
# Target file for the JavaScriptSource library
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = lib
TARGET = JavaScriptCore

include(JavaScriptCore.pri)

WEBKIT += wtf
QT += core
QT -= gui

CONFIG += staticlib

*-g++*:QMAKE_CXXFLAGS_RELEASE -= -O2
*-g++*:QMAKE_CXXFLAGS_RELEASE += -O3

# Rules when JIT enabled (not disabled)
!contains(DEFINES, ENABLE_JIT=0) {
    linux*-g++*:greaterThan(QT_GCC_MAJOR_VERSION,3):greaterThan(QT_GCC_MINOR_VERSION,0) {
        QMAKE_CXXFLAGS += -fno-stack-protector
        QMAKE_CFLAGS += -fno-stack-protector
    }
}

include(yarr/yarr.pri)

INSTALLDEPS += all

debug_and_release: INCLUDEPATH += $$JAVASCRIPTCORE_GENERATED_SOURCES_DIR/$$targetSubDir()

SOURCES += \
    API/JSBase.cpp \
    API/JSCallbackConstructor.cpp \
    API/JSCallbackFunction.cpp \
    API/JSCallbackObject.cpp \
    API/JSClassRef.cpp \
    API/JSContextRef.cpp \
    API/JSObjectRef.cpp \
    API/JSScriptRef.cpp \
    API/JSStringRef.cpp \
    API/JSStringRefQt.cpp \
    API/JSValueRef.cpp \
    API/JSWeakObjectMapRefPrivate.cpp \
    API/OpaqueJSString.cpp \
    assembler/ARMAssembler.cpp \
    assembler/ARMv7Assembler.cpp \
    assembler/LinkBuffer.cpp \
    assembler/MacroAssembler.cpp \
    assembler/MacroAssemblerARM.cpp \
    assembler/MacroAssemblerX86Common.cpp \
    bytecode/ArrayAllocationProfile.cpp \
    bytecode/ArrayProfile.cpp \
    bytecode/CallLinkInfo.cpp \
    bytecode/CallLinkStatus.cpp \
    bytecode/CodeBlock.cpp \
    bytecode/CodeBlockHash.cpp \
    bytecode/CodeOrigin.cpp \
    bytecode/CodeType.cpp \
    bytecode/DFGExitProfile.cpp \
    bytecode/DeferredCompilationCallback.cpp \
    bytecode/ExecutionCounter.cpp \
    bytecode/ExitKind.cpp \
    bytecode/GetByIdStatus.cpp \
    bytecode/JumpTable.cpp \
    bytecode/LazyOperandValueProfile.cpp \
    bytecode/MethodOfGettingAValueProfile.cpp \
    bytecode/Opcode.cpp \
    bytecode/PolymorphicPutByIdList.cpp \
    bytecode/PreciseJumpTargets.cpp \
    bytecode/PutByIdStatus.cpp \
    bytecode/ReduceWhitespace.cpp \
    bytecode/SamplingTool.cpp \
    bytecode/SpecialPointer.cpp \
    bytecode/SpeculatedType.cpp \
    bytecode/StructureStubClearingWatchpoint.cpp \
    bytecode/StructureStubInfo.cpp \
    bytecode/UnlinkedCodeBlock.cpp \
    bytecode/Watchpoint.cpp \
    bytecompiler/BytecodeGenerator.cpp \
    bytecompiler/NodesCodegen.cpp \
    heap/CopiedSpaceInlines.h \
    heap/CopiedSpace.cpp \
    heap/CopyVisitor.cpp \
    heap/ConservativeRoots.cpp \
    heap/DFGCodeBlocks.cpp \
    heap/Weak.cpp \
    heap/WeakBlock.cpp \
    heap/WeakHandleOwner.cpp \
    heap/WeakSet.cpp \
    heap/HandleSet.cpp \
    heap/HandleStack.cpp \
    heap/BlockAllocator.cpp \
    heap/GCThreadSharedData.cpp \
    heap/GCThread.cpp \
    heap/Heap.cpp \
    heap/HeapStatistics.cpp \
    heap/HeapTimer.cpp \
    heap/IncrementalSweeper.cpp \
    heap/JITStubRoutineSet.cpp \
    heap/MachineStackMarker.cpp \
    heap/MarkStack.cpp \
    heap/MarkedAllocator.cpp \
    heap/MarkedBlock.cpp \
    heap/MarkedSpace.cpp \
    heap/SlotVisitor.cpp \
    heap/SuperRegion.cpp \
    heap/VTableSpectrum.cpp \
    heap/WriteBarrierSupport.cpp \
    debugger/DebuggerActivation.cpp \
    debugger/DebuggerCallFrame.cpp \
    debugger/Debugger.cpp \
    dfg/DFGAbstractHeap.cpp \
    dfg/DFGAbstractValue.cpp \
    dfg/DFGArgumentsSimplificationPhase.cpp \
    dfg/DFGArrayMode.cpp \
    dfg/DFGAssemblyHelpers.cpp \
    dfg/DFGAtTailAbstractState.cpp \
    dfg/DFGBackwardsPropagationPhase.cpp \
    dfg/DFGBasicBlock.cpp \
    dfg/DFGBinarySwitch.cpp \
    dfg/DFGBlockInsertionSet.cpp \
    dfg/DFGByteCodeParser.cpp \
    dfg/DFGCapabilities.cpp \
    dfg/DFGClobberize.cpp \
    dfg/DFGClobberSet.cpp \
    dfg/DFGCommon.cpp \
    dfg/DFGCommonData.cpp \
    dfg/DFGCFAPhase.cpp \
    dfg/DFGCFGSimplificationPhase.cpp \
    dfg/DFGCPSRethreadingPhase.cpp \
    dfg/DFGConstantFoldingPhase.cpp \
    dfg/DFGCriticalEdgeBreakingPhase.cpp \
    dfg/DFGCSEPhase.cpp \
    dfg/DFGDCEPhase.cpp \
    dfg/DFGDesiredIdentifiers.cpp \
    dfg/DFGDesiredStructureChains.cpp \
    dfg/DFGDesiredTransitions.cpp \
    dfg/DFGDesiredWatchpoints.cpp \
    dfg/DFGDesiredWeakReferences.cpp \
    dfg/DFGDesiredWriteBarriers.cpp \
    dfg/DFGDisassembler.cpp \
    dfg/DFGDominators.cpp \
    dfg/DFGDriver.cpp \
    dfg/DFGEdge.cpp \
    dfg/DFGFailedFinalizer.cpp \
    dfg/DFGFinalizer.cpp \
    dfg/DFGFixupPhase.cpp \
    dfg/DFGFlushFormat.cpp \
    dfg/DFGFlushLivenessAnalysisPhase.cpp \
    dfg/DFGGraph.cpp \
    dfg/DFGInPlaceAbstractState.cpp \
    dfg/DFGJITCode.cpp \
    dfg/DFGJITCompiler.cpp \
    dfg/DFGJITFinalizer.cpp \
    dfg/DFGLICMPhase.cpp \
    dfg/DFGLazyJSValue.cpp \
    dfg/DFGLivenessAnalysisPhase.cpp \
    dfg/DFGLongLivedState.cpp \
    dfg/DFGLoopPreHeaderCreationPhase.cpp \
    dfg/DFGMinifiedNode.cpp \
    dfg/DFGNaturalLoops.cpp \
    dfg/DFGNode.cpp \
    dfg/DFGNodeFlags.cpp \
    dfg/DFGOperations.cpp \
    dfg/DFGOSRAvailabilityAnalysisPhase.cpp \
    dfg/DFGOSREntry.cpp \
    dfg/DFGOSRExit.cpp \
    dfg/DFGOSRExitBase.cpp \
    dfg/DFGOSRExitCompiler.cpp \
    dfg/DFGOSRExitCompiler64.cpp \
    dfg/DFGOSRExitCompiler32_64.cpp \
    dfg/DFGOSRExitCompilerCommon.cpp \
    dfg/DFGOSRExitJumpPlaceholder.cpp \
    dfg/DFGOSRExitPreparation.cpp \
    dfg/DFGPhase.cpp \
    dfg/DFGPlan.cpp \
    dfg/DFGPredictionPropagationPhase.cpp \
    dfg/DFGPredictionInjectionPhase.cpp \
    dfg/DFGRepatch.cpp \
    dfg/DFGSSAConversionPhase.cpp \
    dfg/DFGSpeculativeJIT.cpp \
    dfg/DFGSpeculativeJIT32_64.cpp \
    dfg/DFGSpeculativeJIT64.cpp \
    dfg/DFGTypeCheckHoistingPhase.cpp \
    dfg/DFGThunks.cpp \
    dfg/DFGUnificationPhase.cpp \
    dfg/DFGUseKind.cpp \
    dfg/DFGValueSource.cpp \
    dfg/DFGVariableAccessDataDump.cpp \
    dfg/DFGVariableEvent.cpp \
    dfg/DFGVariableEventStream.cpp \
    dfg/DFGValidate.cpp \
    dfg/DFGVirtualRegisterAllocationPhase.cpp \
    dfg/DFGWorklist.cpp \
    disassembler/Disassembler.cpp \
    interpreter/AbstractPC.cpp \
    interpreter/CallFrame.cpp \
    interpreter/Interpreter.cpp \
    interpreter/JSStack.cpp \
    interpreter/StackIterator.cpp \
    jit/ClosureCallStubRoutine.cpp \
    jit/ExecutableAllocatorFixedVMPool.cpp \
    jit/ExecutableAllocator.cpp \
    jit/HostCallReturnValue.cpp \
    jit/GCAwareJITStubRoutine.cpp \
    jit/JITArithmetic.cpp \
    jit/JITArithmetic32_64.cpp \
    jit/JITCall.cpp \
    jit/JITCall32_64.cpp \
    jit/JITCode.cpp \
    jit/JIT.cpp \
    jit/JITDisassembler.cpp \
    jit/JITExceptions.cpp \
    jit/JITOpcodes.cpp \
    jit/JITOpcodes32_64.cpp \
    jit/JITPropertyAccess.cpp \
    jit/JITPropertyAccess32_64.cpp \
    jit/JITStubRoutine.cpp \
    jit/JITStubs.cpp \
    jit/JITThunks.cpp \
    jit/JITToDFGDeferredCompilationCallback.cpp \
    jit/JumpReplacementWatchpoint.cpp \
    jit/ThunkGenerators.cpp \
    llint/LLIntCLoop.cpp \
    llint/LLIntData.cpp \
    llint/LLIntEntrypoints.cpp \
    llint/LLIntExceptions.cpp \
    llint/LLIntSlowPaths.cpp \
    llint/LLIntThunks.cpp \
    llint/LowLevelInterpreter.cpp \
    parser/Lexer.cpp \
    parser/Nodes.cpp \
    parser/ParserArena.cpp \
    parser/Parser.cpp \
    parser/SourceCode.cpp \
    parser/SourceProvider.cpp \
    parser/SourceProviderCache.cpp \
    profiler/ProfilerBytecode.cpp \
    profiler/ProfilerBytecode.h \
    profiler/ProfilerBytecodeSequence.cpp \
    profiler/ProfilerBytecodes.cpp \
    profiler/ProfilerBytecodes.h \
    profiler/ProfilerCompilation.cpp \
    profiler/ProfilerCompilation.h \
    profiler/ProfilerCompilationKind.cpp \
    profiler/ProfilerCompilationKind.h \
    profiler/ProfilerCompiledBytecode.cpp \
    profiler/ProfilerCompiledBytecode.h \
    profiler/ProfilerDatabase.cpp \
    profiler/ProfilerDatabase.h \
    profiler/ProfilerExecutionCounter.h \
    profiler/ProfilerOrigin.cpp \
    profiler/ProfilerOrigin.h \
    profiler/ProfilerOriginStack.cpp \
    profiler/ProfilerOriginStack.h \
    profiler/ProfilerOSRExit.cpp \
    profiler/ProfilerOSRExitSite.cpp \
    profiler/ProfilerProfiledBytecodes.cpp \
    profiler/Profile.cpp \
    profiler/ProfileGenerator.cpp \
    profiler/ProfileNode.cpp \
    profiler/LegacyProfiler.cpp \
    runtime/ArgList.cpp \
    runtime/Arguments.cpp \
    runtime/ArrayBuffer.cpp \
    runtime/ArrayBufferView.cpp \
    runtime/ArrayConstructor.cpp \
    runtime/ArrayPrototype.cpp \
    runtime/BooleanConstructor.cpp \
    runtime/BooleanObject.cpp \
    runtime/BooleanPrototype.cpp \
    runtime/CallData.cpp \
    runtime/CodeCache.cpp \
    runtime/CodeSpecializationKind.cpp \
    runtime/CommonIdentifiers.cpp \
    runtime/CommonSlowPaths.cpp \
    runtime/CommonSlowPathsExceptions.cpp \
    runtime/CompilationResult.cpp \
    runtime/Completion.cpp \
    runtime/ConstructData.cpp \
    runtime/DataView.cpp \
    runtime/DateConstructor.cpp \
    runtime/DateConversion.cpp \
    runtime/DateInstance.cpp \
    runtime/DatePrototype.cpp \
    runtime/DumpContext.cpp \
    runtime/Error.cpp \
    runtime/ErrorConstructor.cpp \
    runtime/ErrorInstance.cpp \
    runtime/ErrorPrototype.cpp \
    runtime/ExceptionHelpers.cpp \
    runtime/Executable.cpp \
    runtime/FunctionConstructor.cpp \
    runtime/FunctionExecutableDump.cpp \
    runtime/FunctionPrototype.cpp \
    runtime/GCActivityCallback.cpp \
    runtime/GetterSetter.cpp \
    runtime/Identifier.cpp \
    runtime/IndexingType.cpp \
    runtime/InitializeThreading.cpp \
    runtime/IntendedStructureChain.cpp \
    runtime/InternalFunction.cpp \
    runtime/JSAPIValueWrapper.cpp \
    runtime/JSActivation.cpp \
    runtime/JSArray.cpp \
    runtime/JSArrayBuffer.cpp \
    runtime/JSArrayBufferConstructor.cpp \
    runtime/JSArrayBufferPrototype.cpp \
    runtime/JSArrayBufferView.cpp \
    runtime/JSBoundFunction.cpp \
    runtime/JSCJSValue.cpp \
    runtime/JSCell.cpp \
    runtime/JSDataView.cpp \
    runtime/JSDataViewPrototype.cpp \
    runtime/JSDateMath.cpp \
    runtime/JSFunction.cpp \
    runtime/JSGlobalObject.cpp \
    runtime/JSGlobalObjectFunctions.cpp \
    runtime/JSLock.cpp \
    runtime/JSNameScope.cpp \
    runtime/JSNotAnObject.cpp \
    runtime/JSONObject.cpp \
    runtime/JSObject.cpp \
    runtime/JSPromise.cpp \
    runtime/JSPromiseCallback.cpp \
    runtime/JSPromiseConstructor.cpp \
    runtime/JSPromisePrototype.cpp \
    runtime/JSPromiseResolver.cpp \
    runtime/JSPromiseResolverConstructor.cpp \
    runtime/JSPromiseResolverPrototype.cpp \
    runtime/JSPropertyNameIterator.cpp \
    runtime/JSProxy.cpp \
    runtime/JSScope.cpp \
    runtime/JSSegmentedVariableObject.cpp \
    runtime/JSString.cpp \
    runtime/JSStringJoiner.cpp \
    runtime/JSSymbolTableObject.cpp \
    runtime/JSTypedArrayConstructors.cpp \
    runtime/JSTypedArrayPrototypes.cpp \
    runtime/JSTypedArrays.cpp \
    runtime/JSVariableObject.cpp \
    runtime/JSWithScope.cpp \
    runtime/JSWrapperObject.cpp \
    runtime/LiteralParser.cpp \
    runtime/Lookup.cpp \
    runtime/MathObject.cpp \
    runtime/MemoryStatistics.cpp \
    runtime/NameConstructor.cpp \
    runtime/NameInstance.cpp \
    runtime/NamePrototype.cpp \
    runtime/NativeErrorConstructor.cpp \
    runtime/NativeErrorPrototype.cpp \
    runtime/NumberConstructor.cpp \
    runtime/NumberObject.cpp \
    runtime/NumberPrototype.cpp \
    runtime/ObjectConstructor.cpp \
    runtime/ObjectPrototype.cpp \
    runtime/Operations.cpp \
    runtime/Options.cpp \
    runtime/PropertyDescriptor.cpp \
    runtime/PropertyNameArray.cpp \
    runtime/PropertySlot.cpp \
    runtime/PropertyTable.cpp \
    runtime/PrototypeMap.cpp \
    runtime/RegExp.cpp \
    runtime/RegExpCache.cpp \
    runtime/RegExpCachedResult.cpp \
    runtime/RegExpConstructor.cpp \
    runtime/RegExpMatchesArray.cpp \
    runtime/RegExpObject.cpp \
    runtime/RegExpPrototype.cpp \
    runtime/SamplingCounter.cpp \
    runtime/SimpleTypedArrayController.cpp \
    runtime/SmallStrings.cpp \
    runtime/SparseArrayValueMap.cpp \
    runtime/StrictEvalActivation.cpp \
    runtime/StringConstructor.cpp \
    runtime/StringObject.cpp \
    runtime/StringPrototype.cpp \
    runtime/StringRecursionChecker.cpp \
    runtime/Structure.cpp \
    runtime/StructureChain.cpp \
    runtime/StructureRareData.cpp \
    runtime/SymbolTable.cpp \
    runtime/TypedArrayController.cpp \
    runtime/TypedArrayType.cpp \
    runtime/VM.cpp \
    runtime/Watchdog.cpp \
    runtime/WatchdogNone.cpp \
    tools/CodeProfile.cpp \
    tools/CodeProfiling.cpp \
    yarr/YarrJIT.cpp \

linux-*:if(isEqual(QT_ARCH, "i386")|isEqual(QT_ARCH, "x86_64")) {
    SOURCES += \
        disassembler/X86Disassembler.cpp \
        disassembler/UDis86Disassembler.cpp \
        disassembler/udis86/udis86.c \
        disassembler/udis86/udis86_decode.c \
        disassembler/udis86/udis86_input.c \
        disassembler/udis86/udis86_itab_holder.c \
        disassembler/udis86/udis86_syn-att.c \
        disassembler/udis86/udis86_syn-intel.c \
        disassembler/udis86/udis86_syn.c \
}

win32:!win32-g++*:isEqual(QT_ARCH, "x86_64"):{
    asm_compiler.commands = ml64 /c
    asm_compiler.commands +=  /Fo ${QMAKE_FILE_OUT} ${QMAKE_FILE_IN}
    asm_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
    asm_compiler.input = ASM_SOURCES
    asm_compiler.variable_out = OBJECTS
    asm_compiler.name = compiling[asm] ${QMAKE_FILE_IN}
    silent:asm_compiler.commands = @echo compiling[asm] ${QMAKE_FILE_IN} && $$asm_compiler.commands
    QMAKE_EXTRA_COMPILERS += asm_compiler

    ASM_SOURCES += jit/JITStubsMSVC64.asm
}

build?(qttestsupport) {
    HEADERS += API/JSCTestRunnerUtils.h
    SOURCES += API/JSCTestRunnerUtils.cpp
}

HEADERS += $$files(*.h, true)

*sh4* {
    QMAKE_CXXFLAGS += -mieee -w
    QMAKE_CFLAGS   += -mieee -w
}
