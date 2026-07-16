#include "s3paper/fake_backend.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "s3paper/frame_arena.h"

namespace s3paper {

FakeBackend::FakeBackend(char *trace_buffer, uint32_t trace_capacity,
                         Size physical_size)
    : trace_(trace_buffer), trace_capacity_(trace_buffer ? trace_capacity : 0),
      physical_size_(physical_size) {
    if (trace_capacity_ > 0) {
        trace_[0] = '\0';
    }
}

void FakeBackend::ClearTrace() {
    trace_len_ = 0;
    trace_truncated_ = false;
    if (trace_capacity_ > 0) {
        trace_[0] = '\0';
    }
}

void FakeBackend::Append(const char *fmt, ...) {
    if (trace_capacity_ == 0 || trace_len_ + 1 >= trace_capacity_) {
        trace_truncated_ = true;
        return;
    }
    va_list args;
    va_start(args, fmt);
    const int written = vsnprintf(trace_ + trace_len_,
                                  trace_capacity_ - trace_len_, fmt, args);
    va_end(args);
    if (written < 0) {
        trace_truncated_ = true;
        return;
    }
    if (static_cast<uint32_t>(written) >= trace_capacity_ - trace_len_) {
        trace_len_ = trace_capacity_ - 1;
        trace_truncated_ = true;
        return;
    }
    trace_len_ += static_cast<uint32_t>(written);
}

Status FakeBackend::Init() {
    initialized_ = true;
    Append("init size=%dx%d\n", physical_size_.w, physical_size_.h);
    return OkStatus();
}

PresentResult FakeBackend::Present(const RenderFrame &frame,
                                   PresentIntent intent) {
    PresentResult result{};
    result.frame_id = frame.id;
    if (!initialized_) {
        result.status = StatusCode::Busy;
        return result;
    }
    Append("present id=%u intent=%s ops=%u damage=%d,%d,%d,%d\n",
           static_cast<unsigned>(frame.id), PresentIntentName(intent),
           static_cast<unsigned>(frame.op_count), frame.damage.x,
           frame.damage.y, frame.damage.w, frame.damage.h);
    for (uint32_t i = 0; i < frame.op_count; ++i) {
        const DrawOp &op = frame.ops[i];
        Append("op kind=%s gray=%u bounds=%d,%d,%d,%d clip=%d,%d,%d,%d",
               DrawOpKindName(op.kind), static_cast<unsigned>(op.gray),
               op.bounds.x, op.bounds.y, op.bounds.w, op.bounds.h, op.clip.x,
               op.clip.y, op.clip.w, op.clip.h);
        switch (op.kind) {
            case DrawOpKind::StrokeRect:
                Append(" thickness=%d", op.payload.stroke.thickness);
                break;
            case DrawOpKind::GlyphRun: {
                const GlyphRunPayload &g = op.payload.glyph_run;
                Append(" baseline=%d font=%u size=%u text=\"", g.baseline_y,
                       static_cast<unsigned>(g.font_id),
                       static_cast<unsigned>(g.size_px));
                if (frame.arena != nullptr) {
                    const char *text = reinterpret_cast<const char *>(
                        frame.arena->Data(g.text_offset));
                    for (uint32_t c = 0; c < g.text_len; ++c) {
                        const char ch = text[c];
                        if (ch >= 0x20 && ch < 0x7f && ch != '"') {
                            Append("%c", ch);
                        } else {
                            Append("\\x%02x", static_cast<unsigned char>(ch));
                        }
                    }
                }
                Append("\"");
                break;
            }
            default:
                break;
        }
        Append("\n");
        result.ops_drawn++;
    }
    frames_presented_++;
    result.status = StatusCode::Ok;
    result.damage = frame.damage;
    return result;
}

BackendState FakeBackend::GetState() const {
    return BackendState{initialized_, physical_size_, frames_presented_};
}

const char *PresentIntentName(PresentIntent intent) {
    switch (intent) {
        case PresentIntent::InteractiveInk: return "InteractiveInk";
        case PresentIntent::TextRegion: return "TextRegion";
        case PresentIntent::TextPage: return "TextPage";
        case PresentIntent::ImageQuality: return "ImageQuality";
        case PresentIntent::CleanFull: return "CleanFull";
    }
    return "Unknown";
}

}  // namespace s3paper
