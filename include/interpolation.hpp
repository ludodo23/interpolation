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
 * As a counterpart to the access to the source code and rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty and the software's author, the holder of the
 * economic rights, and the successive licensors have only limited
 * liability.
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading, using, modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean that it is complicated to manipulate, and that also
 * therefore means that it is reserved for developers and experienced
 * professionals having in-depth computer knowledge.
 * 
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-C license and that you accept its terms.
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

#define INTERPOLATION_VERSION "0.1.0"


/// @brief Interpolation module.
namespace interpolation {

/**
 * @brief Version string
 */
inline const char* version() { return INTERPOLATION_VERSION; }

// ================================
// Utility functions
// ================================

/**
 * @brief Generic lerp for scalar/vector-like T.
 * Requires T supports: T * double, T + T
 * 
 * Full template on a and b here (not case with lerp of numeric)
 */
template <typename T>
constexpr T lerp(const T &a, const T &b, double t) {
    return a * (1.0 - t) + b * t;
}

#if __cplusplus < 201703L
/**
 * @brief Generic clamp for scalar/vector-like T.
 * Requires T supports: T * double, T + T
 */
template <typename T>
constexpr const T& clamp(const T &v, const T & lo, const T & hi) {
    return (v < lo) ? lo : (hi < v) ? hi : v;
}
#else
using std::clamp; // from <algorithm>
#endif

// ================================
// Interval search strategies
// ================================

/**
 * @brief Abstract interval search strategy.
 * Should return i s.t. x[i] <= X < x[i+1].
 */
class IntervalSearch {
public:
    IntervalSearch(std::shared_ptr<const std::vector<double>> x) : x_(std::move(x)) {
        if (x_->size() < 2) {
            INTERP_ERROR("IntervalSearch: x vector should have at least two points !");
        }
        // verify increasing x
        for (size_t i = 1; i < x_->size(); ++i)
            if (!((*x_)[i] > (*x_)[i - 1])) {
                INTERP_ERROR("Interpolator: x must be strictly increasing");
            }
    }
    IntervalSearch() = delete;
    virtual ~IntervalSearch() = default;
    virtual int find(double X) const final {
        if (X <= x_->front()) {
            return get_first();
        }
        if (X >= x_->back()) {
            return get_last();
        }

        return find_impl(X);
    }

    const std::vector<double>& x() const { return *x_; }
    std::shared_ptr<const std::vector<double>> xpointer() const {return x_;}
    
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
    BinarySearchInterval(std::shared_ptr<const std::vector<double>> x) : IntervalSearch(x) {}
protected:
    int find_impl(double X) const override {
        // upper_bound use a binary search.
        auto it = std::upper_bound(x_->begin(), x_->end(), X); // first element in x > X
        return static_cast<int>(it - x_->begin()) - 1;  // O(1) instead of std::distance which O(1) is not guaranted.
    }
};

/**
 * @brief Cached interval search that is efficient for monotonic queries.
 * Keeps last index and walks forward/backwards.
 */
class CachedInterval : public IntervalSearch {
public:
    CachedInterval(std::shared_ptr<const std::vector<double>> x) : IntervalSearch(x) {}
protected:
    int find_impl(double X) const override {
        int i = static_cast<int>(last_);
        // we should never have i = n - 1.
        if (X >= (*x_)[i+1]) {
            while (i + 1 < x_->size() - 1 && X >= (*x_)[i+1]) {
                ++i;
            }
            last_ = i;
        } else {
            while (i > 0 && X < (*x_)[i]) {
                --i;
            }
            last_ = i; 
        }

        return i;
    }
    
