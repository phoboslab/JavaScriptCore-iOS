/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "StackIterator.h"

#include "Arguments.h"
#include "CallFrameInlines.h"
#include "Executable.h"
#include "Operations.h"
#include <wtf/DataLog.h>

namespace JSC {

StackIterator::StackIterator(CallFrame* startFrame, StackIterator::FrameFilter filter)
    : m_startFrame(startFrame)
    , m_filter(filter)
{
    ASSERT(startFrame);
    resetIterator();
}

size_t StackIterator::numberOfFrames()
{
    int savedFrameIndex = m_frameIndex;
    resetIterator();
    while (m_frame)
        gotoNextFrame();
    size_t numberOfFrames = m_frameIndex;

    resetIterator();
    gotoFrameAtIndex(savedFrameIndex);

    return numberOfFrames;
}

void StackIterator::gotoFrameAtIndex(size_t index)
{
    while (m_frame && (m_frameIndex != index))
        gotoNextFrame();
}

void StackIterator::gotoNextFrame()
{
    Frame* frame = m_frame;
    while (frame) {
        frame = frame->logicalCallerFrame();
        if (!frame || !m_filter || !m_filter(frame))
            break;
    }
    m_frame = frame;
    m_frameIndex++;
}

void StackIterator::resetIterator()
{
    m_frameIndex = 0;
    m_frame = Frame::create(m_startFrame);
    m_frame = m_frame->logicalFrame();
}

void StackIterator::find(JSFunction* functionObj)
{
    ASSERT(functionObj);
    JSObject* targetCallee = jsDynamicCast<JSObject*>(functionObj);
    while (m_frame) {
        if (m_frame->callee() == targetCallee)
            break;
        gotoNextFrame();
    }
}

StackIterator::Frame::CodeType StackIterator::Frame::codeType() const
{
    if (!isJSFrame())
        return StackIterator::Frame::Native;

    switch (codeBlock()->codeType()) {
    case EvalCode:
        return StackIterator::Frame::Eval;
    case FunctionCode:
        return StackIterator::Frame::Function;
    case GlobalCode:
        return StackIterator::Frame::Global;
    }
    RELEASE_ASSERT_NOT_REACHED();
    return StackIterator::Frame::Global;
}

String StackIterator::Frame::functionName()
{
    String traceLine;
    JSObject* callee = this->callee();

    switch (codeType()) {
    case StackIterator::Frame::Eval:
        traceLine = "eval code";
        break;
    case StackIterator::Frame::Native:
        if (callee)
            traceLine = getCalculatedDisplayName(callFrame(), callee).impl();
        break;
    case StackIterator::Frame::Function:
        traceLine = getCalculatedDisplayName(callFrame(), callee).impl();
        break;
    case StackIterator::Frame::Global:
        traceLine = "global code";
        break;
    }
    return traceLine.isNull() ? emptyString() : traceLine;
}

String StackIterator::Frame::sourceURL()
{
    String traceLine;

    switch (codeType()) {
    case StackIterator::Frame::Eval:
    case StackIterator::Frame::Function:
    case StackIterator::Frame::Global: {
        String sourceURL = codeBlock()->ownerExecutable()->sourceURL();
        if (!sourceURL.isEmpty())
            traceLine = sourceURL.impl();
        break;
    }
    case StackIterator::Frame::Native:
        traceLine = "[native code]";
        break;
    }
    return traceLine.isNull() ? emptyString() : traceLine;
}

String StackIterator::Frame::toString()
{
    StringBuilder traceBuild;
    String functionName = this->functionName();
    String sourceURL = this->sourceURL();
    traceBuild.append(functionName);
    if (!sourceURL.isEmpty()) {
        if (!functionName.isEmpty())
            traceBuild.append('@');
        traceBuild.append(sourceURL);
        if (isJSFrame()) {
            unsigned line = 0;
            unsigned column = 0;
            computeLineAndColumn(line, column);
            traceBuild.append(':');
            traceBuild.appendNumber(line);
            traceBuild.append(':');
            traceBuild.appendNumber(column);
        }
    }
    return traceBuild.toString().impl();
}

unsigned StackIterator::Frame::bytecodeOffset()
{
    if (!isJSFrame())
        return 0;
#if ENABLE(DFG_JIT)
    if (hasLocationAsCodeOriginIndex())
        return bytecodeOffsetFromCodeOriginIndex();
#endif
    return locationAsBytecodeOffset();
}

Arguments* StackIterator::Frame::arguments()
{
    CallFrame* callFrame = this->callFrame();
    Arguments* arguments = Arguments::create(vm(), callFrame);
    arguments->tearOff(callFrame);
    return arguments;
}

void StackIterator::Frame::computeLineAndColumn(unsigned& line, unsigned& column)
{
    CodeBlock* codeBlock = this->codeBlock();
    if (!codeBlock) {
        line = 0;
        column = 0;
        return;
    }

    int divot = 0;
    int unusedStartOffset = 0;
    int unusedEndOffset = 0;
    unsigned divotLine = 0;
    unsigned divotColumn = 0;
    retrieveExpressionInfo(divot, unusedStartOffset, unusedEndOffset, divotLine, divotColumn);

    line = divotLine + codeBlock->ownerExecutable()->lineNo();
    column = divotColumn + (divotLine ? 1 : codeBlock->firstLineColumnOffset());
}

void StackIterator::Frame::retrieveExpressionInfo(int& divot, int& startOffset, int& endOffset, unsigned& line, unsigned& column)
{
    CodeBlock* codeBlock = this->codeBlock();
    codeBlock->unlinkedCodeBlock()->expressionRangeForBytecodeOffset(bytecodeOffset(), divot, startOffset, endOffset, line, column);
    divot += codeBlock->sourceOffset();
}


StackIterator::Frame* StackIterator::Frame::logicalFrame()
{
#if !ENABLE(DFG_JIT)
    return this;

#else // !ENABLE(DFG_JIT)
    if (isInlinedFrame())
        return this;

    // If I don't have a code block, then I'm not DFG code, so I'm the true call frame.
    CodeBlock* codeBlock = this->codeBlock();
    if (!codeBlock)
        return this;

    // If the code block does not have any code origins, then there was no inlining, so
    // I'm done.
    if (!codeBlock->hasCodeOrigins())
        return this;
    
    CodeBlock* outerMostCodeBlock = codeBlock;
    unsigned index = locationAsCodeOriginIndex();
    ASSERT(outerMostCodeBlock->canGetCodeOrigin(index));
    if (!outerMostCodeBlock->canGetCodeOrigin(index)) {
        // See above. In release builds, we try to protect ourselves from crashing even
        // though stack walking will be goofed up.
        return 0;
    }

    CodeOrigin codeOrigin = outerMostCodeBlock->codeOrigin(index);
    if (!codeOrigin.inlineCallFrame)
        return this; // Not currently in inlined code.

    // We've got inlined frames. So, reify them so that the iterator can walk through them.
    CallFrame* currFrame = this->callFrame();
    CallFrame* innerMostLogicalFrame = currFrame + codeOrigin.inlineCallFrame->stackOffset;

    CallFrame* logicalFrame = innerMostLogicalFrame;
    while (logicalFrame != currFrame) {
        InlineCallFrame* inlinedFrameInfo = codeOrigin.inlineCallFrame;
        
        // Fill in the logical (i.e. inlined) frame
        logicalFrame->setCodeBlock(inlinedFrameInfo->baselineCodeBlock());
        logicalFrame->setInlineCallFrame(inlinedFrameInfo);
        logicalFrame->setArgumentCountIncludingThis(inlinedFrameInfo->arguments.size());
        logicalFrame->setLocationAsBytecodeOffset(codeOrigin.bytecodeIndex);
        logicalFrame->setIsInlinedFrame();

        JSFunction* callee = inlinedFrameInfo->callee.get();
        if (callee) {
            logicalFrame->setScope(callee->scope());
            logicalFrame->setCallee(callee);
        }
        
        CodeOrigin* callerCodeOrigin = &inlinedFrameInfo->caller;
        InlineCallFrame* callerInlinedFrameInfo = callerCodeOrigin->inlineCallFrame;
        unsigned callerFrameOffset = callerInlinedFrameInfo ? callerInlinedFrameInfo->stackOffset : 0;
        CallFrame* callerFrame = currFrame + callerFrameOffset;
        logicalFrame->setCallerFrame(callerFrame);

        codeOrigin = *callerCodeOrigin;
        logicalFrame = callerFrame;
    }
    
    ASSERT(!innerMostLogicalFrame->hasHostCallFrameFlag());
    return Frame::create(innerMostLogicalFrame);
#endif // !ENABLE(DFG_JIT)
}

StackIterator::Frame* StackIterator::Frame::logicalCallerFrame()
{
    Frame* callerFrame = create(this->callerFrame()->removeHostCallFrameFlag());
#if !ENABLE(DFG_JIT)
    return callerFrame;

#else // !ENABLE(DFG_JIT)
    if (!isJSFrame() || !callerFrame)
        return callerFrame;

    // If I am known to be an inlined frame, then I've been reified already and
    // have my caller.
    if (isInlinedFrame())
        return callerFrame;
    
    // I am not an inlined frame. So the question is: is my caller a CallFrame
    // that has inlines or a CallFrame that doesn't?

    // If my caller is not a JS frame, it cannot have inlines, and we're done.
    if (!callerFrame->isJSFrame())
        return callerFrame;

    ASSERT(!callerFrame->isInlinedFrame());
    return callerFrame->logicalFrame();

#endif // !ENABLE(DFG_JIT)
}

#ifndef NDEBUG

static const char* jitTypeName(JITCode::JITType jitType)
{
    switch (jitType) {
    case JITCode::None: return "None";
    case JITCode::HostCallThunk: return "HostCallThunk";
    case JITCode::InterpreterThunk: return "InterpreterThunk";
    case JITCode::BaselineJIT: return "BaselineJIT";
    case JITCode::DFGJIT: return "DFGJIT";
    case JITCode::FTLJIT: return "FTLJIT";
    }
    return "<unknown>";
}

static void printIndents(int levels)
{
    while (levels--)
        dataLogFString("   ");
}

static void printif(int indentLevels, const char* format, ...)
{
    va_list argList;
    va_start(argList, format);

    if (indentLevels)
        printIndents(indentLevels);

#if COMPILER(CLANG) || (COMPILER(GCC) && GCC_VERSION_AT_LEAST(4, 6, 0))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wmissing-format-attribute"
#endif

    WTF::dataLogFV(format, argList);

#if COMPILER(CLANG) || (COMPILER(GCC) && GCC_VERSION_AT_LEAST(4, 6, 0))
#pragma GCC diagnostic pop
#endif

    va_end(argList);
}

void StackIterator::Frame::print(int indentLevel)
{
    int i = indentLevel;

    CodeBlock* codeBlock = this->codeBlock();
    printif(i, "frame %p {\n", this);

    CallFrame* callerFrame = this->callerFrame();
    void* returnPC = hasReturnPC() ? this->returnPC().value() : 0;

    printif(i, "   name '%s'\n", functionName().utf8().data());
    printif(i, "   sourceURL '%s'\n", sourceURL().utf8().data());
    printif(i, "   hostFlag %d\n", callerFrame->hasHostCallFrameFlag());
    printif(i, "   isInlinedFrame %d\n", isInlinedFrame());

    if (isInlinedFrame())
        printif(i, "   InlineCallFrame %p\n", this->inlineCallFrame());

    printif(i, "   callee %p\n", callee());
    printif(i, "   returnPC %p\n", returnPC);
    printif(i, "   callerFrame %p\n", callerFrame->removeHostCallFrameFlag());
    printif(i, "   logicalCallerFrame %p\n", logicalCallerFrame());
    printif(i, "   rawLocationBits %u 0x%x\n", locationAsRawBits(), locationAsRawBits());
    printif(i, "   codeBlock %p\n", codeBlock);
    if (codeBlock) {
        JITCode::JITType jitType = codeBlock->jitType();
        if (hasLocationAsBytecodeOffset()) {
            unsigned bytecodeOffset = locationAsBytecodeOffset();
            printif(i, "      bytecodeOffset %u %p / %zu\n", bytecodeOffset, reinterpret_cast<void*>(bytecodeOffset), codeBlock->instructions().size());
#if ENABLE(DFG_JIT)
        } else {
            unsigned codeOriginIndex = locationAsCodeOriginIndex();
            printif(i, "      codeOriginIdex %u %p / %zu\n", codeOriginIndex, reinterpret_cast<void*>(codeOriginIndex), codeBlock->codeOrigins().size());
#endif
        }
        unsigned line = 0;
        unsigned column = 0;
        computeLineAndColumn(line, column);
        printif(i, "      line %d\n", line);
        printif(i, "      column %d\n", column);
        printif(i, "      jitType %d <%s> isOptimizingJIT %d\n", jitType, jitTypeName(jitType), JITCode::isOptimizingJIT(jitType));
#if ENABLE(DFG_JIT)
        printif(i, "      hasCodeOrigins %d\n", codeBlock->hasCodeOrigins());
        if (codeBlock->hasCodeOrigins()) {
            JITCode* jitCode = codeBlock->jitCode().get();
            printif(i, "         jitCode %p start %p end %p\n", jitCode, jitCode->start(), jitCode->end());
        }
#endif
    }
    printif(i, "}\n");
}

#endif // NDEBUG

} // namespace JSC

#ifndef NDEBUG
// For use in the debugger
void debugPrintCallFrame(JSC::CallFrame*);

void debugPrintCallFrame(JSC::CallFrame* callFrame)
{
    if (!callFrame)
        return;
    JSC::StackIterator::Frame* frame = JSC::StackIterator::Frame::create(callFrame);
    frame->print(2);
}
#endif // !NDEBUG
