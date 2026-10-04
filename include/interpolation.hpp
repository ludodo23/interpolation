/*
 * License: CeCILL-C
 *
 * Copyright (c) 2026 Ludovic Andrieux
 * contributor(s): Ludovic Andrieux (2026)
 *
 * ludovic.andrieux23@gmail.com
 *
 * This software is a header-only C++ interpolation library provided as a
 * single header file. It offers templated interpolators for generic value
 * types (e.g. double, Vector2, Vector3, Eigen::VectorXd) and implements
 * several interpolation methods such as Linear, Hermite and Catmull-Rom.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software. You can use,
 * modify and/or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL:
 * https://www.cecill.info
 *
 * This software is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <limits>
#include <functional>
#include <utility>

#ifndef INTERP_ERROR
#define INTERP_ERROR(msg) throw std::runtime_error(msg)
#endif

#define INTERPOLATION_VERSION "0.3.0"

/// @brief Interpolation module.
namespace interpolation {

/**
 * @brief Version string.
 */
inline const char* version() {
    return INTERPOLATION_VERSION;
}

// ================================
// Utility functions
// ================================

/**
 * @brief Generic linear interpolation for scalar/vector-like T.
 *
 * Requires:
 *   T * double
 *   T + T
 */
template <typename T>
constexpr T lerp(const T& a, const T& b, double t) {
    return a * (1.0 - t) + b * t;
}

/**
 * @brief Generic clamp for scalar/vector-like T.
 *
 * Requires:
 *   T supports operator<.
 */
template <typename T>
constexpr const T& clamp(
    const T& v,
    const T& lo,
    const T& hi
) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

// ================================
// Interval search strategies
// ================================

/**
 * @brief Abstract interval search strategy.
 *
 * Returns i such that:
 *
 *     x[i] <= X < x[i+1]
 *
 * Boundary values are clamped to the first/last interval.
 */
class IntervalSearch {
public:
    explicit IntervalSearch(
        std::shared_ptr<const std::vector<double>> x
    )
        : x_(std::move(x)) {

        if (!x_) {
            INTERP_ERROR("IntervalSearch: x is null");
        }

        if (x_->size() < 2) {
            INTERP_ERROR(
                "IntervalSearch: x vector should have at least two points"
            );
        }

        for (size_t i = 1; i < x_->size(); ++i) {
            if (!((*x_)[i] > (*x_)[i - 1])) {
                INTERP_ERROR(
                    "Interpolator: x must be strictly increasing"
                );
            }
        }
    }

    IntervalSearch() = delete;
    virtual ~IntervalSearch() = default;

    int find(double X) const {
        if (X <= x_->front()) {
            return get_first();
        }

        if (X >= x_->back()) {
            return get_last();
        }

        return find_impl(X);
    }

    const std::vector<double>& x() const {
        return *x_;
    }

    std::shared_ptr<const std::vector<double>> xpointer() const {
        return x_;
    }

protected:
    virtual int get_first() const {
        return 0;
    }

    virtual int get_last() const {
        return static_cast<int>(x_->size()) - 2;
    }

    virtual int find_impl(double X) const = 0;

    std::shared_ptr<const std::vector<double>> x_;
};

/**
 * @brief Binary search.
 */
class BinarySearchInterval : public IntervalSearch {
public:
    explicit BinarySearchInterval(
        std::shared_ptr<const std::vector<double>> x
    )
        : IntervalSearch(std::move(x)) {}

protected:
    int find_impl(double X) const override {
        auto it = std::upper_bound(
            x_->begin(),
            x_->end(),
            X
        );

        return static_cast<int>(it - x_->begin()) - 1;
    }
};

/**
 * @brief Cached interval search efficient for monotonic queries.
 *
 * Keeps the last interval index and walks forward/backward from it.
 */