    /**
     * @brief get first indice of vector.
     * 
     * Override IntervalSearch to update cache.
    */
    virtual int get_first() const override {
        last_ = IntervalSearch::get_first();
        return static_cast<int>(last_);
    }
    /**
     * @brief get last indice of vector.
     * 
     * Override IntervalSearch to update cache.
    */
    virtual int get_last() const override {
        last_ = IntervalSearch::get_last();
        return static_cast<int>(last_);
    }
private:
    mutable size_t last_{0};
};

/**
 * @brief Uniform grid: constant spacing dx starting at x0.
 * Faster O(1) index compute.
 */
class UniformGridInterval : public IntervalSearch {
public:
    UniformGridInterval(std::shared_ptr<const std::vector<double>> x) : IntervalSearch(x) {
            x0_ = (*x_)[0];
            dx_ = (*x_)[1] - (*x_)[0];
            if (dx_ <= 0.0) {
            INTERP_ERROR("UniformGridInterval: dx must be > 0");
        }
    }
protected:
    int find_impl(double X) const override {
        int i = int(std::floor((X - x0_) / dx_));
        return std::clamp(i, 0, int(x_->size()) - 2);
    }
private:
    double x0_, dx_;
};

// ================================
// Interpolator base (template) - holds shared_ptr to data
// ================================

/**
 * @brief Base class for templated interpolators.
 * @tparam T value type (double, Vector2, Vector3, Eigen vector...)
 */
template <typename T>
class Interpolator {
protected:
    std::shared_ptr<const std::vector<double>> x_; // shared, const
    std::shared_ptr<const std::vector<T>> y_; // shared, const
    std::shared_ptr<IntervalSearch> search_;

public:
    /**
     * @brief Construct from shared pointers (no copy).
     * @param x shared_ptr to vector<double> (x nodes) - must be sorted ascending
     * @param y shared_ptr to vector<T> (y values)
     * @param s search strategy (if nullptr, default CachedInterval is used)
     */
    Interpolator(std::shared_ptr<const std::vector<double>> x,
                 std::shared_ptr<const std::vector<T>> y)
        : x_(std::move(x)), y_(std::move(y)) {
        if (!x_ || !y_) {
            INTERP_ERROR("Interpolator: x or y is null");
        }
        if (x_->size() != y_->size()) {
            INTERP_ERROR("Interpolator: x.size() != y.size()");
        }
        search_ = std::make_shared<CachedInterval>(x_);
    }
    Interpolator(std::shared_ptr<const std::vector<T>> y,
                 std::shared_ptr<IntervalSearch> s)
        : y_(std::move(y)), search_(std::move(s)) {
        
        x_ = search_->xpointer();

        if (!y_) {
            INTERP_ERROR("Interpolator: y is null");
        }
        if (x_->size() != y_->size()) {
            INTERP_ERROR("Interpolator: x.size() != y.size()");
        }
    }

    Interpolator() = delete;

    virtual ~Interpolator() = default;

    /// search indice
    virtual int search(double X) const {
        return search_->find(X);
    }

    /// Evaluate at a single X
    virtual T eval(double X) const = 0;

    /// Evaluate batch - default calls eval per point (override for performance)
    virtual void eval_batch(const double* X, int n, T* outY) const {
        for (int i = 0; i < n; ++i) outY[i] = eval(X[i]);
    }

    /// convenience: evaluate from vector<double> to vector<T>
    std::vector<T> eval_batch(const std::vector<double> &X) const {
        std::vector<T> Y; 
        Y.reserve(X.size());
        for (double v : X) {
            Y.push_back(eval(v));
        }
        return Y;
    }

    const std::vector<double>& xdata() const { return *x_; }
    const std::vector<T>& ydata() const { return *y_; }
};

// ================================
// Linear interpolator
// ================================

/**
 * @brief Piecewise linear interpolator
 */
template <typename T>
class LinearInterpolator : public Interpolator<T> {
    using Interpolator<T>::x_;
    using Interpolator<T>::y_;
    using Interpolator<T>::search_;
    bool extrapolate_;

public:

    LinearInterpolator(std::shared_ptr<const std::vector<double>> x,
                       std::shared_ptr<const std::vector<T>> y,
                       bool extrapolate = false)
        : Interpolator<T>(x, y), extrapolate_(extrapolate) {
        
    }
    LinearInterpolator(std::shared_ptr<const std::vector<T>> y,
                       std::shared_ptr<IntervalSearch> s,
                       bool extrapolate = false) :
        Interpolator<T>(y, s), extrapolate_(extrapolate) {

    }

    T eval(double X) const override {
        int i = this->search(X);
        double x0 = (*x_)[i], x1 = (*x_)[i + 1];
        double t = (X - x0) / (x1 - x0);
        if (!extrapolate_) {
            t = clamp(t, 0.0, 1.0);
        }
        return lerp((*y_)[i], (*y_)[i + 1], t);
    }
};

// ================================
// Hermite cubic per-segment
// ================================

/**
 * @brief Hermite cubic (per segment) interpolator.
 * T must support multiplication by double and addition.
 */
template <typename T>
class CubicHermiteInterpolator : public Interpolator<T> {
protected:
    using Interpolator<T>::x_;
    using Interpolator<T>::y_;
    using Interpolator<T>::search_;

