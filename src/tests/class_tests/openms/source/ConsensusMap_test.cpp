// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// 
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: $
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/test_config.h>
#include <OpenMS/KERNEL/StandardTypes.h>
#include <OpenMS/KERNEL/ConsensusFeature.h>


///////////////////////////
#include <OpenMS/KERNEL/ConsensusMap.h>
#include <OpenMS/KERNEL/FeatureMap.h>
///////////////////////////

#include <OpenMS/METADATA/ProteinIdentification.h>
#include <OpenMS/METADATA/PeptideIdentification.h>
#include <OpenMS/METADATA/DataProcessing.h>
#include <OpenMS/CONCEPT/LogStream.h>

#include <sstream>

using namespace OpenMS;
using namespace std;

START_TEST(ConsensusMap, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

ConsensusMap* ptr = nullptr;
ConsensusMap* nullPointer = nullptr;
START_SECTION((ConsensusMap()))
	ptr = new ConsensusMap();
	TEST_NOT_EQUAL(ptr, nullPointer)
	TEST_TRUE(ptr->isMetaEmpty())
END_SECTION

START_SECTION((~ConsensusMap()))
	delete ptr;
END_SECTION

START_SECTION((const std::vector<ProteinIdentification>& getProteinIdentifications() const))
	FeatureMap tmp;
	TEST_EQUAL(tmp.getProteinIdentifications().size(),0)
END_SECTION

START_SECTION((std::vector<ProteinIdentification>& getProteinIdentifications()))
	FeatureMap tmp;
	tmp.getProteinIdentifications().resize(1);
	TEST_EQUAL(tmp.getProteinIdentifications().size(),1)
END_SECTION

START_SECTION((void setProteinIdentifications(const std::vector<ProteinIdentification>& protein_identifications)))
	FeatureMap tmp;
	tmp.setProteinIdentifications(std::vector<ProteinIdentification>(2));
	TEST_EQUAL(tmp.getProteinIdentifications().size(),2)
END_SECTION

START_SECTION((const PeptideIdentificationList& getUnassignedPeptideIdentifications() const))
	FeatureMap tmp;
	TEST_EQUAL(tmp.getUnassignedPeptideIdentifications().size(),0)
END_SECTION

START_SECTION((PeptideIdentificationList& getUnassignedPeptideIdentifications()))
	FeatureMap tmp;
	tmp.getUnassignedPeptideIdentifications().resize(1);
	TEST_EQUAL(tmp.getUnassignedPeptideIdentifications().size(),1)
END_SECTION

START_SECTION((void setUnassignedPeptideIdentifications(const PeptideIdentificationList& unassigned_peptide_identifications)))
	FeatureMap tmp;
	tmp.setUnassignedPeptideIdentifications(PeptideIdentificationList(2));
	TEST_EQUAL(tmp.getUnassignedPeptideIdentifications().size(),2)
END_SECTION

START_SECTION((const std::vector<DataProcessing>& getDataProcessing() const))
  ConsensusMap tmp;
  TEST_EQUAL(tmp.getDataProcessing().size(),0);
END_SECTION

START_SECTION((std::vector<DataProcessing>& getDataProcessing()))
  ConsensusMap tmp;
  tmp.getDataProcessing().resize(1);
  TEST_EQUAL(tmp.getDataProcessing().size(),1);
END_SECTION

START_SECTION((void setDataProcessing(const std::vector< DataProcessing > &processing_method)))
  ConsensusMap tmp;
  std::vector<DataProcessing> dummy;
  dummy.resize(1);
  tmp.setDataProcessing(dummy);
  TEST_EQUAL(tmp.getDataProcessing().size(),1);
END_SECTION

Feature feature1;
feature1.getPosition()[0] = 2.0;
feature1.getPosition()[1] = 3.0;
feature1.setIntensity(1.0f);

Feature feature2;
feature2.getPosition()[0] = 0.0;
feature2.getPosition()[1] = 2.5;
feature2.setIntensity(0.5f);

Feature feature3;
feature3.getPosition()[0] = 10.5;
feature3.getPosition()[1] = 0.0;
feature3.setIntensity(0.01f);

Feature feature4;
feature4.getPosition()[0] = 5.25;
feature4.getPosition()[1] = 1.5;
feature4.setIntensity(0.5f);

START_SECTION((void updateRanges()))
  ConsensusMap map;
  feature1.setUniqueId(1);
	ConsensusFeature f;
	f.setIntensity(1.0f);
	f.setRT(2.0);
	f.setMZ(3.0);
	f.insert(1,feature1);
	map.push_back(f);

  map.updateRanges();
  TEST_REAL_SIMILAR(map.getMinIntensity(), 1.0)
  TEST_REAL_SIMILAR(map.getMaxIntensity(), 1.0)
  TEST_REAL_SIMILAR(map.getMaxRT(),2.0)
  TEST_REAL_SIMILAR(map.getMaxMZ(),3.0)
  TEST_REAL_SIMILAR(map.getMinRT(),2.0)
  TEST_REAL_SIMILAR(map.getMinMZ(),3.0)

  // second time to check the initialization
  map.updateRanges();
  TEST_REAL_SIMILAR(map.getMinIntensity(), 1.0)
  TEST_REAL_SIMILAR(map.getMaxIntensity(), 1.0)
  TEST_REAL_SIMILAR(map.getMaxRT(), 2.0)
  TEST_REAL_SIMILAR(map.getMaxMZ(), 3.0)
  TEST_REAL_SIMILAR(map.getMinRT(), 2.0)
  TEST_REAL_SIMILAR(map.getMinMZ(), 3.0)

  // two points
  feature2.setUniqueId(2);
	f.insert(1,feature2);
	map.push_back(f);
	map.updateRanges();
  TEST_REAL_SIMILAR(map.getMinIntensity(), 0.5)
  TEST_REAL_SIMILAR(map.getMaxIntensity(), 1.0)
  TEST_REAL_SIMILAR(map.getMaxRT(), 2.0)
  TEST_REAL_SIMILAR(map.getMaxMZ(), 3.0)
  TEST_REAL_SIMILAR(map.getMinRT(), 0.0)
  TEST_REAL_SIMILAR(map.getMinMZ(), 2.5)

	// four points
  feature3.setUniqueId(3);
	f.insert(1,feature3);
  feature4.setUniqueId(4);
	f.insert(1,feature4);
	map.push_back(f);
	map.updateRanges();
  TEST_REAL_SIMILAR(map.getMinIntensity(), 0.01)
  TEST_REAL_SIMILAR(map.getMaxIntensity(), 1.0)
  TEST_REAL_SIMILAR(map.getMaxRT(), 10.5)
  TEST_REAL_SIMILAR(map.getMaxMZ(), 3.0)
  TEST_REAL_SIMILAR(map.getMinRT(), 0.0)
  TEST_REAL_SIMILAR(map.getMinMZ(), 0.0)
END_SECTION

START_SECTION((ConsensusMap& appendRows(const ConsensusMap &rhs)))
{
  ConsensusMap m1, m2, m3;
  // adding empty maps has no effect:
  m1.appendRows(m2) ;
  TEST_EQUAL(m1, m3);

  // with content:
  ConsensusFeature f1;
  f1.setMZ(100.12);
  m1.push_back(f1);
  m3 = m1;
  m1.appendRows(m2);
  TEST_EQUAL(m1, m3);

  // test basic classes
  m1.setIdentifier ("123");
  m1.getDataProcessing().resize(1);
  m1.getProteinIdentifications().resize(1);
  m1.getUnassignedPeptideIdentifications().resize(1);
  m1.ensureUniqueId();
  m1.getColumnHeaders()[0].filename = "m1";

  m2.setIdentifier ("321");
  m2.getDataProcessing().resize(2);
  m2.getProteinIdentifications().resize(2);
  m2.getUnassignedPeptideIdentifications().resize(2);
  m2.push_back(ConsensusFeature());
  m2.push_back(ConsensusFeature());
  m2.getColumnHeaders()[1].filename = "m2";

  m1.appendRows(m2);
  TEST_EQUAL(m1.getIdentifier(), "");
  TEST_EQUAL(UniqueIdInterface::isValid(m1.getUniqueId()), false);
  TEST_EQUAL(m1.getDataProcessing().size(), 3);
  TEST_EQUAL(m1.getProteinIdentifications().size(),3);
  TEST_EQUAL(m1.getUnassignedPeptideIdentifications().size(),3);
  TEST_EQUAL(m1.size(),3);
  TEST_EQUAL(m1.getColumnHeaders().size(), 2);
}
END_SECTION

START_SECTION((ConsensusMap& appendColumns(const ConsensusMap &rhs)))
{
  ConsensusMap m1, m2;

  // Test1: adding empty map has no effect:
  m1.appendColumns(ConsensusMap()) ;
  TEST_EQUAL(m1, ConsensusMap());

  // one consensus feature with one element referencing the first map
  Feature f1;
  f1.setRT(1);
  f1.setMZ(1);
  f1.setIntensity(1);
  f1.setUniqueId(1);
  ConsensusFeature cf1;
  cf1.insert(0, f1, 0); // map = 0, feature 1, element = 0
  cf1.setMZ(100.12);
  m1.push_back(cf1);
  // m1 now contains one consensus feature with one element associated with map 0

  // Test2: adding empty map to map with content
  ConsensusMap old_m1 = m1;
  m1.appendColumns(ConsensusMap());
  TEST_EQUAL(m1, old_m1);

  m1.setIdentifier("123");
  m1.getDataProcessing().resize(1);
  m1.getProteinIdentifications().resize(1);
  m1.getUnassignedPeptideIdentifications().resize(1);
  m1.ensureUniqueId();
  m1.getColumnHeaders()[0].filename = "m1";

  m2.setIdentifier("321");
  m2.getDataProcessing().resize(2);
  m2.getProteinIdentifications().resize(2);
  m2.getUnassignedPeptideIdentifications().resize(2);
  m2.getColumnHeaders()[0].filename = "m2_1";
  m2.getColumnHeaders()[1].filename = "m2_2";

  // one consensus feature with two elements referencing the first and second map
  Feature f2, f3;
  f2.setRT(2);
  f2.setMZ(2);
  f2.setIntensity(2);
  f2.setUniqueId(2);
  f3.setRT(3);
  f3.setMZ(3);
  f3.setIntensity(3);
  f3.setUniqueId(3);
  ConsensusFeature cf2;
  cf2.insert(0, f2, 0); // first map, feature2, first element
  cf2.insert(1, f3, 1); // second map, feature3, second element
  m2.push_back(cf2);

  // append columns of m2 to m1
  m1.appendColumns(m2); // now contains 1 (m1) + 2 columns (m2)

  TEST_EQUAL(m1.getIdentifier(), "");
  TEST_EQUAL(UniqueIdInterface::isValid(m1.getUniqueId()), false);
  TEST_EQUAL(m1.getDataProcessing().size(), 3);
  TEST_EQUAL(m1.getProteinIdentifications().size(), 3);
  TEST_EQUAL(m1.getUnassignedPeptideIdentifications().size(), 3);
  TEST_EQUAL(m1.size(), 2);
  TEST_EQUAL(m1.getColumnHeaders().size(), 3);
  TEST_EQUAL(m1.getColumnHeaders()[0].filename, "m1");
  TEST_EQUAL(m1.getColumnHeaders()[1].filename, "m2_1");
  TEST_EQUAL(m1.getColumnHeaders()[2].filename, "m2_2");

  auto cfh1 = m1[0].getFeatures();
  TEST_EQUAL(cfh1.begin()->getIntensity(), 1);
  TEST_EQUAL(cfh1.begin()->getUniqueId(), 0);
  TEST_EQUAL(cfh1.begin()->getMapIndex(), 0);

  // test the second consensus feature with two elements
  // they should now reference to the second and third map (map index 1 and 2)
  auto cfh2 = m1[1].getFeatures();
  Size element(0); 
  for (auto & h : cfh2)
  {
    if (element == 0)
    {
      TEST_EQUAL(h.getIntensity(), 2);
      TEST_EQUAL(h.getUniqueId(), 0); // references the unique id of the original feature
      TEST_EQUAL(h.getMapIndex(), 1);
    }
    else
    {
      TEST_EQUAL(h.getIntensity(), 3);
      TEST_EQUAL(h.getUniqueId(), 1); // references the unique id of the original feature
      TEST_EQUAL(h.getMapIndex(), 2);
    }
    ++element;
  }
}
END_SECTION

const ConsensusMap map_const_1 = []() {
  ConsensusMap map1;
  map1.resize(3);
  map1.setMetaValue("meta",std::string("value"));
  map1.setIdentifier("lsid");
  map1.getColumnHeaders()[0].filename = "blub";
  map1.getColumnHeaders()[0].size = 47;
  map1.getColumnHeaders()[0].label = "label";
  map1.getColumnHeaders()[0].setMetaValue("meta",std::string("meta"));
  map1.getDataProcessing().resize(1);
  map1.setExperimentType("labeled_MS2");
  map1.getProteinIdentifications().resize(1);
  map1.getUnassignedPeptideIdentifications().resize(1);
  return map1;
}();



START_SECTION((ConsensusMap& operator = (const ConsensusMap& source)))
  //assignment
  ConsensusMap map2;
  map2 = map_const_1;
  TEST_EQUAL(map2.size(), 3)
  TEST_EQUAL(map2.getIdentifier(), "lsid")
  TEST_EQUAL(map2.getMetaValue("meta").toString(),"value")

  TEST_EQUAL(map2.getColumnHeaders()[0].filename == "blub", true)
  TEST_EQUAL(map2.getColumnHeaders()[0].label == "label", true)
  TEST_EQUAL(map2.getColumnHeaders()[0].size == 47, true)
	TEST_EQUAL(map2.getColumnHeaders()[0].getMetaValue("meta") == "meta", true)
  TEST_EQUAL(map2.getExperimentType(), "labeled_MS2")

  TEST_EQUAL(map2.getDataProcessing().size(),1)
	TEST_EQUAL(map2.getProteinIdentifications().size(),1)
	TEST_EQUAL(map2.getUnassignedPeptideIdentifications().size(),1)

  //assignment of empty object
  ConsensusMap map2_empty;
  map2 = map2_empty;
  TEST_EQUAL(map2.getIdentifier(),"")
  TEST_EQUAL(map2.getColumnHeaders().size(),0)
  TEST_EQUAL(map2.getExperimentType(),"label-free") // default
  TEST_EQUAL(map2.getDataProcessing().size(),0)
	TEST_EQUAL(map2.getProteinIdentifications().size(),0)
	TEST_EQUAL(map2.getUnassignedPeptideIdentifications().size(),0)
END_SECTION

START_SECTION((ConsensusMap(const ConsensusMap& source)))
  ConsensusMap map2(map_const_1);
  TEST_EQUAL(map2.getIdentifier(),"lsid")
  TEST_EQUAL(map2.getMetaValue("meta").toString(),"value")
  TEST_EQUAL(map2.getColumnHeaders()[0].filename == "blub", true)
  TEST_EQUAL(map2.getColumnHeaders()[0].label == "label", true)
  TEST_EQUAL(map2.getColumnHeaders()[0].size == 47, true)
	TEST_EQUAL(map2.getColumnHeaders()[0].getMetaValue("meta") == "meta", true)
  TEST_EQUAL(map2.getExperimentType(),"labeled_MS2")
  TEST_EQUAL(map2.getDataProcessing().size(),1)
	TEST_EQUAL(map2.getProteinIdentifications().size(),1);
	TEST_EQUAL(map2.getUnassignedPeptideIdentifications().size(),1);
END_SECTION



START_SECTION((ConsensusMap& operator = (ConsensusMap&& source)))
  static_assert(std::is_move_assignable_v<ConsensusMap>);
  //assignment
  ConsensusMap map2;
  map2 = ConsensusMap(map_const_1); // move
  TEST_EQUAL(map2.size(), 3)
  TEST_EQUAL(map2.getIdentifier(), "lsid")
  TEST_EQUAL(map2.getMetaValue("meta").toString(),"value")

  TEST_EQUAL(map2.getColumnHeaders()[0].filename == "blub", true)
  TEST_EQUAL(map2.getColumnHeaders()[0].label == "label", true)
  TEST_EQUAL(map2.getColumnHeaders()[0].size == 47, true)
	TEST_EQUAL(map2.getColumnHeaders()[0].getMetaValue("meta") == "meta", true)
  TEST_EQUAL(map2.getExperimentType(), "labeled_MS2")

  TEST_EQUAL(map2.getDataProcessing().size(),1)
	TEST_EQUAL(map2.getProteinIdentifications().size(),1)
	TEST_EQUAL(map2.getUnassignedPeptideIdentifications().size(),1)

  //assignment of empty object
  ConsensusMap map2_empty;
  map2 = std::move(map2_empty); // move
  TEST_EQUAL(map2.getIdentifier(), "")
  TEST_EQUAL(map2.getColumnHeaders().size(), 0)
  TEST_EQUAL(map2.getExperimentType(), "label-free") // default
  TEST_EQUAL(map2.getDataProcessing().size(), 0)
  TEST_EQUAL(map2.getProteinIdentifications().size(), 0)
  TEST_EQUAL(map2.getUnassignedPeptideIdentifications().size(), 0)
  END_SECTION

START_SECTION((ConsensusMap(ConsensusMap && source)))
  static_assert(std::is_move_constructible_v<ConsensusMap>);
  ConsensusMap mp_source(map_const_1);
  ConsensusMap map2(std::move(mp_source));
  TEST_EQUAL(map2.getIdentifier(),"lsid")
  TEST_EQUAL(map2.getMetaValue("meta").toString(),"value")
  TEST_EQUAL(map2.getColumnHeaders()[0].filename == "blub", true)
  TEST_EQUAL(map2.getColumnHeaders()[0].label == "label", true)
  TEST_EQUAL(map2.getColumnHeaders()[0].size == 47, true)
	TEST_EQUAL(map2.getColumnHeaders()[0].getMetaValue("meta") == "meta", true)
  TEST_EQUAL(map2.getExperimentType(),"labeled_MS2")
  TEST_EQUAL(map2.getDataProcessing().size(),1)
	TEST_EQUAL(map2.getProteinIdentifications().size(),1);
	TEST_EQUAL(map2.getUnassignedPeptideIdentifications().size(),1);
END_SECTION


START_SECTION((ConsensusMap(size_type n)))
  ConsensusMap cons_map(5);

  TEST_EQUAL(cons_map.size(),5)
END_SECTION

/////

START_SECTION(([ConsensusMap::ColumnHeader] ColumnHeader()))
  ConsensusMap::ColumnHeader* fd_ptr = new ConsensusMap::ColumnHeader();;
  ConsensusMap::ColumnHeader* fd_nullPointer = nullptr;
  TEST_NOT_EQUAL(fd_ptr, fd_nullPointer)
  delete fd_ptr;
END_SECTION

START_SECTION((const ColumnHeaders& getColumnHeaders() const))
  ConsensusMap cons_map;

  TEST_EQUAL(cons_map.getColumnHeaders().size(),0)
END_SECTION

START_SECTION((ColumnHeaders& getColumnHeaders()))
  ConsensusMap cons_map;

  cons_map.getColumnHeaders()[0].filename = "blub";
  TEST_EQUAL(cons_map.getColumnHeaders()[0].filename == "blub", true)
END_SECTION

START_SECTION((const std::string& getExperimentType() const))
  ConsensusMap cons_map;
	TEST_EQUAL(cons_map.getExperimentType() == "label-free", true)
END_SECTION

START_SECTION((void setExperimentType(const std::string& experiment_type)))
  ConsensusMap cons_map;
	cons_map.setExperimentType("labeled_MS2");
  TEST_EQUAL(cons_map.getExperimentType(),"labeled_MS2")
END_SECTION

START_SECTION((void swap(ConsensusMap& from)))
	ConsensusMap map1, map2;
	ConsensusFeature f;
	f.insert(1,Feature());
	map1.push_back(f);
  map1.getColumnHeaders()[1].filename = "bla";
	map1.getColumnHeaders()[1].size = 5;
	map1.setIdentifier("LSID");
	map1.setExperimentType("labeled_MS2");
	map1.getDataProcessing().resize(1);
	map1.getProteinIdentifications().resize(1);
	map1.getUnassignedPeptideIdentifications().resize(1);

	map1.swap(map2);

	TEST_EQUAL(map1.size(),0)
	TEST_EQUAL(map1.getColumnHeaders().size(),0)
	TEST_EQUAL(map1.getIdentifier(),"")
  TEST_EQUAL(map1.getDataProcessing().size(),0)
	TEST_EQUAL(map1.getProteinIdentifications().size(),0);
	TEST_EQUAL(map1.getUnassignedPeptideIdentifications().size(),0);

	TEST_EQUAL(map2.size(),1)
	TEST_EQUAL(map2.getColumnHeaders().size(),1)
	TEST_EQUAL(map2.getIdentifier(),"LSID")
  TEST_EQUAL(map2.getExperimentType(),"labeled_MS2")
  TEST_EQUAL(map2.getDataProcessing().size(),1)
	TEST_EQUAL(map2.getProteinIdentifications().size(),1);
	TEST_EQUAL(map2.getUnassignedPeptideIdentifications().size(),1);
END_SECTION

START_SECTION((bool operator == (const ConsensusMap& rhs) const))
	ConsensusMap empty,edit;

	TEST_TRUE(empty == edit);

	edit.setIdentifier("lsid");;
	TEST_EQUAL(empty==edit, false);

	edit = empty;
	edit.push_back(ConsensusFeature(feature1));
	TEST_EQUAL(empty==edit, false);

	edit = empty;
	edit.getDataProcessing().resize(1);
	TEST_EQUAL(empty==edit, false);

	edit = empty;
	edit.setMetaValue("bla", 4.1);
	TEST_EQUAL(empty==edit, false);

	edit = empty;
	edit.getColumnHeaders()[0].filename = "bla";
	TEST_EQUAL(empty==edit, false);

	edit = empty;
	edit.setExperimentType("labeled_MS2");
	TEST_EQUAL(empty==edit, false);

	edit = empty;
	edit.getProteinIdentifications().resize(10);
	TEST_EQUAL(empty==edit, false);

	edit = empty;
	edit.getUnassignedPeptideIdentifications().resize(10);
	TEST_EQUAL(empty==edit, false);

	edit = empty;
	edit.setExperimentType("labeled_MS2");
	TEST_EQUAL(empty==edit, false);

	edit = empty;
	edit.push_back(ConsensusFeature(feature1));
	edit.push_back(ConsensusFeature(feature2));
	edit.updateRanges();
	edit.clear(false);
	TEST_EQUAL(empty==edit, false);
END_SECTION

START_SECTION((bool operator != (const ConsensusMap& rhs) const))
	ConsensusMap empty,edit;

	TEST_EQUAL(empty!=edit, false);

	edit.setIdentifier("lsid");;
	TEST_FALSE(empty == edit);

	edit = empty;
	edit.push_back(ConsensusFeature(feature1));
	TEST_FALSE(empty == edit);

	edit = empty;
	edit.getDataProcessing().resize(1);
	TEST_FALSE(empty == edit);

	edit = empty;
	edit.setMetaValue("bla", 4.1);
	TEST_FALSE(empty == edit);

	edit = empty;
	edit.getColumnHeaders()[0].filename = "bla";
	TEST_FALSE(empty == edit)

	edit = empty;
	edit.setExperimentType("labeled_MS2");
	TEST_FALSE(empty == edit);

	edit = empty;
	edit.getProteinIdentifications().resize(10);
	TEST_FALSE(empty == edit);

	edit = empty;
	edit.getUnassignedPeptideIdentifications().resize(10);
	TEST_FALSE(empty == edit);

	edit = empty;
	edit.push_back(ConsensusFeature(feature1));
	edit.push_back(ConsensusFeature(feature2));
	edit.updateRanges();
	edit.clear(false);
	TEST_FALSE(empty == edit);
END_SECTION


START_SECTION((void sortByIntensity(bool reverse=false)))
{
  NOT_TESTABLE; // tested within TOPP TextExporter
}
END_SECTION

START_SECTION((void sortByRT()))
{
  NOT_TESTABLE; // tested within TOPP TextExporter
}
END_SECTION

START_SECTION((void sortByMZ()))
{
  NOT_TESTABLE; // tested within TOPP TextExporter
}
END_SECTION

START_SECTION((void sortByPosition()))
{
  NOT_TESTABLE; // tested within TOPP TextExporter
}
END_SECTION

START_SECTION((void sortByQuality(bool reverse=false)))
{
  NOT_TESTABLE; // tested within TOPP TextExporter
}
END_SECTION

START_SECTION((void sortBySize()))
{
  NOT_TESTABLE; // tested within TOPP TextExporter
}
END_SECTION

START_SECTION((void sortByMaps()))
{
  NOT_TESTABLE; // tested within TOPP TextExporter
}
END_SECTION

START_SECTION((void sortPeptideIdentificationsByMapIndex()))
{
  NOT_TESTABLE; // tested within TOPP IDMapper
}
END_SECTION

START_SECTION((void clear(bool clear_meta_data = true)))
{
  ConsensusMap map1;
	ConsensusFeature f;
	f.insert(1,Feature());
	map1.push_back(f);
  map1.getColumnHeaders()[1].filename = "bla";
	map1.getColumnHeaders()[1].size = 5;
	map1.setIdentifier("LSID");
	map1.setExperimentType("labeled_MS2");
	map1.getDataProcessing().resize(1);
	map1.getProteinIdentifications().resize(1);
	map1.getUnassignedPeptideIdentifications().resize(1);
	
	map1.clear(false);
	TEST_EQUAL(map1.size(), 0)
	TEST_EQUAL(map1 == ConsensusMap(),false)
  TEST_EQUAL(map1.empty(),true)

	map1.clear(true);
  TEST_EQUAL(map1.size(), 0)
  TEST_EQUAL(map1 == ConsensusMap(),true)
	TEST_EQUAL(map1.empty(),true)
}
END_SECTION

START_SECTION((template < typename Type > Size applyMemberFunction(Size(Type::*member_function)())))
{
  ConsensusMap cm;
  cm.push_back(ConsensusFeature());
  cm.push_back(ConsensusFeature());
  cm.push_back(ConsensusFeature());

  TEST_EQUAL(cm.applyMemberFunction(&UniqueIdInterface::hasInvalidUniqueId),4);
  cm.setUniqueId();
  TEST_EQUAL(cm.applyMemberFunction(&UniqueIdInterface::hasInvalidUniqueId),3);
  cm.applyMemberFunction(&UniqueIdInterface::setUniqueId);
  TEST_EQUAL(cm.applyMemberFunction(&UniqueIdInterface::hasValidUniqueId),4);
  TEST_EQUAL(cm.applyMemberFunction(&UniqueIdInterface::hasInvalidUniqueId),0);
  cm.begin()->clearUniqueId();
  TEST_EQUAL(cm.applyMemberFunction(&UniqueIdInterface::hasValidUniqueId),3);
  TEST_EQUAL(cm.applyMemberFunction(&UniqueIdInterface::hasInvalidUniqueId),1);
}
END_SECTION

START_SECTION((template < typename Type > Size applyMemberFunction(Size(Type::*member_function)() const ) const ))
{
  ConsensusMap cm;
  ConsensusMap const & cmc(cm);
  cm.push_back(ConsensusFeature());
  cm.push_back(ConsensusFeature());
  cm.push_back(ConsensusFeature());

  TEST_EQUAL(cmc.applyMemberFunction(&UniqueIdInterface::hasInvalidUniqueId),4);
  cm.setUniqueId();
  TEST_EQUAL(cmc.applyMemberFunction(&UniqueIdInterface::hasInvalidUniqueId),3);
  cm.applyMemberFunction(&UniqueIdInterface::setUniqueId);
  TEST_EQUAL(cmc.applyMemberFunction(&UniqueIdInterface::hasValidUniqueId),4);
  TEST_EQUAL(cm.applyMemberFunction(&UniqueIdInterface::hasInvalidUniqueId),0);
  cm.begin()->clearUniqueId();
  TEST_EQUAL(cmc.applyMemberFunction(&UniqueIdInterface::hasValidUniqueId),3);
  TEST_EQUAL(cmc.applyMemberFunction(&UniqueIdInterface::hasInvalidUniqueId),1);
}
END_SECTION

START_SECTION(void split(std::vector<FeatureMap>& fmaps, SplitMeta mode = SplitMeta::DISCARD) const)
{
  // prepare test data
  ConsensusMap cm;
  auto& headers = cm.getColumnHeaders();
  headers[0].filename = "file.FeatureXML";
  headers[1].filename = "file2.FeatureXML";

  ConsensusFeature cf1, cf2;
  cf1.insert(FeatureHandle(0, Peak2D({ 10, 433.33 }, 100000), 0));
  cf1.insert(FeatureHandle(1, Peak2D({ 11, 434.33 }, 200000), 0));

  PeptideIdentification id1, id2;
  id1.setRT(10);
  id1.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("AAA")));
  id1.setMetaValue("map_index", 0);
  cf1.getPeptideIdentifications().push_back(id1);
  cf1.setMetaValue("test", "some information");
  cm.push_back(cf1);

  cf2.insert(FeatureHandle(0, Peak2D({ 20, 433.33 }, 300000), 0));
  cf2.insert(FeatureHandle(1, Peak2D({ 21, 433.33 }, 400000), 0));
  id2.setRT(20);
  id2.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("WWW")));
  id2.setMetaValue("map_index", 1);
  cf2.getPeptideIdentifications().push_back(id2);

  cm.push_back(cf2);

  PeptideIdentification uid1, uid2;
  uid1.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("LLL")));
  uid1.setMetaValue("map_index", 0);
  uid2.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("KKK")));
  uid2.setMetaValue("map_index", 1);
  cm.getUnassignedPeptideIdentifications().push_back(uid1);
  cm.getUnassignedPeptideIdentifications().push_back(uid2);

  vector<FeatureMap> fmaps;

  // test with non iso analyze data
  fmaps = cm.split(ConsensusMap::SplitMeta::DISCARD);
  ABORT_IF(fmaps.size() != 2);
  ABORT_IF(fmaps[0].size() != 2);
  ABORT_IF(fmaps[1].size() != 2);
  // map 0
  TEST_EQUAL(fmaps[0][0].getRT(), 10);
  TEST_EQUAL(fmaps[0][0].getIntensity(), 100000);
  TEST_EQUAL(fmaps[0][0].getPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "AAA");
  TEST_EQUAL(fmaps[0][0].metaValueExists("test"), false);
  TEST_EQUAL(fmaps[0][1].getRT(), 20);
  TEST_EQUAL(fmaps[0][1].getIntensity(), 300000);
  TEST_EQUAL(fmaps[0][1].getPeptideIdentifications().empty(), true);
  // map 1
  TEST_EQUAL(fmaps[1][0].getRT(), 11);
  TEST_EQUAL(fmaps[1][0].getIntensity(), 200000);
  TEST_EQUAL(fmaps[1][0].getPeptideIdentifications().empty(), true);
  TEST_EQUAL(fmaps[1][1].getRT(), 21);
  TEST_EQUAL(fmaps[1][1].getIntensity(), 400000);
  TEST_EQUAL(fmaps[1][1].getPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "WWW");
  TEST_EQUAL(fmaps[0].getUnassignedPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "LLL");
  TEST_EQUAL(fmaps[1].getUnassignedPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "KKK");

  // test with iso analyze data
  DataProcessing p;
  set<DataProcessing::ProcessingAction> actions;
  actions.insert(DataProcessing::QUANTITATION);
  p.setProcessingActions(actions);
  p.setSoftware(Software("IsobaricAnalyzer"));
  cm.getDataProcessing().push_back(p);
  fmaps = cm.split(ConsensusMap::SplitMeta::DISCARD);
  ABORT_IF(fmaps.size() != 2);
  ABORT_IF(fmaps[0].size() != 2);
  ABORT_IF(fmaps[1].size() != 2);
  const auto pi00 = fmaps[0][0].getPeptideIdentifications();
  const auto pi01 = fmaps[0][1].getPeptideIdentifications();
  TEST_EQUAL(pi00[0].getHits()[0].getSequence(),AASequence::fromString("AAA"));
  TEST_EQUAL(pi01[0].getHits()[0].getSequence(),AASequence::fromString("WWW"));
  TEST_EQUAL(fmaps[0].getUnassignedPeptideIdentifications()[0].getHits()[0].getSequence(),AASequence::fromString("LLL"));
  TEST_EQUAL(fmaps[0].getUnassignedPeptideIdentifications()[1].getHits()[0].getSequence(),AASequence::fromString("KKK"));
  TEST_EQUAL(fmaps[1][0].getPeptideIdentifications().empty(), true);
  TEST_EQUAL(fmaps[1][1].getPeptideIdentifications().empty(), true);

  // test different all meta value modes
  fmaps = cm.split(ConsensusMap::SplitMeta::COPY_FIRST);
  ABORT_IF(fmaps.size() != 2);
  ABORT_IF(fmaps[0].size() != 2);
  ABORT_IF(fmaps[1].size() != 2);
  TEST_EQUAL(fmaps[0][0].metaValueExists("test"), true);
  TEST_EQUAL(fmaps[0][0].getMetaValue("test"), "some information");
  TEST_EQUAL(fmaps[1][0].metaValueExists("test"), false);
  
  fmaps = cm.split(ConsensusMap::SplitMeta::COPY_ALL);
  ABORT_IF(fmaps.size() != 2);
  ABORT_IF(fmaps[0].size() != 2);
  ABORT_IF(fmaps[1].size() != 2);
  TEST_EQUAL(fmaps[0][0].metaValueExists("test"), true);
  TEST_EQUAL(fmaps[0][0].getMetaValue("test"),"some information");
  TEST_EQUAL(fmaps[1][0].metaValueExists("test"), true);
  TEST_EQUAL(fmaps[1][0].getMetaValue("test"),"some information");
}
END_SECTION

