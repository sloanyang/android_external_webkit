/*
 * Copyright 2006, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RenderThemeAndroid_h
#define RenderThemeAndroid_h

#include "PaintInfo.h"
#include "RenderTheme.h"

namespace WebCore {

struct ThemeData {
    ThemeData()
        : m_part(0)
        , m_state(0)
    {
    }

    unsigned m_part;
    unsigned m_state;
};

class RenderThemeAndroid : public RenderTheme {
public:
    static PassRefPtr<RenderTheme> create();
    ~RenderThemeAndroid();
    
    virtual bool stateChanged(RenderObject*, ControlState) const;

    virtual bool supportsFocusRing(const RenderStyle*) const;
    // A method asking if the theme's controls actually care about redrawing when hovered.
    virtual bool supportsHover(const RenderStyle* style) const { return style->affectedByHoverRules(); }

    virtual int baselinePosition(const RenderObject*) const;

    virtual Color platformActiveSelectionBackgroundColor() const;
    virtual Color platformInactiveSelectionBackgroundColor() const;
    virtual Color platformActiveSelectionForegroundColor() const;
    virtual Color platformInactiveSelectionForegroundColor() const;
    virtual Color platformTextSearchHighlightColor() const;
    virtual Color platformFocusRingColor() const;

    virtual Color platformActiveListBoxSelectionBackgroundColor() const;
    virtual Color platformInactiveListBoxSelectionBackgroundColor() const;
    virtual Color platformActiveListBoxSelectionForegroundColor() const;
    virtual Color platformInactiveListBoxSelectionForegroundColor() const;

    virtual Color platformActiveTextSearchHighlightColor() const;
    virtual Color platformInactiveTextSearchHighlightColor() const;

    virtual void systemFont(int, WebCore::FontDescription&) const {}

    virtual int minimumMenuListSize(RenderStyle*) const { return 0; }

protected:
    virtual bool paintCheckbox(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void setCheckboxSize(RenderStyle*) const;

#if ENABLE(VIDEO)
    virtual String extraMediaControlsStyleSheet();
    virtual void adjustSliderThumbSize(RenderObject* o) const;
    virtual bool shouldRenderMediaControlPart(ControlPart part, Element* e);
    virtual bool paintMediaFullscreenButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r);
    virtual bool paintMediaMuteButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r);
    virtual bool paintMediaPlayButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r);
    virtual bool paintMediaSeekBackButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r);
    virtual bool paintMediaSeekForwardButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r);
    virtual bool paintMediaSliderTrack(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r);
    virtual bool paintMediaSliderThumb(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r);
    virtual bool paintMediaControlsBackground(RenderObject* object, const PaintInfo& paintInfo, const IntRect& rect);
    virtual double mediaControlsFadeInDuration() { return 0.5; }
    virtual double mediaControlsFadeOutDuration() { return 0.5; }
#endif

    virtual bool paintRadio(RenderObject*, const PaintInfo&, const IntRect&);
    virtual void setRadioSize(RenderStyle*) const;

    virtual void adjustButtonStyle(CSSStyleSelector*, RenderStyle*, WebCore::Element*) const;
    virtual bool paintButton(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustTextFieldStyle(CSSStyleSelector*, RenderStyle*, WebCore::Element*) const;
    virtual bool paintTextField(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustTextAreaStyle(CSSStyleSelector*, RenderStyle*, WebCore::Element*) const;
    virtual bool paintTextArea(RenderObject*, const PaintInfo&, const IntRect&);
    
    bool paintCombo(RenderObject*, const PaintInfo&,  const IntRect&);

    virtual void adjustListboxStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual void adjustMenuListStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintMenuList(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustMenuListButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintMenuListButton(RenderObject*, const PaintInfo&, const IntRect&);
    
    virtual void adjustSearchFieldStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchField(RenderObject*, const PaintInfo&, const IntRect&);

    virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);
    virtual bool paintSliderThumb(RenderObject*, const PaintInfo&, const IntRect&);

private:
    RenderThemeAndroid();
    void addIntrinsicMargins(RenderStyle*) const;
    void close();

    bool supportsFocus(ControlPart);
    friend RenderTheme* theme();
    /*void paintItem(RenderObject*, const PaintInfo&, IntRect, int listIndex);*/
    /*guoxiaolei 20120827 end>*/

};

} // namespace WebCore

#endif // RenderThemeAndroid_h
