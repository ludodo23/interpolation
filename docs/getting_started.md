# Getting Started — `interpolation.hpp`

> A lightweight, header-only C++ interpolation library by [Ludovic Andrieux](https://github.com/ludodo23).  
> Version: `0.2.0` — License: CeCILL-C

---

## Installation

This is a **header-only** library — no build step required. Just copy the file.

```bash
# Clone the repository
git clone https://github.com/ludodo23/interpolation.git
```

Then include it in your project:

```cpp
#include "interpolation/include/interpolation.hpp"
```

> Requires **C++17** or later.

---

## Key Concepts

The library is fully **generic** (`template <typename T>`): it works with `double`, `float`, or any custom vector type (Eigen, glm, etc.) as long as it supports `+` and `* double`.

---

## 1. Linear Interpolation

```cpp
#include "interpolation.hpp"
using namespace interpolation;

auto x = std::make_shared<const std::vector<double>>(std::vector<double>{0.0, 1.0, 2.0, 3.0});
auto y = std::make_shared<const std::vector<double>>(std::vector<double>{0.0, 1.0, 4.0, 9.0});

LinearInterpolator<double> interp(x, y);

double val = interp.eval(1.5); // ≈ 2.5
```

Evaluate over a batch of points:

```cpp
std::vector<double> xs = {0.5, 1.5, 2.5};
auto ys = interp.eval_batch(xs);
```

By default, values are **clamped** to the data range. To enable extrapolation:

```cpp
LinearInterpolator<double> interp(x, y, /*extrapolate=*/true);
```

---

## 2. Catmull-Rom Spline

Produces a smooth curve with **no need to supply derivatives** — slopes are estimated automatically from neighboring points.

```cpp
CatmullRomInterpolator<double> spline(x, y);

double val = spline.eval(1.5);
```

---

## 3. Cubic Hermite Spline

Use this when you know the **exact derivative at each node**:

```cpp
auto slopes = std::make_shared<const std::vector<double>>(
    std::vector<double>{0.0, 2.0, 4.0, 6.0} // dy/dx at each node
);

CubicHermiteInterpolator<double> hermite(x, y, slopes);

double val = hermite.eval(1.5);
```

---

## 4. Interval Search Strategies

The default strategy is `LinearCachedIntervalSearch`, which is efficient for sequential (monotonic) queries. You can override it:

| Strategy | Best for |
|---|---|
| `LinearCachedIntervalSearch` | Sequential / monotonic queries (default) |
| `BinarySearchInterval` | Random access, general purpose |
| `UniformGridIntervalSearch` | Uniformly spaced data — O(1) lookup |

```cpp
// Binary search
auto search = std::make_shared<BinarySearchInterval>(x);

// Uniform grid (fastest when spacing is constant)
auto search = std::make_shared<UniformGridIntervalSearch>(x);

// Pass the strategy to any interpolator
LinearInterpolator<double> interp(y, search);
```

---

## 5. Custom Types (e.g. Eigen)

```cpp
#include <Eigen/Dense>
using Vec3 = Eigen::Vector3d;

auto x = std::make_shared<const std::vector<double>>(std::vector<double>{0.0, 1.0, 2.0});
auto y = std::make_shared<const std::vector<Vec3>>(std::vector<Vec3>{
    Vec3(0, 0, 0), Vec3(1, 2, 3), Vec3(2, 4, 6)
});

CatmullRomInterpolator<Vec3> spline(x, y);
Vec3 val = spline.eval(0.5);
```

> The type `T` must support `T * double` and `T + T`.

---

## Check the Version

```cpp
std::cout << interpolation::version(); // "0.2.0"
```

---

## Error Handling

The library throws `std::runtime_error` on invalid input (mismatched sizes, non-strictly-increasing x, null pointers, etc.).

You can override the error macro before including the header:

```cpp
#define INTERP_ERROR(msg) myCustomHandler(msg)
#include "interpolation.hpp"
```

---

## Quick Reference

| Class | Description |
|---|---|
| `LinearInterpolator<T>` | Piecewise linear interpolation |
| `CatmullRomInterpolator<T>` | Smooth spline, auto-estimated slopes |
| `CubicHermiteInterpolator<T>` | Smooth spline, user-provided slopes |
| `BinarySearchInterval` | General-purpose interval search |
| `LinearCachedIntervalSearch` | Fast for sequential queries (default) |
| `UniformGridIntervalSearch` | O(1) for uniform grids |