    // slopes at nodes (y' values) - stored as shared_ptr to avoid copying if user wants
    std::shared_ptr<const std::vector<T>> dy_dx_;

public:
    /**
     * @brief Construct Hermite interpolator with explicit slopes.
     * @param x shared_ptr to x nodes
     * @param y shared_ptr to y values
     * @param slopes shared_ptr to slopes at nodes (size == n)
     */
    CubicHermiteInterpolator(
        std::shared_ptr<const std::vector<double>> x,
        std::shared_ptr<const std::vector<T>> y,
        std::shared_ptr<const std::vector<T>> dy_dx
        ) : Interpolator<T>(std::move(x), std::move(y)), dy_dx_(std::move(dy_dx)) {
        check();
    }
    CubicHermiteInterpolator(
        std::shared_ptr<const std::vector<T>> y,
        std::shared_ptr<const std::vector<T>> dy_dx,
        std::shared_ptr<IntervalSearch> s
        ) : Interpolator<T>(std::move(y), std::move(s)), dy_dx_(std::move(dy_dx)) {
        check();
    }
    void check() {
        if (!dy_dx_) {
            INTERP_ERROR("CubicHermiteInterpolator: slopes null");
        }
        if (dy_dx_->size() != y_->size()) {
            INTERP_ERROR("CubicHermiteInterpolator: slopes.size() mismatch");
        }
    }

    /**
     * @brief Evaluate Hermite cubic at X
     */
    T eval(double X) const override {
        int i = this->search(X);
        double x0 = (*x_)[i], x1 = (*x_)[i + 1];
        double h = x1 - x0;
        double t = (X - x0) / h;
        double t2 = t * t, t3 = t2 * t;

        double h00 = 2 * t3 - 3 * t2 + 1;
        double h10 = t3 - 2 * t2 + t;
        double h01 = -2 * t3 + 3 * t2;
        double h11 = t3 - t2;

        // Note: dy_dx_ stores derivative dy/dx; multiply by h for Hermite basis
        T res = ((*y_)[i]) * h00 + (((*dy_dx_)[i]) * (h * h10)) + (((*y_)[i + 1]) * h01) + (((*dy_dx_)[i + 1]) * (h * h11));
        return res;
    }
};

// ================================
// Catmull-Rom // TODO : non uniform Catmull-Rom.
// ================================

/**
 * @brief Catmull-Rom spline: Hermite with slopes estimated by neighbor differences.
 * Slopes computed as m_i = 0.5 * (y_{i+1} - y_{i-1})
 */
template <typename T>
class CatmullRomInterpolator : public Interpolator<T> {
    using Interpolator<T>::y_;
    using Interpolator<T>::x_;

public:
    CatmullRomInterpolator(
        std::shared_ptr<const std::vector<double>> x,
        std::shared_ptr<const std::vector<T>> y
        ) : Interpolator<T>(
            std::move(x),
            std::move(y)
        ) {
    }
    CatmullRomInterpolator(
        std::shared_ptr<const std::vector<T>> y,
        std::shared_ptr<IntervalSearch> s
        ) : Interpolator<T>(
            std::move(y),
            std::move(s)
            ) {
    }

    T eval(double X) const override {
        int i = this->search(X);
        double x0 = (*x_)[i], x1 = (*x_)[i + 1];
        double h = x1 - x0;
        double t = (X - x0) / h;
        double t2 = t * t, t3 = t2 * t;

        double h00 = 2 * t3 - 3 * t2 + 1;
        double h10 = t3 - 2 * t2 + t;
        double h01 = -2 * t3 + 3 * t2;
        double h11 = t3 - t2;

        T ym1 = (i == 0) ? (*y_)[0] : (*y_)[i-1];
        T y2  = (i+2 >= y_->size()) ? (*y_)[i+1] : (*y_)[i+2];
        T y0 = (*y_)[i];
        T y1 = (*y_)[i+1];

        // non uniform Catmull-Rom. 
        T m0 = (y1 - ym1) / (x1 - x_{i-1});
        T m1 = (y2 - y0) / (x_{i+2} - x0);

        T res = h00 * y0
              + h10 * (m0 * h)
              + h01 * y1
              + h11 * (m1 * h);

        return res;
    }

};


// ================================
// End of header
// ================================

} // namespace interpolation

// EOF