START_SECTION(([EXTRA] std::vector<FeatureMap> split(SplitMeta mode = SplitMeta::DISCARD) const with sparse column header keys))
{
  // column headers keyed {0, 3}: the k-th output map belongs to the k-th header in key order
  ConsensusMap cm;
  cm.getColumnHeaders()[0].filename = "file0.featureXML";
  cm.getColumnHeaders()[3].filename = "file3.featureXML";

  ConsensusFeature cf1, cf2;
  cf1.insert(FeatureHandle(0, Peak2D({ 10, 433.33 }, 100000), 0));
  cf1.insert(FeatureHandle(3, Peak2D({ 11, 434.33 }, 200000), 0));
  PeptideIdentification id1;
  id1.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("AAA")));
  id1.setMetaValue("map_index", 3);
  cf1.getPeptideIdentifications().push_back(id1);
  cm.push_back(cf1);

  cf2.insert(FeatureHandle(3, Peak2D({ 21, 435.33 }, 300000), 1));
  cm.push_back(cf2);

  PeptideIdentification uid0, uid3;
  uid0.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("LLL")));
  uid0.setMetaValue("map_index", 0);
  uid3.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("KKK")));
  uid3.setMetaValue("map_index", 3);
  cm.getUnassignedPeptideIdentifications().push_back(uid0);
  cm.getUnassignedPeptideIdentifications().push_back(uid3);
  TEST_TRUE(cm.isMapConsistent())

  vector<FeatureMap> fmaps = cm.split(ConsensusMap::SplitMeta::DISCARD);
  TEST_EQUAL(fmaps.size(), 2)
  ABORT_IF(fmaps.size() != 2)
  // map index 0
  TEST_EQUAL(fmaps[0].size(), 1)
  ABORT_IF(fmaps[0].size() != 1)
  TEST_EQUAL(fmaps[0][0].getRT(), 10)
  TEST_EQUAL(fmaps[0][0].getPeptideIdentifications().empty(), true)
  TEST_EQUAL(fmaps[0].getUnassignedPeptideIdentifications().size(), 1)
  ABORT_IF(fmaps[0].getUnassignedPeptideIdentifications().size() != 1)
  TEST_EQUAL(fmaps[0].getUnassignedPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "LLL")
  // map index 3
  TEST_EQUAL(fmaps[1].size(), 2)
  ABORT_IF(fmaps[1].size() != 2)
  TEST_EQUAL(fmaps[1][0].getRT(), 11)
  TEST_EQUAL(fmaps[1][0].getPeptideIdentifications().size(), 1)
  ABORT_IF(fmaps[1][0].getPeptideIdentifications().size() != 1)
  TEST_EQUAL(fmaps[1][0].getPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "AAA")
  TEST_EQUAL(fmaps[1][1].getRT(), 21)
  TEST_EQUAL(fmaps[1].getUnassignedPeptideIdentifications().size(), 1)
  ABORT_IF(fmaps[1].getUnassignedPeptideIdentifications().size() != 1)
  TEST_EQUAL(fmaps[1].getUnassignedPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "KKK")
}
END_SECTION

