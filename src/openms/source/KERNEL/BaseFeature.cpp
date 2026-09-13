// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hendrik Weisser $
// $Authors: Hendrik Weisser, Chris Bielow $
// --------------------------------------------------------------------------

#include <OpenMS/KERNEL/BaseFeature.h>
#include <OpenMS/KERNEL/FeatureHandle.h>

using namespace std;

namespace OpenMS
{
  const std::string BaseFeature::NamesOfAnnotationState[] =
    {"no ID", "single ID", "multiple IDs (identical)", "multiple IDs (divergent)"};


  BaseFeature::BaseFeature() :
    RichPeak2D(), quality_(0.0), charge_(0), width_(0)
  {
  }

  BaseFeature::BaseFeature(const BaseFeature& rhs, UInt64 map_index) :
      RichPeak2D(rhs), quality_(rhs.quality_), charge_(rhs.charge_), width_(rhs.width_),
      peptides_(rhs.peptides_), primary_id_(rhs.primary_id_), id_matches_(rhs.id_matches_)
  {
    for (auto& pep : this->peptides_)
    {
      pep.setMetaValue("map_index", map_index);
    }
  }

  BaseFeature::BaseFeature(const RichPeak2D& point) :
    RichPeak2D(point), quality_(0.0), charge_(0), width_(0)
  {
  }

  BaseFeature::BaseFeature(const FeatureHandle& fh) :
    RichPeak2D(fh),
    quality_(0.0),
    charge_(fh.getCharge()),
    width_(fh.getWidth()),
    peptides_()
  {
  }

  BaseFeature::BaseFeature(const Peak2D& point) :
    RichPeak2D(point), quality_(0.0), charge_(0), width_(0)
  {
  }

  bool BaseFeature::operator==(const BaseFeature& rhs) const
  {
    return RichPeak2D::operator==(rhs)
           && (quality_ == rhs.quality_)
           && (charge_ == rhs.charge_)
           && (width_ == rhs.width_)
           && (peptides_ == rhs.peptides_)
           && (primary_id_ == rhs.primary_id_)
           && (id_matches_ == rhs.id_matches_);
  }

  bool BaseFeature::operator!=(const BaseFeature& rhs) const
  {
    return !operator==(rhs);
  }

  BaseFeature::~BaseFeature() = default;

  BaseFeature::QualityType BaseFeature::getQuality() const
  {
    return quality_;
  }

  void BaseFeature::setQuality(BaseFeature::QualityType quality)
  {
    quality_ = quality;
  }

  BaseFeature::WidthType BaseFeature::getWidth() const
  {
    return width_;
  }

  void BaseFeature::setWidth(BaseFeature::WidthType fwhm)
  {
    // !!! Dirty hack: as long as featureXML doesn't support a width field,
    // we abuse the meta information for this.
    // See also FeatureXMLFile::readFeature_().
    width_ = fwhm;
    setMetaValue("FWHM", fwhm);
  }

  const BaseFeature::ChargeType& BaseFeature::getCharge() const
  {
    return charge_;
  }

  void BaseFeature::setCharge(const BaseFeature::ChargeType& charge)
  {
    charge_ = charge;
  }

  const PeptideIdentificationList& BaseFeature::getPeptideIdentifications()
  const
  {
    return peptides_;
  }

  PeptideIdentificationList& BaseFeature::getPeptideIdentifications()
  {
    return peptides_;
  }

  void BaseFeature::setPeptideIdentifications(
    const PeptideIdentificationList& peptides)
  {
    peptides_ = peptides;
  }

  void BaseFeature::sortPeptideIdentifications()
  {
    // the hits are sorted in a pass of their own: doing it from inside the comparator would modify
    // the objects std::sort is comparing, and would leave the hits of any element that the sort
    // never happens to compare (e.g. the only identification of a feature) unsorted
    for (PeptideIdentification& pep : peptides_)
    {
      pep.sort();
    }
    // the score orientation is taken from the first identification that has hits and then used for
    // every comparison: reading it from the left operand would make the comparator asymmetric as
    // soon as two identifications disagree, which breaks the strict weak ordering std::sort requires
    bool higher_score_better = true;
    for (const PeptideIdentification& pep : peptides_)
    {
      // hits, not PeptideIdentification::empty(): that is false for a hit-less identification
      // read from featureXML, which carries its identifier and score type
      if (!pep.getHits().empty())
      {
        higher_score_better = pep.isHigherScoreBetter();
        break;
      }
    }
    std::sort(peptides_.rbegin(),peptides_.rend(),
              [higher_score_better](const PeptideIdentification& p1, const PeptideIdentification& p2)
              {
              // two identifications without hits are equivalent; returning true for both orders
              // would violate asymmetry (undefined behaviour in std::sort)
              if (p1.getHits().empty())
              {
                return !p2.getHits().empty();
              }
              if (p2.getHits().empty())
              {
                return false; // getHits()[0] below would read past the end of an empty vector
              }
              if (higher_score_better)
              {
                return p1.getHits()[0].getScore() < p2.getHits()[0].getScore();
              }
              else
              {
                return p1.getHits()[0].getScore() > p2.getHits()[0].getScore();
              }});
  }

