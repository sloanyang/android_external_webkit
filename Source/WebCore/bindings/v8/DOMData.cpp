/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMData.h"
/// begin z00178179 2012-04-11 Web Worker Feature ///
#include "V8Binding.h"
#include "V8IsolatedContext.h"
/// end z00178179 2012-04-11 Web Worker Feature ///

#include "ChildThreadDOMData.h"
#include "MainThreadDOMData.h"
#include "WebGLContextAttributes.h"
#include "WebGLUniformLocation.h"

namespace WebCore {
/// begin z00178179 2012-04-11 Web Worker Feature ///
static StaticDOMDataStore& getDefaultStore() 
{
    DEFINE_STATIC_LOCAL(StaticDOMDataStore, defaultStore, ());
    return defaultStore;
}

DOMDataStore& DOMData::getCurrentStore()
{
    V8BindingPerIsolateData* data = V8BindingPerIsolateData::current();
    if (UNLIKELY(data->domDataStore() != 0))
        return *data->domDataStore();
    V8IsolatedContext* context = V8IsolatedContext::getEntered();
    if (UNLIKELY(context != 0))
        return *context->world()->domDataStore();
    return getDefaultStore();
}
/// end z00178179 2012-04-11 Web Worker Feature ///

DOMData::DOMData()
    : m_owningThread(WTF::currentThread())
{
}

DOMData::~DOMData()
{
}
/// begin z00178179 2012-04-11 Web Worker Feature ///
/*
DOMData* DOMData::getCurrent()
{
    if (WTF::isMainThread())
        return MainThreadDOMData::getCurrent();

    DEFINE_STATIC_LOCAL(WTF::ThreadSpecific<ChildThreadDOMData>, childThreadDOMData, ());
    return childThreadDOMData;
}
*/
/// end z00178179 2012-04-11 Web Worker Feature ///
void DOMData::derefObject(WrapperTypeInfo* type, void* domObject)
{
    type->derefObject(domObject);
}

} // namespace WebCore