START_SECTION(([EXTRA] std::vector<FeatureMap> split(SplitMeta mode = SplitMeta::DISCARD) const with PeptideIdentifications of unknown map indices))
{
  // Identifications that reference a map without a column header (e.g. kept from a map whose column was
  // removed) do not make the map inconsistent. No output map belongs to their run, so split() drops them
  // instead of filing them under another map, and logs one warning per call that lists the dropped counts.
  //
  // OpenMS LogStream does not write a line again that is identical to one of the last lines it wrote (it
  // only counts the repeats), so the log assertions below rely on these warnings being logged here for the
  // first time; the unusual map indices 23 and 117 keep other sections from logging the same text first.
  auto splitCapturingWarnings = [](const ConsensusMap& map, ConsensusMap::SplitMeta mode, std::string& log)
  {
    std::ostringstream warnings;
    OPENMS_LOG_WARN.insert(warnings);
    vector<FeatureMap> result;
    try
    {
      result = map.split(mode);
    }
    catch (...)
    {
      OPENMS_LOG_WARN.remove(warnings);
      throw;
    }
    OPENMS_LOG_WARN.remove(warnings);
    log = warnings.str();
    return result;
  };
  auto count = [](const std::string& log, const std::string& what)
  {
    Size n(0);
    for (std::string::size_type pos = log.find(what); pos != std::string::npos; pos = log.find(what, pos + what.size()))
    {
      ++n;
    }
    return n;
  };

  ConsensusMap cm;
  cm.getColumnHeaders()[0].filename = "file0.featureXML";
  cm.getColumnHeaders()[1].filename = "file1.featureXML";

  ConsensusFeature cf;
  cf.insert(FeatureHandle(0, Peak2D({ 10, 433.33 }, 100000), 0));
  cf.insert(FeatureHandle(1, Peak2D({ 11, 434.33 }, 200000), 0));
  PeptideIdentification id0, id23, id117;
  id0.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("AAA")));
  id0.setMetaValue("map_index", 0);
  id23.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("CCC")));
  id23.setMetaValue("map_index", 23);
  id117.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("DDD")));
  id117.setMetaValue("map_index", 117);
  cf.getPeptideIdentifications().push_back(id0);
  cf.getPeptideIdentifications().push_back(id23);
  cf.getPeptideIdentifications().push_back(id117);
  cm.push_back(cf);

  PeptideIdentification uid1, uid23;
  uid1.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("KKK")));
  uid1.setMetaValue("map_index", 1);
  uid23.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("EEE")));
  uid23.setMetaValue("map_index", 23);
  cm.getUnassignedPeptideIdentifications().push_back(uid1);
  cm.getUnassignedPeptideIdentifications().push_back(uid23);
  TEST_TRUE(cm.isMapConsistent())

  std::string log;
  vector<FeatureMap> fmaps = splitCapturingWarnings(cm, ConsensusMap::SplitMeta::COPY_ALL, log);

  TEST_EQUAL(fmaps.size(), 2)
  ABORT_IF(fmaps.size() != 2)
  // no features are made up for the unknown map indices
  TEST_EQUAL(fmaps[0].size(), 1)
  TEST_EQUAL(fmaps[1].size(), 1)
  ABORT_IF(fmaps[0].size() != 1 || fmaps[1].size() != 1)
  TEST_EQUAL(fmaps[0][0].getPeptideIdentifications().size(), 1)
  ABORT_IF(fmaps[0][0].getPeptideIdentifications().size() != 1)
  TEST_EQUAL(fmaps[0][0].getPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "AAA")
  TEST_EQUAL(fmaps[1][0].getPeptideIdentifications().empty(), true)

  // the identifications of unknown maps are in none of the maps
  TEST_EQUAL(fmaps[0].getUnassignedPeptideIdentifications().size(), 0)
  TEST_EQUAL(fmaps[1].getUnassignedPeptideIdentifications().size(), 1)
  ABORT_IF(fmaps[1].getUnassignedPeptideIdentifications().size() != 1)
  TEST_EQUAL(fmaps[1].getUnassignedPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "KKK")

  // one warning for the call, listing each unknown map index with its number of dropped identifications
  TEST_EQUAL(count(log, "ConsensusMap::split()"), 1)
  TEST_EQUAL(count(log, "dropped PeptideIdentifications"), 1)
  TEST_EQUAL(count(log, "(map index 23: 2, map index 117: 1)"), 1)

  // without any column, all identifications are dropped and there are no maps
  ConsensusMap no_columns;
  ConsensusFeature cf_without_handles;
  cf_without_handles.getPeptideIdentifications().push_back(id117);
  no_columns.push_back(cf_without_handles);
  no_columns.getUnassignedPeptideIdentifications().push_back(uid23);
  std::string no_columns_log;
  TEST_EQUAL(splitCapturingWarnings(no_columns, ConsensusMap::SplitMeta::DISCARD, no_columns_log).size(), 0)
  TEST_EQUAL(count(no_columns_log, "dropped PeptideIdentifications"), 1)
  TEST_EQUAL(count(no_columns_log, "(map index 23: 1, map index 117: 1)"), 1)
}
END_SECTION

