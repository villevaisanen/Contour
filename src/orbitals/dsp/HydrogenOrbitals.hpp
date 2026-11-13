#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace orbital {
constexpr double pi = 3.14159265358979323846;
struct Vec3 {
    double x, y, z;
};
constexpr int count = 14;
static const char *const names[count] = {"1s",    "2s",    "2p_x",     "2p_y", "2p_z",
                                         "3s",    "3p_x",  "3p_y",     "3p_z", "3d_xy",
                                         "3d_xz", "3d_yz", "3d_x2-y2", "3d_z2"};
inline double bounded(double x, double lo, double hi) noexcept {
    return std::isfinite(x) ? std::max(lo, std::min(hi, x)) : 0.;
}
// R_nl times a real solid harmonic: no spherical-coordinate singularities.
inline double evaluate(int id, Vec3 q) noexcept {
    if (id < 0 || id >= count)
        return 0.;
    const double r = std::hypot(std::hypot(q.x, q.y), q.z);
    if (!std::isfinite(r) || r > 2000.)
        return 0.;
    const int n = id == 0 ? 1 : (id < 5 ? 2 : 3);
    const int l = (id == 0 || id == 1 || id == 5) ? 0 : (id < 9 ? 1 : 2);
    const double rho = 2. * r / n;
    const int k = n - l - 1;
    double lag = 1.;
    if (k == 1)
        lag = 2 * l + 2 - rho;
    if (k == 2)
        lag = .5 * (rho * rho - 2 * (2 * l + 3) * rho + (2 * l + 3) * (2 * l + 2));
    static const double fact[] = {1, 1, 2, 6, 24, 120};
    const double norm = std::sqrt(std::pow(2. / n, 3) * fact[k] / (2 * n * fact[n + l]));
    double solid = 1. / std::sqrt(4 * pi);
    if (l == 1) {
        int axis = n == 2 ? id - 2 : id - 6;
        solid = std::sqrt(3. / (4 * pi)) * (axis == 0 ? q.x : axis == 1 ? q.y : q.z);
    }
    if (l == 2) {
        const double c = std::sqrt(15. / (4 * pi));
        switch (id) {
        case 9:
            solid = c * q.x * q.y;
            break;
        case 10:
            solid = c * q.x * q.z;
            break;
        case 11:
            solid = c * q.y * q.z;
            break;
        case 12:
            solid = .5 * c * (q.x * q.x - q.y * q.y);
            break;
        default:
            solid = std::sqrt(5. / (16 * pi)) * (2 * q.z * q.z - q.x * q.x - q.y * q.y);
        }
    }
    return norm * std::exp(-r / n) * std::pow(2. / n, l) * lag * solid;
}
inline std::array<double, count> weights(int a, int b, double mix) noexcept {
    std::array<double, count> w{};
    a = std::max(0, std::min(count - 1, a));
    b = std::max(0, std::min(count - 1, b));
    mix = bounded(mix, 0, 1);
    if (a == b)
        w[a] = 1.;
    else {
        w[a] = std::cos(pi * .5 * mix);
        w[b] = std::sin(pi * .5 * mix);
    }
    return w;
}
} // namespace orbital
