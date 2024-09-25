#pragma once

#include "common.h"
#include "frame_processors.h"
#include "array_view.h"

namespace discord {
namespace dave {
namespace codec_utils {

bool ProcessFrameOpus(OutboundFrameProcessor& processor, ArrayView<const uint8_t> frame);
bool ProcessFrameVp8(OutboundFrameProcessor& processor, ArrayView<const uint8_t> frame);
bool ProcessFrameVp9(OutboundFrameProcessor& processor, ArrayView<const uint8_t> frame);
bool ProcessFrameH264(OutboundFrameProcessor& processor, ArrayView<const uint8_t> frame);
bool ProcessFrameH265(OutboundFrameProcessor& processor, ArrayView<const uint8_t> frame);
bool ProcessFrameAv1(OutboundFrameProcessor& processor, ArrayView<const uint8_t> frame);

bool ValidateEncryptedFrame(OutboundFrameProcessor& processor, ArrayView<uint8_t> frame);

} // namespace codec_utils
} // namespace dave
} // namespace discord