START_SECTION(([EXTRA] std::vector<FeatureMap> split(SplitMeta mode = SplitMeta::DISCARD) const with a FeatureHandle of an unknown map index))
{
  // column headers keyed {0, 3} give two output maps; map index 1 is not a header key although it would be
  // a valid position in the output, so it must not be filed under the second map (the one of map index 3)
  ConsensusMap cm;
  cm.getColumnHeaders()[0].filename = "file0.featureXML";
  cm.getColumnHeaders()[3].filename = "file3.featureXML";

  ConsensusFeature cf;
  cf.insert(FeatureHandle(0, Peak2D({ 10, 433.33 }, 100000), 0));
  cf.insert(FeatureHandle(1, Peak2D({ 11, 434.33 }, 200000), 0));
  cm.push_back(cf);
  TEST_FALSE(cm.isMapConsistent())

  TEST_EXCEPTION(Exception::ElementNotFound, cm.split())
}
END_SECTION

START_SECTION(([EXTRA] std::vector<FeatureMap> split(SplitMeta mode = SplitMeta::DISCARD) const with a PeptideIdentification of a column without a FeatureHandle))
{
  // an assigned identification whose map has a column, but no handle on its consensus feature, gets an
  // empty feature of its own in that map
  ConsensusMap cm;
  cm.getColumnHeaders()[0].filename = "file0.featureXML";
  cm.getColumnHeaders()[1].filename = "file1.featureXML";

  ConsensusFeature cf;
  cf.insert(FeatureHandle(0, Peak2D({ 10, 433.33 }, 100000), 0));
  PeptideIdentification id1;
  id1.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("AAA")));
  id1.setMetaValue("map_index", 1);
  cf.getPeptideIdentifications().push_back(id1);
  cm.push_back(cf);

  vector<FeatureMap> fmaps = cm.split();
  TEST_EQUAL(fmaps.size(), 2)
  ABORT_IF(fmaps.size() != 2)
  TEST_EQUAL(fmaps[0].size(), 1)
  ABORT_IF(fmaps[0].size() != 1)
  TEST_EQUAL(fmaps[0][0].getRT(), 10)
  TEST_EQUAL(fmaps[0][0].getPeptideIdentifications().empty(), true)
  TEST_EQUAL(fmaps[1].size(), 1)
  ABORT_IF(fmaps[1].size() != 1)
  TEST_EQUAL(fmaps[1][0].getRT(), 0)
  TEST_EQUAL(fmaps[1][0].getMZ(), 0)
  TEST_EQUAL(fmaps[1][0].getIntensity(), 0)
  TEST_EQUAL(fmaps[1][0].getPeptideIdentifications().size(), 1)
  ABORT_IF(fmaps[1][0].getPeptideIdentifications().size() != 1)
  TEST_EQUAL(fmaps[1][0].getPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "AAA")
}
END_SECTION