class LinearCachedIntervalSearch : public IntervalSearch {
public:
    explicit LinearCachedIntervalSearch(
        std::shared_ptr<const std::vector<double>> x
    )
        : IntervalSearch(std::move(x)) {}

protected:
    int find_impl(double X) const override {
        int i = static_cast<int>(last_);

        if (X >= (*x_)[i + 1]) {
            while (
                i + 1 < static_cast<int>(x_->size()) - 1 &&
                X >= (*x_)[i + 1]
            ) {
                ++i;
            }

            last_ = static_cast<size_t>(i);
        }
        else {
            while (i > 0 && X < (*x_)[i]) {
                --i;
            }

            last_ = static_cast<size_t>(i);
        }

        return i;
    }

    int get_first() const override {
        last_ = 0;
        return 0;
    }

    int get_last() const override {
        last_ = x_->size() - 2;
        return static_cast<int>(last_);
    }

private:
    mutable size_t last_{0};
};

/**
 * @brief Uniform grid interval search.
 *
 * Assumes constant spacing.
 */
class UniformGridIntervalSearch : public IntervalSearch {
public:
    explicit UniformGridIntervalSearch(
        std::shared_ptr<const std::vector<double>> x
    )
        : IntervalSearch(std::move(x)) {

        x0_ = (*x_)[0];
        dx_ = (*x_)[1] - (*x_)[0];

        if (dx_ <= 0.0) {
            INTERP_ERROR(
                "UniformGridIntervalSearch: dx must be > 0"
            );
        }

        // Verify uniformity.
        for (size_t i = 2; i < x_->size(); ++i) {
            const double dx = (*x_)[i] - (*x_)[i - 1];

            if (!std::isfinite(dx) ||
                std::abs(dx - dx_) >
                    std::numeric_limits<double>::epsilon() *
                    std::max(1.0, std::abs(dx_))) {
                INTERP_ERROR(
                    "UniformGridIntervalSearch: x must be uniformly spaced"
                );
            }
        }
    }

protected:
    int find_impl(double X) const override {
        const int i = static_cast<int>(
            std::floor((X - x0_) / dx_)
        );

        return std::clamp(
            i,
            0,
            static_cast<int>(x_->size()) - 2
        );
    }

private:
    double x0_{0.0};
    double dx_{0.0};
};

// ================================
// Interpolator base
// ================================

/**
 * @brief Base class for interpolators.
 *
 * @tparam T Value type.
 *
 * Examples:
 *   double
 *   Eigen::Vector3d
 *   Eigen::VectorXd
 */
template <typename T>
class Interpolator {
protected:
    std::shared_ptr<const std::vector<double>> x_;
    std::shared_ptr<const std::vector<T>> y_;
    std::shared_ptr<IntervalSearch> search_;

public:
    Interpolator(
        std::shared_ptr<const std::vector<double>> x,
        std::shared_ptr<const std::vector<T>> y
    )
        : x_(std::move(x)),
          y_(std::move(y)) {

        if (!x_ || !y_) {
            INTERP_ERROR(
                "Interpolator: x or y is null"
            );
        }

        if (x_->size() != y_->size()) {
            INTERP_ERROR(
                "Interpolator: x.size() != y.size()"
            );
        }

        if (x_->size() < 2) {
            INTERP_ERROR(
                "Interpolator: at least two points are required"
            );
        }

        search_ =
            std::make_shared<LinearCachedIntervalSearch>(x_);
    }

    Interpolator(
        std::shared_ptr<const std::vector<T>> y,
        std::shared_ptr<IntervalSearch> s
    )
        : y_(std::move(y)),
          search_(std::move(s)) {

        if (!y_) {
            INTERP_ERROR(
                "Interpolator: y is null"
            );
        }

        if (!search_) {
            INTERP_ERROR(
                "Interpolator: search is null"
            );
        }

        x_ = search_->xpointer();

        if (x_->size() != y_->size()) {
            INTERP_ERROR(
                "Interpolator: x.size() != y.size()"
            );
        }
    }

    Interpolator() = delete;
    virtual ~Interpolator() = default;

    /**
     * @brief Find the interpolation interval.
     */
    virtual int search(double X) const {
        return search_->find(X);
    }

