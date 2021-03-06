/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef SKY_ENGINE_CORE_ANIMATION_ANIMATION_H_
#define SKY_ENGINE_CORE_ANIMATION_ANIMATION_H_

#include "sky/engine/core/animation/AnimationEffect.h"
#include "sky/engine/core/animation/AnimationNode.h"
#include "sky/engine/core/animation/EffectInput.h"
#include "sky/engine/core/animation/TimingInput.h"
#include "sky/engine/wtf/RefPtr.h"

namespace blink {

class Element;
class ExceptionState;
class SampledEffect;

class Animation final : public AnimationNode {
    DEFINE_WRAPPERTYPEINFO();
public:
    enum Priority { DefaultPriority, TransitionPriority };

    static PassRefPtr<Animation> create(Element*, PassRefPtr<AnimationEffect> effect, const Timing&, Priority = DefaultPriority, PassOwnPtr<EventDelegate> = nullptr);
    // Web Animations API Bindings constructors.
    static PassRefPtr<Animation> create(Element*, ExceptionState&);
    static PassRefPtr<Animation> create(Element*, double duration, ExceptionState&);

    virtual ~Animation();

    virtual bool isAnimation() const override { return true; }

    bool affects(CSSPropertyID) const;
    const AnimationEffect* effect() const { return m_effect.get(); }
    AnimationEffect* effect() { return m_effect.get(); }
    Priority priority() const { return m_priority; }
    Element* target() { return m_target; }

    void notifySampledEffectRemovedFromAnimationStack();
    void notifyElementDestroyed();

protected:
    void applyEffects();
    void clearEffects();
    virtual void updateChildrenAndEffects() const override;
    virtual void attach(AnimationPlayer*) override;
    virtual void detach() override;
    virtual void specifiedTimingChanged() override;
    virtual double calculateTimeToEffectChange(bool forwards, double inheritedTime, double timeToNextIteration) const override;

private:
    Animation(Element*, PassRefPtr<AnimationEffect> effect, const Timing&, Priority, PassOwnPtr<EventDelegate>);

    RawPtr<Element> m_target;
    RefPtr<AnimationEffect> m_effect;
    RawPtr<SampledEffect> m_sampledEffect;

    Priority m_priority;

    friend class AnimationAnimationV8Test;
};

DEFINE_TYPE_CASTS(Animation, AnimationNode, animationNode, animationNode->isAnimation(), animationNode.isAnimation());

} // namespace blink

#endif  // SKY_ENGINE_CORE_ANIMATION_ANIMATION_H_