START_SECTION(([EXTRA] std::vector<FeatureMap> split(SplitMeta mode = SplitMeta::DISCARD) const with a missing or invalid map_index meta value))
{
  ConsensusMap cm;
  cm.getColumnHeaders()[0].filename = "file0.featureXML";

  PeptideIdentification no_index, negative_index, string_index;
  negative_index.setMetaValue("map_index", -1);
  string_index.setMetaValue("map_index", "0");

  ConsensusMap unassigned_no_index(cm), unassigned_negative(cm), unassigned_string(cm);
  unassigned_no_index.getUnassignedPeptideIdentifications().push_back(no_index);
  unassigned_negative.getUnassignedPeptideIdentifications().push_back(negative_index);
  unassigned_string.getUnassignedPeptideIdentifications().push_back(string_index);
  TEST_EXCEPTION(Exception::MissingInformation, unassigned_no_index.split())
  TEST_EXCEPTION(Exception::ConversionError, unassigned_negative.split())
  TEST_EXCEPTION(Exception::ConversionError, unassigned_string.split())

  ConsensusMap assigned_negative(cm), assigned_string(cm);
  ConsensusFeature cf_negative, cf_string;
  cf_negative.insert(FeatureHandle(0, Peak2D({ 10, 433.33 }, 100000), 0));
  cf_negative.getPeptideIdentifications().push_back(negative_index);
  assigned_negative.push_back(cf_negative);
  cf_string.insert(FeatureHandle(0, Peak2D({ 10, 433.33 }, 100000), 0));
  cf_string.getPeptideIdentifications().push_back(string_index);
  assigned_string.push_back(cf_string);
  TEST_EXCEPTION(Exception::ConversionError, assigned_negative.split())
  TEST_EXCEPTION(Exception::ConversionError, assigned_string.split())
}
END_SECTION

