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
#include "V8DOMMap.h"
/// begin z00178175 2012-04-11 Web Worker Feature ///
#include "V8Binding.h"
/// end z00178175 2012-04-11 Web Worker Feature ///

#include "DOMData.h"
#include "DOMDataStore.h"
#include "MainThreadDOMData.h"
#include "ScopedDOMDataStore.h"

namespace WebCore {

DOMDataStoreHandle::DOMDataStoreHandle()
/// begin z00178175 2012-04-11 Web Worker Feature ///
    : m_store(new ScopedDOMDataStore())
{
    V8BindingPerIsolateData::current()->registerDOMDataStore(m_store.get());
}

DOMDataStoreHandle::~DOMDataStoreHandle()
{
    V8BindingPerIsolateData::current()->unregisterDOMDataStore(m_store.get());
}
/// end z00178175 2012-04-11 Web Worker Feature ///
static bool fasterDOMStoreAccess = false;

static inline DOMDataStore& getDOMDataStore()
{
    /// begin z00178175 2012-04-11 Web Worker Feature ///
    return DOMData::getCurrentStore();
	/// end z00178175 2012-04-11 Web Worker Feature ///
}

void enableFasterDOMStoreAccess()
{
    fasterDOMStoreAccess = true;
}

DOMNodeMapping& getDOMNodeMap()
{
    return getDOMDataStore().domNodeMap();
}
/// begin z00178175 2012-04-11 Web Worker Feature ///
DOMNodeMapping& getActiveDOMNodeMap()
{
    return getDOMDataStore().activeDomNodeMap();
}
/// end z00178175 2012-04-11 Web Worker Feature ///

DOMWrapperMap<void>& getDOMObjectMap()
{
    return getDOMDataStore().domObjectMap();
}

DOMWrapperMap<void>& getActiveDOMObjectMap()
{
    return getDOMDataStore().activeDomObjectMap();
}

#if ENABLE(SVG)

DOMWrapperMap<SVGElementInstance>& getDOMSVGElementInstanceMap()
{
    return getDOMDataStore().domSvgElementInstanceMap();
}

#endif // ENABLE(SVG)
/// begin z00178175 2012-04-11 Web Worker Feature ///
void removeAllDOMObjects()
{
/// end z00178175 2012-04-11 Web Worker Feature ///
    DOMDataStore& store = getDOMDataStore();

    v8::HandleScope scope;

    // The DOM objects with the following types only exist on the main thread.
	/// begin z00178175 2012-04-11 Web Worker Feature ///
    if (isMainThread()) {
        // Remove all DOM nodes.
        DOMData::removeObjectsFromWrapperMap<Node>(&store, store.domNodeMap());

#if 0 // WJ
        // Remove all active DOM nodes.
        DOMData::removeObjectsFromWrapperMap<Node>(&store, store.activeDomNodeMap());
#endif
    /// end z00178175 2012-04-11 Web Worker Feature ///
#if ENABLE(SVG)
        // Remove all SVG element instances in the wrapper map.
        DOMData::removeObjectsFromWrapperMap<SVGElementInstance>(&store, store.domSvgElementInstanceMap());
#endif
    }

    // Remove all DOM objects in the wrapper map.
    DOMData::removeObjectsFromWrapperMap<void>(&store, store.domObjectMap());

    // Remove all active DOM objects in the wrapper map.
    DOMData::removeObjectsFromWrapperMap<void>(&store, store.activeDomObjectMap());
}

void visitDOMNodesInCurrentThread(DOMWrapperMap<Node>::Visitor* visitor)
{
    v8::HandleScope scope;
	/// begin z00178179 2012-04-11 Web Worker Feature ///
    //WTF::MutexLocker locker(DOMDataStore::allStoresMutex());
    DOMDataList& list = DOMDataStore::allStores();
    for (size_t i = 0; i < list.size(); ++i) {
        DOMDataStore* store = list[i];
    //    if (!store->domData()->owningThread() == WTF::currentThread())
     //       continue;
	/// begin z00178179 2012-04-11 Web Worker Feature ///
        store->domNodeMap().visit(store, visitor);
    }
}

void visitDOMObjectsInCurrentThread(DOMWrapperMap<void>::Visitor* visitor)
{
    v8::HandleScope scope;
	/// begin z00178179 2012-04-11 Web Worker Feature ///
    //WTF::MutexLocker locker(DOMDataStore::allStoresMutex());
    DOMDataList& list = DOMDataStore::allStores();
    for (size_t i = 0; i < list.size(); ++i) {
        DOMDataStore* store = list[i];
    //    if (!store->domData()->owningThread() == WTF::currentThread())
     //       continue;
	/// end z00178179 2012-04-11 Web Worker Feature ///
        store->domObjectMap().visit(store, visitor);
    }
}

void visitActiveDOMObjectsInCurrentThread(DOMWrapperMap<void>::Visitor* visitor)
{
    v8::HandleScope scope;
	/// begin z00178179 2012-04-11 Web Worker Feature ///
    //WTF::MutexLocker locker(DOMDataStore::allStoresMutex());
    DOMDataList& list = DOMDataStore::allStores();
    for (size_t i = 0; i < list.size(); ++i) {
        DOMDataStore* store = list[i];
    //    if (!store->domData()->owningThread() == WTF::currentThread())
     //       continue;
	/// begin z00178179 2012-04-11 Web Worker Feature ///
        store->activeDomObjectMap().visit(store, visitor);
    }
}

#if ENABLE(SVG)

void visitDOMSVGElementInstancesInCurrentThread(DOMWrapperMap<SVGElementInstance>::Visitor* visitor)
{
    v8::HandleScope scope;
	/// begin z00178179 2012-04-11 Web Worker Feature ///
    //WTF::MutexLocker locker(DOMDataStore::allStoresMutex());
    DOMDataList& list = DOMDataStore::allStores();
    for (size_t i = 0; i < list.size(); ++i) {
        DOMDataStore* store = list[i];
    //    if (!store->domData()->owningThread() == WTF::currentThread())
    //        continue;
	/// begin z00178179 2012-04-11 Web Worker Feature ///
        store->domSvgElementInstanceMap().visit(store, visitor);
    }
}

#endif

} // namespace WebCore
