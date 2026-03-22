#include "protractor_math.h"

#include <algorithm>
#include <cmath>

namespace protractor_demo {

namespace {

constexpr float kPi = 3.14159265358979323846f;

float ClampUnit(float value)
{
    return std::clamp(value, -1.0f, 1.0f);
}

}  // namespace

float PathLength(const std::vector<PointF>& points)
{
    float total = 0.0f;
    for (std::size_t i = 1; i < points.size(); ++i) {
        const float dx = points[i].x - points[i - 1].x;
        const float dy = points[i].y - points[i - 1].y;
        total += std::sqrt(dx * dx + dy * dy);
    }
    return total;
}

PointF Centroid(const std::vector<PointF>& points)
{
    if (points.empty()) {
        return {};
    }

    float sum_x = 0.0f;
    float sum_y = 0.0f;
    for (const auto& point : points) {
        sum_x += point.x;
        sum_y += point.y;
    }

    return {
        sum_x / static_cast<float>(points.size()),
        sum_y / static_cast<float>(points.size()),
    };
}

std::vector<PointF> Resample(const std::vector<PointF>& points, std::size_t target_count)
{
    if (points.empty() || target_count == 0) {
        return {};
    }

    if (points.size() == 1) {
        return std::vector<PointF>(target_count, points.front());
    }

    std::vector<PointF> work = points;
    std::vector<PointF> resampled;
    resampled.reserve(target_count);
    resampled.push_back(work.front());

    const float interval = PathLength(work) / static_cast<float>(target_count - 1);
    float distance_accumulator = 0.0f;

    std::size_t i = 1;
    while (i < work.size()) {
        const float dx = work[i].x - work[i - 1].x;
        const float dy = work[i].y - work[i - 1].y;
        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance == 0.0f) {
            ++i;
            continue;
        }

        if ((distance_accumulator + distance) >= interval) {
            const float t = (interval - distance_accumulator) / distance;
            const PointF interpolated = {
                work[i - 1].x + t * dx,
                work[i - 1].y + t * dy,
            };
            resampled.push_back(interpolated);
            work.insert(work.begin() + static_cast<std::ptrdiff_t>(i), interpolated);
            distance_accumulator = 0.0f;
            ++i;
            continue;
        }

        distance_accumulator += distance;
        ++i;
    }

    while (resampled.size() < target_count) {
        resampled.push_back(work.back());
    }

    if (resampled.size() > target_count) {
        resampled.resize(target_count);
    }

    return resampled;
}

VectorizedGesture Vectorize(const std::vector<PointF>& points, bool orientation_sensitive)
{
    VectorizedGesture result;
    if (points.size() < 2) {
        return result;
    }

    result.centroid = Centroid(points);

    std::vector<PointF> centered;
    centered.reserve(points.size());
    for (const auto& point : points) {
        centered.push_back({point.x - result.centroid.x, point.y - result.centroid.y});
    }

    result.indicative_angle_rad = std::atan2(centered.front().y, centered.front().x);
    if (orientation_sensitive) {
        const float base = (kPi / 4.0f) *
                           std::floor((result.indicative_angle_rad + (kPi / 8.0f)) / (kPi / 4.0f));
        result.rotation_delta_rad = base - result.indicative_angle_rad;
    } else {
        result.rotation_delta_rad = -result.indicative_angle_rad;
    }

    const float cos_delta = std::cos(result.rotation_delta_rad);
    const float sin_delta = std::sin(result.rotation_delta_rad);

    float magnitude_squared = 0.0f;
    result.vector.reserve(centered.size() * 2);
    for (const auto& point : centered) {
        const float nx = point.x * cos_delta - point.y * sin_delta;
        const float ny = point.y * cos_delta + point.x * sin_delta;
        result.vector.push_back(nx);
        result.vector.push_back(ny);
        magnitude_squared += nx * nx + ny * ny;
    }

    const float magnitude = std::sqrt(magnitude_squared);
    if (magnitude == 0.0f) {
        return result;
    }

    for (auto& component : result.vector) {
        component /= magnitude;
    }

    result.valid = true;
    return result;
}

float OptimalCosineDistance(const std::vector<float>& lhs, const std::vector<float>& rhs)
{
    if (lhs.size() != rhs.size() || lhs.empty() || (lhs.size() % 2) != 0) {
        return kPi;
    }

    float a = 0.0f;
    float b = 0.0f;
    for (std::size_t i = 0; i < lhs.size(); i += 2) {
        a += lhs[i] * rhs[i] + lhs[i + 1] * rhs[i + 1];
        b += lhs[i] * rhs[i + 1] - lhs[i + 1] * rhs[i];
    }

    const float angle = std::atan2(b, a);
    const float value = ClampUnit(a * std::cos(angle) + b * std::sin(angle));
    return std::acos(value);
}

}  // namespace protractor_demo