START_SECTION(([EXTRA] std::vector<FeatureMap> split(SplitMeta mode = SplitMeta::DISCARD) const with IsobaricAnalyzer data and sparse or missing column header keys))
{
  // For IsobaricAnalyzer data the features are resolved through the column header keys as well, but every
  // identification goes to the first map, the one of map index 0, whatever its map_index meta value.
  DataProcessing isobaric_analyzer;
  isobaric_analyzer.setSoftware(Software("IsobaricAnalyzer"));

  // column headers keyed {0, 3}
  ConsensusMap cm;
  cm.getDataProcessing().push_back(isobaric_analyzer);
  cm.getColumnHeaders()[0].filename = "run.mzML";
  cm.getColumnHeaders()[3].filename = "run.mzML";

  ConsensusFeature cf;
  cf.insert(FeatureHandle(0, Peak2D({ 10, 433.33 }, 100000), 0));
  cf.insert(FeatureHandle(3, Peak2D({ 10, 433.33 }, 200000), 0));
  PeptideIdentification id;
  id.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("AAA")));
  id.setMetaValue("map_index", 7); // not a column header key, but not used for IsobaricAnalyzer data
  cf.getPeptideIdentifications().push_back(id);
  cm.push_back(cf);

  PeptideIdentification uid;
  uid.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("KKK")));
  uid.setMetaValue("map_index", 3);
  cm.getUnassignedPeptideIdentifications().push_back(uid);
  cm.getProteinIdentifications().resize(1);
  cm.getProteinIdentifications()[0].setIdentifier("run");
  TEST_TRUE(cm.isMapConsistent())

  vector<FeatureMap> fmaps = cm.split();
  TEST_EQUAL(fmaps.size(), 2)
  ABORT_IF(fmaps.size() != 2)
  // map index 0: its feature, all peptide identifications and all protein identifications
  TEST_EQUAL(fmaps[0].size(), 1)
  ABORT_IF(fmaps[0].size() != 1)
  TEST_EQUAL(fmaps[0][0].getIntensity(), 100000)
  TEST_EQUAL(fmaps[0][0].getPeptideIdentifications().size(), 1)
  ABORT_IF(fmaps[0][0].getPeptideIdentifications().size() != 1)
  TEST_EQUAL(fmaps[0][0].getPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "AAA")
  TEST_EQUAL(fmaps[0].getUnassignedPeptideIdentifications().size(), 1)
  ABORT_IF(fmaps[0].getUnassignedPeptideIdentifications().size() != 1)
  TEST_EQUAL(fmaps[0].getUnassignedPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "KKK")
  TEST_EQUAL(fmaps[0].getProteinIdentifications().size(), 1)
  // map index 3: only its feature
  TEST_EQUAL(fmaps[1].size(), 1)
  ABORT_IF(fmaps[1].size() != 1)
  TEST_EQUAL(fmaps[1][0].getIntensity(), 200000)
  TEST_EQUAL(fmaps[1][0].getPeptideIdentifications().empty(), true)
  TEST_EQUAL(fmaps[1].getUnassignedPeptideIdentifications().empty(), true)
  TEST_EQUAL(fmaps[1].getProteinIdentifications().empty(), true)

  // Without a column of map index 0 there is no first map for the identifications: neither with columns {1},
  // where the only output map belongs to map index 1, nor without any column.
  ConsensusMap no_column_0;
  no_column_0.getDataProcessing().push_back(isobaric_analyzer);
  no_column_0.getColumnHeaders()[1].filename = "run.mzML";
  no_column_0.getUnassignedPeptideIdentifications().push_back(uid);
  TEST_EXCEPTION(Exception::ElementNotFound, no_column_0.split())
  no_column_0.getColumnHeaders().clear();
  TEST_EXCEPTION(Exception::ElementNotFound, no_column_0.split())
}
END_SECTION

