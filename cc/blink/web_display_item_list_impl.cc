// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/blink/web_display_item_list_impl.h"

#include <vector>

#include "cc/blink/web_blend_mode.h"
#include "cc/resources/clip_display_item.h"
#include "cc/resources/drawing_display_item.h"
#include "cc/resources/filter_display_item.h"
#include "cc/resources/transform_display_item.h"
#include "cc/resources/transparency_display_item.h"
#include "skia/ext/refptr.h"
#include "third_party/WebKit/public/platform/WebFloatRect.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "third_party/skia/include/core/SkImageFilter.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/utils/SkMatrix44.h"
#include "ui/gfx/transform.h"

namespace cc_blink {

WebDisplayItemListImpl::WebDisplayItemListImpl()
    : display_item_list_(cc::DisplayItemList::Create()) {
}

scoped_refptr<cc::DisplayItemList> WebDisplayItemListImpl::ToDisplayItemList() {
  return display_item_list_;
}

void WebDisplayItemListImpl::appendDrawingItem(
    SkPicture* picture,
    const blink::WebFloatPoint& location) {
  display_item_list_->AppendItem(
      cc::DrawingDisplayItem::Create(skia::SharePtr(picture), location));
}

void WebDisplayItemListImpl::appendClipItem(
    const blink::WebRect& clip_rect,
    const blink::WebVector<SkRRect>& rounded_clip_rects) {
  std::vector<SkRRect> rounded_rects;
  for (size_t i = 0; i < rounded_clip_rects.size(); ++i) {
    rounded_rects.push_back(rounded_clip_rects[i]);
  }
  display_item_list_->AppendItem(
      cc::ClipDisplayItem::Create(clip_rect, rounded_rects));
}

void WebDisplayItemListImpl::appendEndClipItem() {
  display_item_list_->AppendItem(cc::EndClipDisplayItem::Create());
}

void WebDisplayItemListImpl::appendTransformItem(const SkMatrix44& matrix) {
  gfx::Transform transform;
  transform.matrix() = matrix;
  display_item_list_->AppendItem(cc::TransformDisplayItem::Create(transform));
}

void WebDisplayItemListImpl::appendTransparencyItem(
    float opacity,
    blink::WebBlendMode blend_mode) {
  display_item_list_->AppendItem(cc::TransparencyDisplayItem::Create(
      opacity, BlendModeToSkia(blend_mode)));
}

void WebDisplayItemListImpl::appendEndTransformItem() {
  display_item_list_->AppendItem(cc::EndTransformDisplayItem::Create());
}

void WebDisplayItemListImpl::appendEndTransparencyItem() {
  display_item_list_->AppendItem(cc::EndTransparencyDisplayItem::Create());
}

void WebDisplayItemListImpl::appendFilterItem(
    SkImageFilter* filter,
    const blink::WebFloatRect& bounds) {
  display_item_list_->AppendItem(
      cc::FilterDisplayItem::Create(skia::SharePtr(filter), bounds));
}

void WebDisplayItemListImpl::appendEndFilterItem() {
  display_item_list_->AppendItem(cc::EndFilterDisplayItem::Create());
}

WebDisplayItemListImpl::~WebDisplayItemListImpl() {
}

}  // namespace cc_blink