  BaseFeature::AnnotationState BaseFeature::getAnnotationState() const
  {
    if (id_matches_.empty()) // consider IDs in old format
    {
      if (peptides_.empty())
      {
        return AnnotationState::FEATURE_ID_NONE;
      }
      if (peptides_.size() == 1 && !peptides_[0].getHits().empty())
      {
        return AnnotationState::FEATURE_ID_SINGLE;
      }
      std::set<std::string> seqs;
      for (Size i = 0; i < peptides_.size(); ++i)
      {
        if (!peptides_[i].getHits().empty())
        {
          PeptideIdentification id_tmp = peptides_[i];
          id_tmp.sort();  // look at best hit only - requires sorting
          seqs.insert(id_tmp.getHits()[0].getSequence().toString());
        }
      }
      // note: the state counts attached identifications, not sequences, so more than one attached
      // identification yields a MULTIPLE_* state even when only one of them contributed a sequence
      // (the shortcut above is the only path to SINGLE). Kept as is because tools branch on the
      // existing states.
      if (seqs.size() == 1)
      {
        return AnnotationState::FEATURE_ID_MULTIPLE_SAME; // hits have identical seqs
      }
      if (seqs.size() > 1)
      {
        return AnnotationState::FEATURE_ID_MULTIPLE_DIVERGENT; // multiple different annotations ... probably bad mapping
      }
      /*else if (seqs.size()==0)*/
      return AnnotationState::FEATURE_ID_NONE;   // very rare case of empty hits
    }
    else // consider IDs in new format
    {
      if (id_matches_.size() == 1)
      {
        return AnnotationState::FEATURE_ID_SINGLE;
      }
      // if there are multiple IDs, check if all are equal (to the first):
      auto it = id_matches_.begin();
      IdentificationData::IdentifiedMolecule molecule = (*it)->identified_molecule_var;
      for (++it; it != id_matches_.end(); ++it)
      {
        if ((*it)->identified_molecule_var != molecule)
        {
          return AnnotationState::FEATURE_ID_MULTIPLE_DIVERGENT;
        }
      }
      return AnnotationState::FEATURE_ID_MULTIPLE_SAME;
    }
  }


  bool BaseFeature::hasPrimaryID() const
  {
    return bool(primary_id_);
  }


  const IdentificationData::IdentifiedMolecule& BaseFeature::getPrimaryID() const
  {
    if (!primary_id_)
    {
      throw Exception::MissingInformation(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
                                          "no primary ID assigned");
    }

    return *primary_id_; // unpack the option
  }


  void BaseFeature::clearPrimaryID()
  {
    primary_id_ = nullopt;
  }


  void BaseFeature::setPrimaryID(const IdentificationData::IdentifiedMolecule& id)
  {
    primary_id_ = id;
  }


  const std::set<IdentificationData::ObservationMatchRef>& BaseFeature::getIDMatches() const
  {
    return id_matches_;
  }


  std::set<IdentificationData::ObservationMatchRef>& BaseFeature::getIDMatches()
  {
    return id_matches_;
  }


  void BaseFeature::addIDMatch(IdentificationData::ObservationMatchRef ref)
  {
    id_matches_.insert(ref);
  }

  void BaseFeature::updateIDReferences(const IdentificationData::RefTranslator& trans)
  {
    // everything is translated into temporaries first and only committed once every translation has
    // succeeded: RefTranslator::translate throws for an unmapped reference, and assigning as we go
    // would leave the feature with only the already translated part of its annotations
    optional<IdentificationData::IdentifiedMolecule> primary_id;
    if (primary_id_ != nullopt) // is feature annotated with a "primary ID"?
    {
      primary_id = trans.translate(*primary_id_);
    }
    set<IdentificationData::ObservationMatchRef> matches; // refs. to e.g. PSMs
    for (const auto& item : id_matches_)
    {
      matches.insert(trans.translate(item));
    }
    if (primary_id != nullopt)
    {
      primary_id_ = primary_id;
    }
    id_matches_.swap(matches);
  }

} // namespace OpenMS
