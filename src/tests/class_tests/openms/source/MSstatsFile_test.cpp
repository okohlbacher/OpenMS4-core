// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Lukas Heumos $
// $Authors: Lukas Heumos $
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/FORMAT/CsvFile.h>
#include <OpenMS/FORMAT/MSstatsFile.h>
#include <OpenMS/test_config.h>

using namespace OpenMS;

START_TEST(MSstatsFile, "$Id$")

START_SECTION(void OpenMS::MSstatsFile::storeLFQ(const std::string &filename, ConsensusMap &consensus_map,
                                                 const OpenMS::ExperimentalDesign& design, const StringList& reannotate_filenames,
                                                 const bool is_isotope_label_type, const std::string& bioreplicate, const std::string& condition,
                                                 const std::string& retention_time_summarization_method,
                                                 const bool remove_shared_peptides))
{
  ExperimentalDesign::MSFileSectionEntry run;
  run.path = "run.mzML";
  run.sample_name = "sample";
  ExperimentalDesign::SampleSection samples({{"sample", "case", "1"}}, {{"sample", 0}}, {{"Sample", 0}, {"Condition", 1}, {"BioReplicate", 2}});
  ExperimentalDesign design({run}, samples);

  ConsensusMap map;
  map.getColumnHeaders()[0].filename = run.path;
  map.getProteinIdentifications().emplace_back();
  PeptideHit hit;
  hit.setSequence(AASequence::fromString("PEPTIDE"));
  hit.setCharge(2);
  PeptideEvidence evidence;
  evidence.setProteinAccession("protein");
  hit.addPeptideEvidence(evidence);
  PeptideIdentification identification;
  identification.insertHit(hit);
  for (Size i = 0; i < 3; ++i)
  {
    FeatureHandle handle;
    handle.setMapIndex(0);
    handle.setUniqueId(i + 1);
    handle.setRT(10.0 * (i + 1));
    handle.setIntensity(i < 2 ? 10.0 : 20.0);
    ConsensusFeature feature;
    feature.insert(handle);
    feature.getPeptideIdentifications().push_back(identification);
    map.push_back(feature);
  }

  MSstatsFile file;
  // Equal intensities at distinct RTs are separate measurements: 10 + 10 + 20.
  for (const std::string method : {"sum", "mean"})
  {
    std::string output;
    NEW_TMP_FILE(output)
    file.storeLFQ(output, map, design, {run.path}, false, "BioReplicate", "Condition", method, false);
    CsvFile csv(output);
    TEST_EQUAL(csv.rowCount(), 2)
    StringList row;
    csv.getRow(1, row);
    TEST_EQUAL(row.size(), 11)
    ABORT_IF(row.size() != 11)
    TEST_REAL_SIMILAR(StringUtils::toDouble(row[9]), method == "sum" ? 40.0 : 40.0 / 3.0)
  }

  std::string invalid_output;
  NEW_TMP_FILE(invalid_output)
  TEST_EXCEPTION(Exception::IllegalArgument,
                 file.storeLFQ(invalid_output, map, design, {run.path}, false, "BioReplicate", "Condition", "unknown", false))
  // The filename is present in the design, but this label is not.
  map.getColumnHeaders()[0].setMetaValue("channel_id", 2);
  TEST_EXCEPTION(Exception::MissingInformation,
                 file.storeLFQ(invalid_output, map, design, {run.path}, false, "BioReplicate", "Condition", "sum", false))
}
END_SECTION

START_SECTION(void OpenMS::MSstatsFile::storeISO(const std::string &filename, ConsensusMap &consensus_map,
                                                 const OpenMS::ExperimentalDesign& design, const StringList& reannotate_filenames,
                                                 const std::string& bioreplicate, const std::string& condition,
                                                 const std::string& mixture, const std::string& retention_time_summarization_method,
                                                 const bool remove_shared_peptides))
{
  // tested via MSstatsConverter tool
}
END_SECTION

END_TEST
