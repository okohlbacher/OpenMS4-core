// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#include <OpenMS/IMAGING/IonImage.h>

#include <OpenMS/CONCEPT/Exception.h>
#include <OpenMS/DATASTRUCTURES/StringUtils.h>

namespace OpenMS
{

  namespace
  {
    /// Pixel ceiling for a single ion image: 2^31 pixels, about 17 GB of intensities. Whole-slide
    /// rasters stay far below it (a 76 x 26 mm slide at 5 um is 79 million pixels), while a corrupt
    /// or hostile header declaring billions of rows and columns is refused here instead of being
    /// handed to the allocator. The dimensions of an extracted image come from the file being read.
    const Size MAX_IMAGE_PIXELS = Size(1) << 31;
  } // namespace

  IonImage::IonImage(UInt width, UInt height)
  {
    resize(width, height);
  }

  void IonImage::resize(UInt width, UInt height)
  {
    const Size n = static_cast<Size>(width) * static_cast<Size>(height);
    if (n > MAX_IMAGE_PIXELS)
    {
      throw Exception::InvalidValue(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
                                    "Ion image dimensions exceed the supported pixel count ("
                                      + StringUtils::toStr(MAX_IMAGE_PIXELS) + ")",
                                    StringUtils::toStr(width) + "x" + StringUtils::toStr(height));
    }
    width_ = width;
    height_ = height;
    intensities_.assign(n, 0.0);
    mask_.assign(n, false);
  }

  UInt IonImage::getWidth() const
  {
    return width_;
  }

  UInt IonImage::getHeight() const
  {
    return height_;
  }

  bool IonImage::hasPixel(UInt x, UInt y) const
  {
    if (x >= width_ || y >= height_) return false;
    return mask_[linearIndex_(x, y)];
  }

  double IonImage::getIntensity(UInt x, UInt y) const
  {
    return intensities_[linearIndex_(x, y)];
  }

  void IonImage::setIntensity(UInt x, UInt y, double intensity)
  {
    const Size idx = linearIndex_(x, y);
    intensities_[idx] = intensity;
    mask_[idx] = true;
  }

  void IonImage::setMzRange(const RangeMZ& range)
  {
    mz_range_ = range;
  }

  const RangeMZ& IonImage::getMzRange() const
  {
    return mz_range_;
  }

  const std::vector<double>& IonImage::getData() const
  {
    return intensities_;
  }

  const std::vector<bool>& IonImage::getMask() const
  {
    return mask_;
  }

  Size IonImage::linearIndex_(UInt x, UInt y) const
  {
    if (x >= width_ || y >= height_)
    {
      const Size attempted = static_cast<Size>(y) * static_cast<Size>(width_ ? width_ : 1)
                             + static_cast<Size>(x);
      throw Exception::IndexOverflow(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
                                     static_cast<SignedSize>(attempted),
                                     intensities_.size());
    }
    return static_cast<Size>(y) * static_cast<Size>(width_) + static_cast<Size>(x);
  }

} // namespace OpenMS
