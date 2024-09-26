#pragma once

#include "common.h"
#include "frame_processors.h"
#include "array_view.h"

namespace dpp::dave::codec_utils {

bool process_frame_opus(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);
bool process_frame_vp8(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);
bool process_frame_vp9(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);
bool process_frame_h264(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);
bool process_frame_h265(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);
bool process_frame_av1(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);

bool validate_encrypted_frame(OutboundFrameProcessor& processor, array_view<uint8_t> frame);

} // namespace dpp::dave::codec_utils