    /**
     * @brief Evaluate at a single X.
     */
    virtual T eval(double X) const = 0;

    /**
     * @brief Evaluate a batch of points.
     */
    virtual void eval_batch(
        const double* X,
        int n,
        T* outY
    ) const {
        for (int i = 0; i < n; ++i) {
            outY[i] = eval(X[i]);
        }
    }

    /**
     * @brief Convenience batch evaluation.
     */
    std::vector<T> eval_batch(
        const std::vector<double>& X
    ) const {
        std::vector<T> Y;
        Y.reserve(X.size());

        for (double v : X) {
            Y.push_back(eval(v));
        }

        return Y;
    }

    const std::vector<double>& xdata() const {
        return *x_;
    }

    const std::vector<T>& ydata() const {
        return *y_;
    }
};

// ================================
// Linear interpolator
// ================================

/**
 * @brief Piecewise linear interpolator.
 */
template <typename T>
class LinearInterpolator : public Interpolator<T> {
    using Interpolator<T>::x_;
    using Interpolator<T>::y_;

    bool extrapolate_;

public:
    LinearInterpolator(
        std::shared_ptr<const std::vector<double>> x,
        std::shared_ptr<const std::vector<T>> y,
        bool extrapolate = false
    )
        : Interpolator<T>(
            std::move(x),
            std::move(y)
        ),
          extrapolate_(extrapolate) {}

    LinearInterpolator(
        std::shared_ptr<const std::vector<T>> y,
        std::shared_ptr<IntervalSearch> s,
        bool extrapolate = false
    )
        : Interpolator<T>(
            std::move(y),
            std::move(s)
        ),
          extrapolate_(extrapolate) {}

    T eval(double X) const override {
        const int i = this->search(X);

        const double x0 = (*x_)[i];
        const double x1 = (*x_)[i + 1];

        double t = (X - x0) / (x1 - x0);

        if (!extrapolate_) {
            t = clamp(t, 0.0, 1.0);
        }

        return lerp(
            (*y_)[i],
            (*y_)[i + 1],
            t
        );
    }
};

// ================================
// Cubic Hermite
// ================================

/**
 * @brief Piecewise cubic Hermite interpolator.
 *
 * The interpolation is defined by:
 *
 *     y(x0), y'(x0)
 *     y(x1), y'(x1)
 *
 * for each segment.
 *
 * In particular, derivative() is the exact derivative of eval().
 *
 * T must support:
 *
 *     T * double
 *     T + T
 */
template <typename T>
class CubicHermiteInterpolator : public Interpolator<T> {
protected:
    using Interpolator<T>::x_;
    using Interpolator<T>::y_;

    std::shared_ptr<const std::vector<T>> dy_dx_;

    /**
     * @brief Get interval and normalized coordinate.
     */
    void coefficients(
        double X,
        int& i,
        double& h,
        double& t
    ) const {
        i = this->search(X);

        const double x0 = (*x_)[i];
        const double x1 = (*x_)[i + 1];

        h = x1 - x0;

        t = (X - x0) / h;
        t = clamp(t, 0.0, 1.0);
    }

public:
    CubicHermiteInterpolator(
        std::shared_ptr<const std::vector<double>> x,
        std::shared_ptr<const std::vector<T>> y,
        std::shared_ptr<const std::vector<T>> dy_dx
    )
        : Interpolator<T>(
            std::move(x),
            std::move(y)
        ),
          dy_dx_(std::move(dy_dx)) {
        check();
    }

    CubicHermiteInterpolator(
        std::shared_ptr<const std::vector<T>> y,
        std::shared_ptr<const std::vector<T>> dy_dx,
        std::shared_ptr<IntervalSearch> s
    )
        : Interpolator<T>(
            std::move(y),
            std::move(s)
        ),
          dy_dx_(std::move(dy_dx)) {
        check();
    }

