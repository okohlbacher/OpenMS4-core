// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
// 
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Stephan Aiche $
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/test_config.h>

///////////////////////////
#include <OpenMS/CHEMISTRY/MASSDECOMPOSITION/IMS/IntegerMassDecomposer.h>
///////////////////////////

#include <OpenMS/CHEMISTRY/MASSDECOMPOSITION/IMS/IMSAlphabet.h>

#include <OpenMS/CHEMISTRY/ResidueDB.h>
#include <OpenMS/CHEMISTRY/Residue.h>

#include <map>

using namespace OpenMS;
using namespace ims;
using namespace std;

Weights createWeights()
{
  std::map<char, double> aa_to_weight;

  set<const Residue*> residues = ResidueDB::getInstance()->getResidues("Natural19WithoutI");

  for (set<const Residue*>::const_iterator it = residues.begin(); it != residues.end(); ++it)
  {
    aa_to_weight[(*it)->getOneLetterCode()[0]] = (*it)->getMonoWeight(Residue::Internal);
  }

  // init mass decomposer
  IMSAlphabet alphabet;
  for (std::map<char, double>::const_iterator it = aa_to_weight.begin(); it != aa_to_weight.end(); ++it)
  {
    alphabet.push_back(StringUtils::toStr(it->first), it->second);
  }

  // initializes weights
  Weights weights(alphabet.getMasses(), 0.01);

  // optimize alphabet by dividing by gcd
  weights.divideByGCD();

  return weights;
}

START_TEST(IntegerMassDecomposer, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

IntegerMassDecomposer<>* ptr = nullptr;
IntegerMassDecomposer<>* null_ptr = nullptr;

START_SECTION((IntegerMassDecomposer(const Weights &alphabet_)))
{
  ptr = new IntegerMassDecomposer<>(createWeights());
  TEST_NOT_EQUAL(ptr, null_ptr)
}
END_SECTION

START_SECTION(~IntegerMassDecomposer())
{
	delete ptr;
}
END_SECTION

START_SECTION((bool exist(value_type mass)))
{
  // TODO
}
END_SECTION

START_SECTION((IntegerMassDecomposer< ValueType, DecompositionValueType >::decomposition_type getDecomposition(value_type mass)))
{
  // gcd(10, 25) == 5, so the third column is built by the cache-optimised
  // (gcd > 1) branch of the extended residue table, which is where the witness
  // counts are produced. 73 = 3*16 + 1*25 is decomposable over this alphabet.
  Weights::alphabet_masses_type masses;
  masses.push_back(10.0);
  masses.push_back(16.0);
  masses.push_back(25.0);
  Weights gcd_weights(masses, 1.0);

  IntegerMassDecomposer<> gcd_decomposer(gcd_weights);
  TEST_EQUAL(gcd_decomposer.exist(73), true)

  IntegerMassDecomposer<>::decomposition_type decomp = gcd_decomposer.getDecomposition(73);
  TEST_EQUAL(decomp.size(), gcd_weights.size())

  // whichever decomposition is returned, it has to add up to the requested mass
  IntegerMassDecomposer<>::value_type sum = 0;
  for (Weights::size_type i = 0; i < gcd_weights.size(); ++i)
  {
    sum += decomp[i] * gcd_weights.getWeight(i);
  }
  TEST_EQUAL(sum, 73u)
}
END_SECTION

START_SECTION((IntegerMassDecomposer< ValueType, DecompositionValueType >::decompositions_type getAllDecompositions(value_type mass)))
{
  // TODO
}
END_SECTION

START_SECTION((IntegerMassDecomposer< ValueType, DecompositionValueType >::decomposition_value_type getNumberOfDecompositions(value_type mass)))
{
  // TODO
}
END_SECTION


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST



