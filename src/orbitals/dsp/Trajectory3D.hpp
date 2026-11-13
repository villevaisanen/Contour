#pragma once
#include "HydrogenOrbitals.hpp"
namespace orbital {
struct Path {
    double radius = 6., offset = 1., shape = .35, tilt = .2;
};
// Shape/tilt trigonometry is invariant across a host sample's four substeps.
struct PreparedPath {
    double radius, offset, yScale, zScale, sy, cy, sz, cz, st, ct;
    explicit PreparedPath(const Path &p) noexcept {
        const double s = bounded(p.shape, 0, 1);
        radius = p.radius;
        offset = p.offset;
        yScale = .35 + .65 * s;
        zScale = 1. - .7 * s;
        sy = std::sin(pi * s);
        cy = std::cos(pi * s);
        sz = std::sin(.5 * pi * s);
        cz = std::cos(.5 * pi * s);
        st = std::sin(pi * p.tilt);
        ct = std::cos(pi * p.tilt);
    }
    Vec3 evaluate(double phase) const noexcept {
        const double t = 2 * pi * phase, a = std::sin(t), c = std::cos(t);
        const double s2 = 2 * a * c, c2 = c * c - a * a;
        const double s3 = s2 * c + c2 * a, c3 = c2 * c - s2 * a;
        Vec3 q{radius * a, radius * yScale * (s2 * cy + c2 * sy),
               radius * zScale * (s3 * cz + c3 * sz)};
        const double y = ct * q.y - st * q.z;
        q.z = st * q.y + ct * q.z;
        q.y = y;
        q.x += offset;
        return q;
    }
};
inline Vec3 trajectory(double phase, const Path &p) noexcept {
    return PreparedPath(p).evaluate(phase);
}
inline Vec3 fieldCoordinates(Vec3 p, double spin) noexcept {
    const double c = std::cos(2 * pi * spin), s = std::sin(2 * pi * spin);
    return {c * p.x + s * p.y, -s * p.x + c * p.y, p.z};
}
} // namespace orbital