    void check() {
        if (!dy_dx_) {
            INTERP_ERROR(
                "CubicHermiteInterpolator: slopes null"
            );
        }

        if (dy_dx_->size() != y_->size()) {
            INTERP_ERROR(
                "CubicHermiteInterpolator: slopes.size() mismatch"
            );
        }
    }

    /**
     * @brief Evaluate y(X).
     */
    T eval(double X) const override {
        int i;
        double h;
        double t;

        coefficients(X, i, h, t);

        const double t2 = t * t;
        const double t3 = t2 * t;

        const double h00 =
            2.0 * t3 - 3.0 * t2 + 1.0;

        const double h10 =
            t3 - 2.0 * t2 + t;

        const double h01 =
            -2.0 * t3 + 3.0 * t2;

        const double h11 =
            t3 - t2;

        return
            (*y_)[i] * h00
            + (*dy_dx_)[i] * (h * h10)
            + (*y_)[i + 1] * h01
            + (*dy_dx_)[i + 1] * (h * h11);
    }

    /**
     * @brief Evaluate dy/dx.
     *
     * This is the exact derivative of eval().
     */
    T derivative(double X) const {
        int i;
        double h;
        double t;

        coefficients(X, i, h, t);

        const double t2 = t * t;

        const double dh00 =
            6.0 * t2 - 6.0 * t;

        const double dh10 =
            3.0 * t2 - 4.0 * t + 1.0;

        const double dh01 =
            -6.0 * t2 + 6.0 * t;

        const double dh11 =
            3.0 * t2 - 2.0 * t;

        return
            (*y_)[i] * (dh00 / h)
            + (*dy_dx_)[i] * dh10
            + (*y_)[i + 1] * (dh01 / h)
            + (*dy_dx_)[i + 1] * dh11;
    }

    /**
     * @brief Evaluate d²y/dx².
     *
     * This is the exact second derivative of eval().
     */
    T second_derivative(double X) const {
        int i;
        double h;
        double t;

        coefficients(X, i, h, t);

        const double d2h00 =
            12.0 * t - 6.0;

        const double d2h10 =
            6.0 * t - 4.0;

        const double d2h01 =
            -12.0 * t + 6.0;

        const double d2h11 =
            6.0 * t - 2.0;

        return
            (*y_)[i] * (d2h00 / (h * h))
            + (*dy_dx_)[i] * (d2h10 / h)
            + (*y_)[i + 1] * (d2h01 / (h * h))
            + (*dy_dx_)[i + 1] * (d2h11 / h);
    }
};

// ================================
// Quintic Hermite
// ================================

/**
 * @brief Piecewise quintic Hermite interpolator.
 *
 * Each segment is defined by:
 *
 *     y(x0),  y'(x0),  y''(x0)
 *     y(x1),  y'(x1),  y''(x1)
 *
 * The resulting polynomial is C2 continuous provided the supplied
 * derivatives are themselves continuous at the nodes.
 *
 * This is particularly useful for physical trajectories where:
 *
 *     position     = y
 *     velocity     = y'
 *     acceleration = y''
 *
 * T must support:
 *
 *     T * double
 *     T + T
 */
template <typename T>
class QuinticHermiteInterpolator : public Interpolator<T> {
protected:
    using Interpolator<T>::x_;
    using Interpolator<T>::y_;

    std::shared_ptr<const std::vector<T>> dy_dx_;
    std::shared_ptr<const std::vector<T>> d2y_dx2_;

    /**
     * @brief Get interval and normalized coordinate.
     */
    void coefficients(
        double X,
        int& i,
        double& h,
        double& t
    ) const {
        i = this->search(X);

        const double x0 = (*x_)[i];
        const double x1 = (*x_)[i + 1];

        h = x1 - x0;

        t = (X - x0) / h;
        t = clamp(t, 0.0, 1.0);
    }

public:
    QuinticHermiteInterpolator(
        std::shared_ptr<const std::vector<double>> x,
        std::shared_ptr<const std::vector<T>> y,
        std::shared_ptr<const std::vector<T>> dy_dx,
        std::shared_ptr<const std::vector<T>> d2y_dx2
    )
        : Interpolator<T>(
            std::move(x),
            std::move(y)
        ),
          dy_dx_(std::move(dy_dx)),
          d2y_dx2_(std::move(d2y_dx2)) {
        check();
    }

