// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// $Maintainer: OpenMS Team $
#include <OpenMS/CHEMISTRY/AASequence.h>
#include <OpenMS/CONCEPT/VersionInfo.h>
#include <OpenMS/FORMAT/MzMLFile.h>
#include <OpenMS/KERNEL/MSExperiment.h>
#include <filesystem>

int main()
{
  OpenMS::MSSpectrum spectrum;
  spectrum.setRT(12.5);
  spectrum.setMSLevel(1);
  OpenMS::Peak1D first;
  first.setMZ(200.0);
  first.setIntensity(42.0);
  OpenMS::Peak1D second;
  second.setMZ(100.0);
  second.setIntensity(21.0);
  spectrum.push_back(first);
  spectrum.push_back(second);
  spectrum.sortByPosition();
  if (spectrum[0].getMZ() != 100.0 || spectrum[1].getIntensity() != 42.0) { return 1; }

  // This crosses the shared-library boundary and uses installed XML resources.
  OpenMS::MSExperiment original;
  original.addSpectrum(spectrum);
  const std::string filename = "sdk-roundtrip.mzML";
  OpenMS::MzMLFile().store(filename, original);
  OpenMS::MSExperiment restored;
  OpenMS::MzMLFile().load(filename, restored);
  std::filesystem::remove(filename);
  if (restored.size() != 1 || restored[0].size() != 2 || restored[0].getRT() != 12.5) { return 2; }
  if (restored[0][0].getMZ() != 100.0 || restored[0][1].getIntensity() != 42.0) { return 3; }

  // Modified sequence parsing requires the chemistry databases in the SDK.
  const auto peptide = OpenMS::AASequence::fromString("M(Oxidation)PEPTIDE");
  if (peptide.size() != 8 || ! peptide.isModified() || peptide.getMonoWeight() <= 0.0) { return 4; }
  return OpenMS::VersionInfo::getVersionStruct().version_major == 4 ? 0 : 5;
}
