/**
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#if ENABLE(WML)
#include "WMLGoElement.h"

#include "Attribute.h"
#include "FormData.h"
#include "FormDataBuilder.h"
#include "FormSubmission.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLNames.h"
#include "ResourceRequest.h"
#include "TextEncoding.h"
#include "WMLCardElement.h"
#include "WMLDocument.h"
#include "WMLNames.h"
#include "WMLPageState.h"
#include "WMLPostfieldElement.h"
#include "WMLTimerElement.h"
#include "WMLVariables.h"
#include <wtf/text/CString.h>

namespace WebCore {

using namespace WMLNames;

WMLGoElement::WMLGoElement(const QualifiedName& tagName, Document* doc)
    : WMLTaskElement(tagName, doc)
{
}
 
PassRefPtr<WMLGoElement> WMLGoElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new WMLGoElement(tagName, document));
}

void WMLGoElement::registerPostfieldElement(WMLPostfieldElement* postfield)
{
    ASSERT(m_postfieldElements.find(postfield) == WTF::notFound);
    m_postfieldElements.append(postfield);
}
 
void WMLGoElement::deregisterPostfieldElement(WMLPostfieldElement* postfield)
{
    size_t position = m_postfieldElements.find(postfield);
    ASSERT(position != WTF::notFound);
    m_postfieldElements.remove(position);
}

void WMLGoElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == HTMLNames::methodAttr)
        m_formAttributes.parseMethodType(attr->value());
    else if (attr->name() == HTMLNames::enctypeAttr)
        m_formAttributes.parseEncodingType(parseValueSubstitutingVariableReferences(attr->value()));
    else if (attr->name() == HTMLNames::accept_charsetAttr)
        m_formAttributes.setAcceptCharset(parseValueForbiddingVariableReferences(attr->value()));
    else
        WMLTaskElement::parseMappedAttribute(attr);
}

void WMLGoElement::executeTask()
{
    ASSERT(document()->isWMLDocument());
    WMLDocument* document = static_cast<WMLDocument*>(this->document());

    WMLPageState* pageState = wmlPageStateForDocument(document);
    if (!pageState)
        return;

    WMLCardElement* card = document->activeCard();
    if (!card)
        return;

    Frame* frame = document->frame();
    if (!frame)
        return;

    FrameLoader* loader = frame->loader();
    if (!loader)
        return;

    String href = getAttribute(HTMLNames::hrefAttr);
    if (href.isEmpty())
        return;

    // Substitute variables within target url attribute value
    KURL url = document->completeURL(substituteVariableReferences(href, document, WMLVariableEscapingEscape));
    if (url.isEmpty())
        return;

    storeVariableState(pageState);

    // Stop the timer of the current card if it is active
    if (WMLTimerElement* eventTimer = card->eventTimer())
        eventTimer->stop();

    // FIXME: 'newcontext' handling not implemented for external cards
    bool inSameDeck = document->url().path() == url.path();
    if (inSameDeck && url.hasFragmentIdentifier()) {
        if (WMLCardElement* card = WMLCardElement::findNamedCardInDocument(document, url.fragmentIdentifier())) {
            if (card->isNewContext())
                pageState->reset();
        }
    }

    // Prepare loading the destination url
    ResourceRequest request(url);

    if (getAttribute(sendrefererAttr) == "true")
        request.setHTTPReferrer(loader->outgoingReferrer());

    String cacheControl = getAttribute(cache_controlAttr);

    if (m_formAttributes.method() == FormSubmission::PostMethod)
        preparePOSTRequest(request, inSameDeck, cacheControl);
    else
        prepareGETRequest(request, url);

    // Set HTTP cache-control header if needed
    if (!cacheControl.isEmpty()) {
        request.setHTTPHeaderField("cache-control", cacheControl);

        if (cacheControl == "no-cache")
            request.setCachePolicy(ReloadIgnoringCacheData);
    }

    loader->load(request, false);
}

void WMLGoElement::preparePOSTRequest(ResourceRequest& request, bool inSameDeck, const String& cacheControl)
{
    request.setHTTPMethod("POST");

    if (inSameDeck && cacheControl != "no-cache") {
        request.setCachePolicy(ReturnCacheDataDontLoad);
        return;
    }

    RefPtr<FormData> data;

    if (m_formAttributes.isMultiPartForm()) { // multipart/form-data
        Vector<char> boundary = FormDataBuilder::generateUniqueBoundaryString();
        data = createFormData(boundary.data());
        request.setHTTPContentType(m_formAttributes.encodingType() + "; boundary=" + boundary.data());
    } else {
        // text/plain or application/x-www-form-urlencoded
        data = createFormData(CString());
        request.setHTTPContentType(m_formAttributes.encodingType());
    }

    request.setHTTPBody(data.get());
}

void WMLGoElement::prepareGETRequest(ResourceRequest& request, const KURL& url)
{
    request.setHTTPMethod("GET");

    // Eventually display error message?
    if (m_formAttributes.isMultiPartForm())
        return;

    RefPtr<FormData> data = createFormData(CString());

    KURL remoteURL(url);
    String query = data->flattenToString();
    if (!query.isEmpty())
        remoteURL.setQuery(data->flattenToString());
    /*guoxiaolei 20120827 end>*/
    request.setURL(remoteURL);
}

PassRefPtr<FormData> WMLGoElement::createFormData(const CString& boundary)
{
    CString key;
    CString value;

    Vector<char> encodedData;
    TextEncoding encoding = FormDataBuilder::encodingFromAcceptCharset(m_formAttributes.acceptCharset(), document()).encodingForFormSubmission();

    Vector<WMLPostfieldElement*>::iterator it = m_postfieldElements.begin();
    Vector<WMLPostfieldElement*>::iterator end = m_postfieldElements.end();

    RefPtr<FormData> result = FormData::create();
    for (; it != end; ++it) {
        (*it)->encodeData(encoding, key, value);

        if (m_formAttributes.isMultiPartForm()) {
            Vector<char> header;
            FormDataBuilder::beginMultiPartHeader(header, boundary, key);
            FormDataBuilder::finishMultiPartHeader(header);
            result->appendData(header.data(), header.size());

            if (size_t dataSize = value.length())
                result->appendData(value.data(), dataSize);

            result->appendData("\r\n", 2);
        } else
            FormDataBuilder::addKeyValuePairAsFormData(encodedData, key, value);
    }

    if (m_formAttributes.isMultiPartForm())
        FormDataBuilder::addBoundaryToMultiPartHeader(encodedData, boundary, true);

    result->appendData(encodedData.data(), encodedData.size());
    return result;
}

}

#endif
