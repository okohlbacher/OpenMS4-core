// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// $Maintainer: OpenMS Team $
#include <OpenMS/DATASTRUCTURES/MatrixEigen.h>

int main()
{
  // The SDK exports this scientific interoperability header and Eigen include path.
  OpenMS::Matrix<double> matrix(2, 2, 0.0);
  auto view = OpenMS::eigenView(matrix);
  view.setIdentity();
  view *= 3.0;
  if (matrix(0, 0) != 3.0 || matrix(1, 1) != 3.0 || matrix(0, 1) != 0.0) { return 1; }
  Eigen::Vector2d input(2.0, 4.0);
  const Eigen::Vector2d result = view * input;
  return result[0] == 6.0 && result[1] == 12.0 ? 0 : 2;
}
