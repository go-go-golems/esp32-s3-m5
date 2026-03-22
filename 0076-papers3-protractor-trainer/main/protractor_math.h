#pragma once

#include <cstddef>
#include <vector>

namespace protractor_demo {

struct PointF {
    float x = 0.0f;
    float y = 0.0f;
};

struct VectorizedGesture {
    std::vector<float> vector;
    PointF centroid;
    float indicative_angle_rad = 0.0f;
    float rotation_delta_rad = 0.0f;
    bool valid = false;
};

float PathLength(const std::vector<PointF>& points);
PointF Centroid(const std::vector<PointF>& points);
std::vector<PointF> Resample(const std::vector<PointF>& points, std::size_t target_count);
VectorizedGesture Vectorize(const std::vector<PointF>& points, bool orientation_sensitive = false);
float OptimalCosineDistance(const std::vector<float>& lhs, const std::vector<float>& rhs);

}  // namespace protractor_demo