    QuinticHermiteInterpolator(
        std::shared_ptr<const std::vector<T>> y,
        std::shared_ptr<const std::vector<T>> dy_dx,
        std::shared_ptr<const std::vector<T>> d2y_dx2,
        std::shared_ptr<IntervalSearch> s
    )
        : Interpolator<T>(
            std::move(y),
            std::move(s)
        ),
          dy_dx_(std::move(dy_dx)),
          d2y_dx2_(std::move(d2y_dx2)) {
        check();
    }

    void check() {
        if (!dy_dx_) {
            INTERP_ERROR(
                "QuinticHermiteInterpolator: slopes null"
            );
        }

        if (!d2y_dx2_) {
            INTERP_ERROR(
                "QuinticHermiteInterpolator: second slopes null"
            );
        }

        if (dy_dx_->size() != y_->size()) {
            INTERP_ERROR(
                "QuinticHermiteInterpolator: "
                "slopes.size() mismatch"
            );
        }

        if (d2y_dx2_->size() != y_->size()) {
            INTERP_ERROR(
                "QuinticHermiteInterpolator: "
                "second slopes.size() mismatch"
            );
        }
    }

    /**
     * @brief Evaluate y(X).
     */
    T eval(double X) const override {
        int i;
        double h;
        double t;

        coefficients(X, i, h, t);

        const double t2 = t * t;
        const double t3 = t2 * t;
        const double t4 = t3 * t;
        const double t5 = t4 * t;

        // Position basis functions.
        const double h00 =
            1.0
            - 10.0 * t3
            + 15.0 * t4
            - 6.0 * t5;

        const double h01 =
            10.0 * t3
            - 15.0 * t4
            + 6.0 * t5;

        // First derivative basis functions.
        const double h10 =
            t
            - 6.0 * t3
            + 8.0 * t4
            - 3.0 * t5;

        const double h11 =
            -4.0 * t3
            + 7.0 * t4
            - 3.0 * t5;

        // Second derivative basis functions.
        const double h20 =
            0.5 * t2
            - 1.5 * t3
            + 1.5 * t4
            - 0.5 * t5;

        const double h21 =
            0.5 * t3
            - t4
            + 0.5 * t5;

        return
            (*y_)[i] * h00
            + (*dy_dx_)[i] * (h * h10)
            + (*d2y_dx2_)[i] * (h * h * h20)
            + (*y_)[i + 1] * h01
            + (*dy_dx_)[i + 1] * (h * h11)
            + (*d2y_dx2_)[i + 1] * (h * h * h21);
    }

    /**
     * @brief Evaluate dy/dx.
     *
     * This is the exact first derivative of eval().
     */
    T derivative(double X) const {
        int i;
        double h;
        double t;

        coefficients(X, i, h, t);

        const double t2 = t * t;
        const double t3 = t2 * t;
        const double t4 = t3 * t;

        // Derivatives of the position basis functions.
        const double dh00 =
            -30.0 * t2
            + 60.0 * t3
            - 30.0 * t4;

        const double dh01 =
            30.0 * t2
            - 60.0 * t3
            + 30.0 * t4;

        // Derivatives of the first derivative basis functions.
        const double dh10 =
            1.0
            - 18.0 * t2
            + 32.0 * t3
            - 15.0 * t4;

        const double dh11 =
            -12.0 * t2
            + 28.0 * t3
            - 15.0 * t4;

        // Derivatives of the second derivative basis functions.
        const double dh20 =
            t
            - 4.5 * t2
            + 6.0 * t3
            - 2.5 * t4;

        const double dh21 =
            1.5 * t2
            - 4.0 * t3
            + 2.5 * t4;

        return
            (*y_)[i] * (dh00 / h)
            + (*dy_dx_)[i] * dh10
            + (*d2y_dx2_)[i] * (h * dh20)
            + (*y_)[i + 1] * (dh01 / h)
            + (*dy_dx_)[i + 1] * dh11
            + (*d2y_dx2_)[i + 1] * (h * dh21);
    }