START_SECTION(([EXTRA] std::vector<FeatureMap> split(SplitMeta mode = SplitMeta::DISCARD) const with several FeatureHandles of the same map index))
{
  // A consensus feature can hold several handles of one map index (the handle set orders them by map index,
  // then unique id), and the map is still consistent. split() makes at most one feature per map index of a
  // consensus feature: the handle with the largest unique id is kept, the others are dropped silently. The
  // feature does not carry the unique id (BaseFeature(const FeatureHandle&) copies the handle as a Peak2D), so
  // the handles are told apart by position and intensity.
  ConsensusMap cm;
  cm.getColumnHeaders()[0].filename = "file0.featureXML";
  cm.getColumnHeaders()[1].filename = "file1.featureXML";

  ConsensusFeature cf;
  // inserted in reverse unique id order: which handle is kept does not depend on the insertion order
  cf.insert(FeatureHandle(0, Peak2D({ 20, 533.33 }, 200000), 2));
  cf.insert(FeatureHandle(0, Peak2D({ 10, 433.33 }, 100000), 1));
  cf.insert(FeatureHandle(1, Peak2D({ 30, 633.33 }, 300000), 3));
  PeptideIdentification id0;
  id0.insertHit(PeptideHit(0.1, 1, 3, AASequence::fromString("AAA")));
  id0.setMetaValue("map_index", 0);
  cf.getPeptideIdentifications().push_back(id0);
  cm.push_back(cf);
  TEST_EQUAL(cm[0].size(), 3)
  TEST_TRUE(cm.isMapConsistent())

  std::ostringstream warnings;
  vector<FeatureMap> fmaps;
  OPENMS_LOG_WARN.insert(warnings);
  try
  {
    fmaps = cm.split(ConsensusMap::SplitMeta::DISCARD);
  }
  catch (...)
  {
    OPENMS_LOG_WARN.remove(warnings);
    throw;
  }
  OPENMS_LOG_WARN.remove(warnings);
  TEST_EQUAL(warnings.str().find("ConsensusMap::split()"), std::string::npos)

  TEST_EQUAL(fmaps.size(), 2)
  ABORT_IF(fmaps.size() != 2)
  TEST_EQUAL(fmaps[0].size(), 1)
  ABORT_IF(fmaps[0].size() != 1)
  TEST_EQUAL(fmaps[0][0].getRT(), 20)
  TEST_EQUAL(fmaps[0][0].getMZ(), 533.33)
  TEST_EQUAL(fmaps[0][0].getIntensity(), 200000)
  // the identification of map index 0 goes to the one feature that is kept
  TEST_EQUAL(fmaps[0][0].getPeptideIdentifications().size(), 1)
  ABORT_IF(fmaps[0][0].getPeptideIdentifications().size() != 1)
  TEST_EQUAL(fmaps[0][0].getPeptideIdentifications()[0].getHits()[0].getSequence().toString(), "AAA")
  TEST_EQUAL(fmaps[1].size(), 1)
  ABORT_IF(fmaps[1].size() != 1)
  TEST_EQUAL(fmaps[1][0].getRT(), 30)
}
END_SECTION

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST
