// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Julianus Pfeuffer $
// $Authors: Julianus Pfeuffer $
// --------------------------------------------------------------------------
//

#include <OpenMS/MATH/STATISTICS/GumbelMaxLikelihoodFitter.h>
#include <OpenMS/CONCEPT/Exception.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

using namespace std;

namespace OpenMS::Math
{
    namespace
    {
      /// log(sum w * exp(-x / b)) with the largest exponent factored out, and the mean of x under
      /// those tilted weights, so neither overflows nor underflows for a small scale b
      std::pair<double, double> tiltedSums_(const std::vector<double>& x, const std::vector<double>& w, const double b)
      {
        double max_exponent = -std::numeric_limits<double>::infinity();
        for (Size i = 0; i < x.size(); ++i)
        {
          if (w[i] > 0.0) max_exponent = std::max(max_exponent, -x[i] / b);
        }
        double sum = 0.0;
        double weighted = 0.0;
        for (Size i = 0; i < x.size(); ++i)
        {
          if (w[i] <= 0.0) continue;
          const double factor = w[i] * std::exp(-x[i] / b - max_exponent);
          sum += factor;
          weighted += factor * x[i];
        }
        return {max_exponent + std::log(sum), weighted / sum};
      }
    }

    GumbelMaxLikelihoodFitter::GumbelDistributionFitResult GumbelMaxLikelihoodFitter::fitWeighted(const std::vector<double> & x, const std::vector<double> & w)
    {
      // one weight per value: the old objective walked the weights alongside the values and read
      // past the end of a shorter weight vector
      if (x.size() != w.size())
      {
        throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
          "Gumbel fit needs one weight per value (" + std::to_string(x.size()) + " values, " + std::to_string(w.size()) + " weights)");
      }
      double total = 0.0;
      double mean = 0.0;
      double x_min = std::numeric_limits<double>::infinity();
      double x_max = -std::numeric_limits<double>::infinity();
      for (Size i = 0; i < x.size(); ++i)
      {
        if (!std::isfinite(x[i]) || !std::isfinite(w[i]) || w[i] < 0.0)
        {
          throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
            "Gumbel fit needs finite values and finite, non-negative weights");
        }
        if (w[i] == 0.0) continue;
        total += w[i];
        mean += w[i] * x[i];
        x_min = std::min(x_min, x[i]);
        x_max = std::max(x_max, x[i]);
      }
      if (!(total > 0.0))
      {
        return init_param_; // no weighted data: nothing to fit, the parameters stay as they are
      }
      if (!(x_max > x_min))
      {
        throw Exception::UnableToFit(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, "UnableToFit-GumbelMaxLikelihoodFitter",
          "Could not fit the gumbel distribution: fewer than two distinct values carry weight");
      }
      mean /= total;

      // The likelihood equations of the Gumbel (maximum) distribution give the location in closed
      // form, a = -b * log(sum w exp(-x / b) / sum w), and leave the scale as the root of
      // g(b) = b - mean + tilted mean(b), which rises monotonically from x_min - mean < 0 towards
      // +infinity. The fitter used to hand the negative log-likelihood to Levenberg-Marquardt, which
      // minimises its square: that shares the minimum only while the negative log-likelihood stays
      // positive, and for a narrow sample (scale well below 1) it settles on the contour where the
      // negative log-likelihood is zero instead of on the maximum-likelihood estimate.
      const auto g = [&](const double b) { return b - mean + tiltedSums_(x, w, b).second; };
      const double spread = x_max - x_min;
      double lo = spread * 1e-12;
      double hi = spread;
      while (g(hi) <= 0.0)
      {
        hi *= 2.0; // g(b) >= b - (mean - x_min), so doubling ends
      }
      for (int iteration = 0; iteration < 200 && hi - lo > 1e-15 * hi; ++iteration)
      {
        const double mid = 0.5 * (lo + hi);
        if (g(mid) > 0.0)
        {
          hi = mid;
        }
        else
        {
          lo = mid;
        }
      }
      const double b = 0.5 * (lo + hi);
      const double a = -b * (tiltedSums_(x, w, b).first - std::log(total));

      init_param_.a = a;
      init_param_.b = b;
      return {a, b};
    }

    double GumbelMaxLikelihoodFitter::GumbelDistributionFitResult::log_eval_no_normalize(const double x) const
    {
      // -log b is a constant again
      double diff = (x - a)/b;
      return -log(b) - diff - exp(- diff);
    }

    GumbelMaxLikelihoodFitter::GumbelMaxLikelihoodFitter():
    init_param_({0.25, 0.1})
    {}

    GumbelMaxLikelihoodFitter::GumbelMaxLikelihoodFitter(GumbelMaxLikelihoodFitter::GumbelDistributionFitResult init):
        init_param_(init)
    {}

    GumbelMaxLikelihoodFitter::~GumbelMaxLikelihoodFitter() = default;

    void GumbelMaxLikelihoodFitter::setInitialParameters(const GumbelDistributionFitResult & param)
    {
      init_param_ = param;
    }

} // namespace OpenMS   //namespace Math