    /**
     * @brief Evaluate d²y/dx².
     *
     * This is the exact second derivative of eval().
     */
    T second_derivative(double X) const {
        int i;
        double h;
        double t;

        coefficients(X, i, h, t);

        const double t2 = t * t;
        const double t3 = t2 * t;

        // Second derivatives of position basis functions.
        const double d2h00 =
            -60.0 * t
            + 180.0 * t2
            - 120.0 * t3;

        const double d2h01 =
            60.0 * t
            - 180.0 * t2
            + 120.0 * t3;

        // Second derivatives of first derivative basis functions.
        const double d2h10 =
            -36.0 * t
            + 96.0 * t2
            - 60.0 * t3;

        const double d2h11 =
            -24.0 * t
            + 84.0 * t2
            - 60.0 * t3;

        // Second derivatives of second derivative basis functions.
        const double d2h20 =
            1.0
            - 9.0 * t
            + 18.0 * t2
            - 10.0 * t3;

        const double d2h21 =
            3.0 * t
            - 12.0 * t2
            + 10.0 * t3;

        return
            (*y_)[i] * (d2h00 / (h * h))
            + (*dy_dx_)[i] * (d2h10 / h)
            + (*d2y_dx2_)[i] * d2h20
            + (*y_)[i + 1] * (d2h01 / (h * h))
            + (*dy_dx_)[i + 1] * (d2h11 / h)
            + (*d2y_dx2_)[i + 1] * d2h21;
    }
};

// ================================
// Catmull-Rom
// ================================

/**
 * @brief Catmull-Rom spline.
 *
 * Slopes are estimated from neighbouring values.
 */
template <typename T>
class CatmullRomInterpolator : public Interpolator<T> {
    using Interpolator<T>::y_;
    using Interpolator<T>::x_;

public:
    CatmullRomInterpolator(
        std::shared_ptr<const std::vector<double>> x,
        std::shared_ptr<const std::vector<T>> y
    )
        : Interpolator<T>(
            std::move(x),
            std::move(y)
        ) {}

    CatmullRomInterpolator(
        std::shared_ptr<const std::vector<T>> y,
        std::shared_ptr<IntervalSearch> s
    )
        : Interpolator<T>(
            std::move(y),
            std::move(s)
        ) {}

    T eval(double X) const override {
        const int i = this->search(X);

        const double x0 = (*x_)[i];
        const double x1 = (*x_)[i + 1];

        const double xm1 =
            (i == 0)
                ? (*x_)[0]
                : (*x_)[i - 1];

        const double x2 =
            (i + 2 >= static_cast<int>(y_->size()))
                ? (*x_)[i + 1]
                : (*x_)[i + 2];

        const double h = x1 - x0;

        double t = (X - x0) / h;
        t = clamp(t, 0.0, 1.0);

        const double t2 = t * t;
        const double t3 = t2 * t;

        const double h00 =
            2.0 * t3 - 3.0 * t2 + 1.0;

        const double h10 =
            t3 - 2.0 * t2 + t;

        const double h01 =
            -2.0 * t3 + 3.0 * t2;

        const double h11 =
            t3 - t2;

        const T ym1 =
            (i == 0)
                ? (*y_)[0]
                : (*y_)[i - 1];

        const T y2 =
            (i + 2 >= static_cast<int>(y_->size()))
                ? (*y_)[i + 1]
                : (*y_)[i + 2];

        const T y0 = (*y_)[i];
        const T y1 = (*y_)[i + 1];

        const T m0 =
            (y1 - ym1) / (x1 - xm1);

        const T m1 =
            (y2 - y0) / (x2 - x0);

        return
            h00 * y0
            + h10 * (m0 * h)
            + h01 * y1
            + h11 * (m1 * h);
    }
};

} // namespace interpolation

// EOF