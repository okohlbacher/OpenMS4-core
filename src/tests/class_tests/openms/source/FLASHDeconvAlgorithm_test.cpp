// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Tom David Müller, Jaekwan Kim$
// $Authors: Tom David Müller, Jaekwan Kim$
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/test_config.h>

///////////////////////////

#include <OpenMS/ANALYSIS/TOPDOWN/FLASHDeconvAlgorithm.h>
#include <OpenMS/DATASTRUCTURES/ListUtils.h>
#include <OpenMS/FORMAT/ParamXMLFile.h>
#include <OpenMS/FORMAT/MzMLFile.h>
#include <memory>

///////////////////////////

START_TEST(FLASHDeconvAlgorithm, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

using namespace OpenMS;
using namespace std;

std::unique_ptr<FLASHDeconvAlgorithm> ptr;
FLASHDeconvAlgorithm* null_ptr = nullptr;

START_SECTION(FLASHDeconvAlgorithm())
  ptr = std::make_unique<FLASHDeconvAlgorithm>();
  TEST_NOT_EQUAL(ptr.get(), null_ptr)
END_SECTION

START_SECTION(FLASHDeconvAlgorithm(const FLASHDeconvAlgorithm& source))
FLASHDeconvAlgorithm copy(*ptr);
  TEST_EQUAL(copy.getParameters(), ptr->getParameters())
END_SECTION

START_SECTION(FLASHDeconvAlgorithm& operator=(const FLASHDeconvAlgorithm& source))
FLASHDeconvAlgorithm copy;
  copy = *ptr;
  TEST_EQUAL(copy.getParameters(), ptr->getParameters())
END_SECTION

START_SECTION(~FLASHDeconvAlgorithm())
  ptr.reset();
END_SECTION

ptr = std::make_unique<FLASHDeconvAlgorithm>();
START_SECTION(FLASHDeconvAlgorithm(FLASHDeconvAlgorithm&& source))
  FLASHDeconvAlgorithm temp;
  temp.setParameters(ptr->getParameters());
  FLASHDeconvAlgorithm moved(std::move(temp));
  TEST_EQUAL(moved.getParameters(), ptr->getParameters());
END_SECTION

START_SECTION(FLASHDeconvAlgorithm& operator=(FLASHDeconvAlgorithm&& source))
  FLASHDeconvAlgorithm temp;
  temp.setParameters(ptr->getParameters());
  FLASHDeconvAlgorithm moved;
  moved = std::move(temp);
  TEST_EQUAL(moved.getParameters(), ptr->getParameters());
END_SECTION

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

ptr = std::make_unique<FLASHDeconvAlgorithm>();
Param params;
START_SECTION(std::vector<double> getTolerances())
  params.setValue("SD:tol", ListUtils::create<double>("10.0,5.0"));
  ptr->setParameters(params);
  auto tolerances = ptr->getTolerances();
  TEST_EQUAL(tolerances.size(), 2)
  TEST_REAL_SIMILAR(tolerances[0], 10.0);
  TEST_REAL_SIMILAR(tolerances[1], 5.0);
END_SECTION

// load test data
// TODO: minimize
PeakMap input;
MzMLFile().load(OPENMS_GET_TEST_DATA_PATH("FLASHDeconv_1_input.mzML"), input);

// Store FD outputs
std::vector<DeconvolvedSpectrum> deconvolved_spectra;
std::vector<FLASHHelperClasses::MassFeature> deconvolved_features;

START_SECTION(void run())
  ptr->run(input, deconvolved_spectra, deconvolved_features);
  TEST_EQUAL(deconvolved_spectra.size(), input.size());
  TEST_FALSE(deconvolved_features.empty());
END_SECTION

START_SECTION(int getScanNumber())
  // The scan number depends on the test file; just verify it returns a positive value
  TEST_EQUAL(FLASHDeconvAlgorithm::getScanNumber(input, 0) > 0, true);
END_SECTION

// Check if the precalculated averagine was initialized during runtime
START_SECTION(FLASHHelperClasses::PrecalculatedAveragine getAveragine())
  TEST_NOT_EQUAL(ptr->getAveragine().getMaxIsotopeIndex(), 0);
  TEST_FALSE(ptr->getAveragine().get(500.0).getContainer().empty());
END_SECTION

START_SECTION(FLASHHelperClasses::PrecalculatedAveragine getDecoyAveragine())
  TEST_NOT_EQUAL(ptr->getDecoyAveragine().getMaxIsotopeIndex(), 0);
  TEST_FALSE(ptr->getDecoyAveragine().get(500.0).getContainer().empty());
END_SECTION

// Decoy Averagine needs to be handled differently if FDR is reported
START_SECTION(FLASHHelperClasses::PrecalculatedAveragine getDecoyAveragine())
  params.setValue("report_FDR", "true");
  ptr = std::make_unique<FLASHDeconvAlgorithm>();
  ptr->setParameters(params);
  ptr->run(input, deconvolved_spectra, deconvolved_features);
  TEST_NOT_EQUAL(ptr->getDecoyAveragine().getMaxIsotopeIndex(), 0);
  TEST_FALSE(ptr->getDecoyAveragine().get(500.0).getContainer().empty());
END_SECTION


/////////////////////////////////////////////////////////////
// Shared deterministic results
//
// FLASHDeconv is deterministic (the TOPDOWN module contains no randomness),
// so every section that runs with identical parameters on the same input
// produces identical output. To keep this test fast we compute each distinct
// configuration only once here and reuse the result in the read-only checks
// below, instead of re-running the full deconvolution in every section.
/////////////////////////////////////////////////////////////

// default-parameter result (protein averagine, no merging, no FDR reporting)
FLASHDeconvAlgorithm algo_default;
std::vector<DeconvolvedSpectrum> default_spectra;
std::vector<FLASHHelperClasses::MassFeature> default_features;
algo_default.run(input, default_spectra, default_features);

// FDR-reporting result (report_FDR = true)
FLASHDeconvAlgorithm algo_fdr;
Param algo_fdr_params;
algo_fdr_params.setValue("report_FDR", "true");
algo_fdr.setParameters(algo_fdr_params);
std::vector<DeconvolvedSpectrum> fdr_spectra;
std::vector<FLASHHelperClasses::MassFeature> fdr_features;
algo_fdr.run(input, fdr_spectra, fdr_features);

/////////////////////////////////////////////////////////////
// Phase 1: Averagine Model Tests (Enhanced)
/////////////////////////////////////////////////////////////

START_SECTION(Averagine model - protein vs RNA mode initialization)
  // Protein averagine (use_RNA_averagine = false) is the default and is
  // already computed in algo_default.
  auto& protein_avg = algo_default.getAveragine();
  TEST_NOT_EQUAL(protein_avg.getMaxIsotopeIndex(), 0)
  TEST_FALSE(protein_avg.get(1000.0).getContainer().empty())

  // RNA averagine is a distinct configuration and needs its own run.
  FLASHDeconvAlgorithm algo_rna;
  Param rna_params;
  rna_params.setValue("use_RNA_averagine", "true");
  algo_rna.setParameters(rna_params);
  std::vector<DeconvolvedSpectrum> rna_spectra;
  std::vector<FLASHHelperClasses::MassFeature> rna_features;
  algo_rna.run(input, rna_spectra, rna_features);

  auto& rna_avg = algo_rna.getAveragine();
  TEST_NOT_EQUAL(rna_avg.getMaxIsotopeIndex(), 0)
  TEST_FALSE(rna_avg.get(1000.0).getContainer().empty())
END_SECTION

START_SECTION(Averagine model - mass range validation)
  auto& avg = algo_default.getAveragine();

  // Test averagine at various mass ranges
  TEST_FALSE(avg.get(500.0).getContainer().empty())
  TEST_FALSE(avg.get(1000.0).getContainer().empty())
  TEST_FALSE(avg.get(5000.0).getContainer().empty())
  TEST_FALSE(avg.get(10000.0).getContainer().empty())
END_SECTION

START_SECTION(Averagine model - consistency check)
  auto& avg = algo_default.getAveragine();

  // Verify max isotope index is reasonable
  int max_iso_idx = avg.getMaxIsotopeIndex();
  TEST_NOT_EQUAL(max_iso_idx, 0)
  TEST_EQUAL(max_iso_idx > 0, true)
  TEST_EQUAL(max_iso_idx < 1000, true) // Reasonable upper bound
END_SECTION

/////////////////////////////////////////////////////////////
// Phase 1: MS Level Processing Tests
/////////////////////////////////////////////////////////////

START_SECTION(MS level processing - scan number retrieval)
  // Test getScanNumber with different indices - verify it returns positive values
  TEST_EQUAL(FLASHDeconvAlgorithm::getScanNumber(input, 0) > 0, true)

  // Test with valid index range
  if (input.size() > 1)
  {
    int scan_num = FLASHDeconvAlgorithm::getScanNumber(input, 1);
    TEST_NOT_EQUAL(scan_num, 0) // Should have a valid scan number
  }
END_SECTION

/////////////////////////////////////////////////////////////
// Phase 1: Spectrum Merging Tests
/////////////////////////////////////////////////////////////

START_SECTION(Spectrum merging - merging method parameter validation)
  FLASHDeconvAlgorithm algo;
  Param merge_params;

  // Test merging_method = 0 (no merging)
  merge_params.setValue("merging_method", 0);
  algo.setParameters(merge_params);
  TEST_EQUAL(static_cast<int>(algo.getParameters().getValue("merging_method")), 0)

  // Test merging_method = 1 (Gaussian)
  merge_params.setValue("merging_method", 1);
  algo.setParameters(merge_params);
  TEST_EQUAL(static_cast<int>(algo.getParameters().getValue("merging_method")), 1)

  // Test merging_method = 2 (block)
  merge_params.setValue("merging_method", 2);
  algo.setParameters(merge_params);
  TEST_EQUAL(static_cast<int>(algo.getParameters().getValue("merging_method")), 2)
END_SECTION

START_SECTION(Spectrum merging - effect on output with no merging)
  // merging_method = 0 (no merging) is the default and is already computed in
  // algo_default. With no merging the output spectrum count equals the input.
  TEST_EQUAL(default_spectra.size(), input.size())
END_SECTION

START_SECTION(Spectrum merging - Gaussian merging mode)
  FLASHDeconvAlgorithm algo_gaussian;
  Param gaussian_params;
  gaussian_params.setValue("merging_method", 1);
  algo_gaussian.setParameters(gaussian_params);

  std::vector<DeconvolvedSpectrum> gaussian_spectra;
  std::vector<FLASHHelperClasses::MassFeature> gaussian_features;

  // Merging mutates its input; preserve the shared fixture for subsequent modes.
  PeakMap gaussian_input = input;
  algo_gaussian.run(gaussian_input, gaussian_spectra, gaussian_features);
  TEST_FALSE(gaussian_spectra.empty())
  TEST_TRUE(gaussian_input.getSpectra() != input.getSpectra())
END_SECTION

START_SECTION(Spectrum merging - block mode combines MS1 scans)
  PeakMap block_input;
  for (const auto& spectrum : input)
  {
    if (spectrum.getMSLevel() == 1 && !spectrum.empty())
    {
      block_input.addSpectrum(spectrum);
      block_input.addSpectrum(spectrum);
      block_input.getSpectra().back().setRT(spectrum.getRT() + 1.0);
      block_input.getSpectra().back().setNativeID("scan=999999");
      break;
    }
  }
  TEST_EQUAL(block_input.size(), 2)
  FLASHDeconvAlgorithm block_algorithm;
  Param block_parameters;
  block_parameters.setValue("merging_method", 2);
  block_algorithm.setParameters(block_parameters);
  std::vector<DeconvolvedSpectrum> block_spectra;
  std::vector<FLASHHelperClasses::MassFeature> block_features;
  block_algorithm.run(block_input, block_spectra, block_features);
  TEST_EQUAL(block_input.size(), 1)
  TEST_EQUAL(block_spectra.size(), 1)
END_SECTION

/////////////////////////////////////////////////////////////
// Phase 1: FDR and Decoy Generation Tests
/////////////////////////////////////////////////////////////

START_SECTION(FDR - report_FDR parameter toggling)
  // FDR reporting disabled (default): the decoy averagine is still initialized.
  auto& decoy_avg_no_fdr = algo_default.getDecoyAveragine();
  TEST_NOT_EQUAL(decoy_avg_no_fdr.getMaxIsotopeIndex(), 0)

  // FDR reporting enabled: decoy averagine is properly initialized for FDR.
  auto& decoy_avg_fdr = algo_fdr.getDecoyAveragine();
  TEST_NOT_EQUAL(decoy_avg_fdr.getMaxIsotopeIndex(), 0)
  TEST_FALSE(decoy_avg_fdr.get(500.0).getContainer().empty())
  TEST_EQUAL(&algo_default.getAveragine(), &decoy_avg_no_fdr)
  TEST_NOT_EQUAL(&algo_fdr.getAveragine(), &decoy_avg_fdr)
  Size default_decoys = 0;
  Size reported_decoys = 0;
  for (const auto& spectrum : default_spectra)
  {
    for (const auto& peak_group : spectrum)
    {
      default_decoys += peak_group.getTargetDecoyType() != PeakGroup::target;
    }
  }
  for (const auto& spectrum : fdr_spectra)
  {
    for (const auto& peak_group : spectrum)
    {
      reported_decoys += peak_group.getTargetDecoyType() != PeakGroup::target;
    }
  }
  TEST_EQUAL(default_decoys, 0)
  TEST_TRUE(reported_decoys > 0)
END_SECTION

START_SECTION(FDR - noise decoy weight validation)
  // Get noise decoy weight from the FDR-reporting run
  double noise_weight = algo_fdr.getNoiseDecoyWeight();

  // Verify noise weight is reasonable (should be positive and bounded)
  TEST_EQUAL(noise_weight > 0, true)
  TEST_EQUAL(noise_weight <= 1.0, true) // Calculated weight should be at most 1.0
END_SECTION

START_SECTION(FDR - decoy averagine properties)
  auto& decoy_avg = algo_fdr.getDecoyAveragine();

  // Verify decoy averagine has similar properties to regular averagine
  TEST_NOT_EQUAL(decoy_avg.getMaxIsotopeIndex(), 0)

  // Test at different masses
  TEST_FALSE(decoy_avg.get(500.0).getContainer().empty())
  TEST_FALSE(decoy_avg.get(1000.0).getContainer().empty())
  TEST_FALSE(decoy_avg.get(5000.0).getContainer().empty())
END_SECTION

START_SECTION(FDR - decoy generation consistency)
  // A second, independent FDR run must yield the same decoy averagine as the
  // shared algo_fdr run (regression guard for deterministic decoy generation).
  FLASHDeconvAlgorithm algo2;
  Param fdr_params;
  fdr_params.setValue("report_FDR", "true");
  algo2.setParameters(fdr_params);

  std::vector<DeconvolvedSpectrum> spectra2;
  std::vector<FLASHHelperClasses::MassFeature> features2;
  algo2.run(input, spectra2, features2);

  // Both should generate decoy averagines with same max isotope index
  TEST_EQUAL(algo_fdr.getDecoyAveragine().getMaxIsotopeIndex(),
             algo2.getDecoyAveragine().getMaxIsotopeIndex())
END_SECTION

/////////////////////////////////////////////////////////////
// Phase 1: Output Validation Tests
/////////////////////////////////////////////////////////////

START_SECTION(Output validation - deconvolved spectrum structure)
  // Verify output has expected size
  TEST_EQUAL(default_spectra.size(), input.size())

  // Check that each deconvolved spectrum has valid properties
  for (const auto& spec : default_spectra)
  {
    // Verify spectrum has been processed (has some metadata)
    TEST_TRUE(spec.getScanNumber() >= 0)
  }
END_SECTION

START_SECTION(Output validation - mass features structure)
  // Verify features were found on this input
  TEST_FALSE(default_features.empty())

  // Validate basic properties of each feature
  for (const auto& feature : default_features)
  {
    // Verify feature has valid mass range
    TEST_TRUE(feature.avg_mass >= 0)
  }
END_SECTION

START_SECTION(Output validation - PeakGroup properties in deconvolved spectra)
  // Check deconvolved spectra for peak groups
  bool found_peak_groups = false;
  for (const auto& spec : default_spectra)
  {
    if (spec.size() > 0)
    {
      found_peak_groups = true;

      // Verify peak groups have valid properties
      for (Size i = 0; i < spec.size(); ++i)
      {
        const auto& pg = spec[i];
        // Peak groups should have positive mass
        TEST_TRUE(pg.getMonoMass() >= 0)
      }
    }
  }

  // At least some spectra should have peak groups
  TEST_TRUE(found_peak_groups)
END_SECTION

START_SECTION(Output validation - consistent output vector sizes)
  // Re-running with the same (default) parameters must yield the same number
  // of spectra as the shared algo_default result (reproducibility guard).
  FLASHDeconvAlgorithm algo;
  std::vector<DeconvolvedSpectrum> output_spectra;
  std::vector<FLASHHelperClasses::MassFeature> output_features;
  algo.run(input, output_spectra, output_features);

  // Should produce same number of spectra on repeated runs
  TEST_EQUAL(default_spectra.size(), output_spectra.size())
END_SECTION

START_SECTION(Output validation - output spectrum properties)
  // Verify each output spectrum has consistent properties with input
  for (size_t i = 0; i < default_spectra.size() && i < input.size(); ++i)
  {
    // Scan numbers should match
    TEST_EQUAL(default_spectra[i].getScanNumber(),
               FLASHDeconvAlgorithm::getScanNumber(input, i))
  }
END_SECTION

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST
