// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hannes Roest $
// $Authors: Hannes Roest $
// --------------------------------------------------------------------------

#include <OpenMS/KERNEL/MRMFeature.h>

#include <OpenMS/CONCEPT/Exception.h>

namespace OpenMS
{
  namespace
  {
    /// Index lookup for the id->index maps; a miss must not default-insert, since that would
    /// register a key that was never added and make the read return an unrelated feature
    int featureIndex_(const std::map<std::string, int>& index_map, const std::string& key, const char* function)
    {
      std::map<std::string, int>::const_iterator it = index_map.find(key);
      if (it == index_map.end())
      {
        throw Exception::ElementNotFound(__FILE__, __LINE__, function, key);
      }
      return it->second;
    }
  }

  MRMFeature::MRMFeature() :
    Feature()
  {
  }

  /// Copy constructor
  MRMFeature::MRMFeature(const MRMFeature & rhs) :
    Feature(rhs),
    features_(rhs.features_),
    precursor_features_(rhs.precursor_features_),
    pg_scores_(rhs.pg_scores_),
    feature_map_(rhs.feature_map_),
    precursor_feature_map_(rhs.precursor_feature_map_)
  {
    setScores(rhs.getScores());
  }

  /// Assignment operator
  MRMFeature & MRMFeature::operator = (const MRMFeature &rhs)
  {
    if (&rhs == this)
    {
      return *this;
    }
    Feature::operator = (rhs);
    setScores(rhs.getScores());
    features_ = rhs.features_;
    precursor_features_ = rhs.precursor_features_;
    feature_map_ = rhs.feature_map_;
    precursor_feature_map_ = rhs.precursor_feature_map_;

    return *this;
  }

  MRMFeature::~MRMFeature() = default;

  const OpenSwath_Scores & MRMFeature::getScores() const
  {
    return pg_scores_;
  }

  OpenSwath_Scores & MRMFeature::getScores()
  {
    return pg_scores_;
  }

  void MRMFeature::setScores(const OpenSwath_Scores & scores)
  {
    pg_scores_ = scores;
  }

  void MRMFeature::addScore(const std::string & score_name, double score)
  {
    setMetaValue(score_name, score);
  }

  void MRMFeature::addFeature(const Feature & feature, const std::string& key)
  {
    // a repeated key replaces the feature in place: appending would leave the previously keyed
    // feature in features_ with no map entry pointing at it, so getFeatures() and getFeatureIDs()
    // would disagree and the stranded entry would still be counted downstream
    std::pair<std::map<std::string, int>::iterator, bool> pos = feature_map_.emplace(key, Int(features_.size()));
    if (pos.second)
    {
      features_.push_back(feature);
    }
    else
    {
      features_.at(pos.first->second) = feature;
    }
  }

  void MRMFeature::addFeature(Feature && feature, const std::string& key)
  {
    std::pair<std::map<std::string, int>::iterator, bool> pos = feature_map_.emplace(key, Int(features_.size()));
    if (pos.second)
    {
      features_.push_back(std::move(feature));
    }
    else
    {
      features_.at(pos.first->second) = std::move(feature);
    }
  }

  Feature & MRMFeature::getFeature(const std::string& key) 
  {
    return features_.at(featureIndex_(feature_map_, key, OPENMS_PRETTY_FUNCTION));
  }

  const Feature & MRMFeature::getFeature(const std::string& key) const 
  {
    return features_.at(featureIndex_(feature_map_, key, OPENMS_PRETTY_FUNCTION));
  }

  const std::vector<Feature> & MRMFeature::getFeatures() const
  {
    return features_;
  }

  void MRMFeature::getFeatureIDs(std::vector<std::string> & result) const
  {
    for (std::map<std::string, int>::const_iterator it = feature_map_.begin(); it != feature_map_.end(); ++it)
    {
      result.push_back(it->first);
    }
  }

  void MRMFeature::addPrecursorFeature(const Feature & feature, const std::string& key)
  {
    // see addFeature: replacing in place keeps precursor_features_ reachable through the map
    std::pair<std::map<std::string, int>::iterator, bool> pos = precursor_feature_map_.emplace(key, Int(precursor_features_.size()));
    if (pos.second)
    {
      precursor_features_.push_back(feature);
    }
    else
    {
      precursor_features_.at(pos.first->second) = feature;
    }
  }

  void MRMFeature::addPrecursorFeature(Feature && feature, const std::string& key)
  {
    std::pair<std::map<std::string, int>::iterator, bool> pos = precursor_feature_map_.emplace(key, Int(precursor_features_.size()));
    if (pos.second)
    {
      precursor_features_.push_back(std::move(feature));
    }
    else
    {
      precursor_features_.at(pos.first->second) = std::move(feature);
    }
  }

  void MRMFeature::getPrecursorFeatureIDs(std::vector<std::string> & result) const
  {
    for (std::map<std::string, int>::const_iterator it = precursor_feature_map_.begin(); it != precursor_feature_map_.end(); ++it)
    {
      result.push_back(it->first);
    }
  }

  Feature & MRMFeature::getPrecursorFeature(const std::string& key)
  {
    return precursor_features_.at(featureIndex_(precursor_feature_map_, key, OPENMS_PRETTY_FUNCTION));
  }

  const Feature & MRMFeature::getPrecursorFeature(const std::string& key) const
  {
    return precursor_features_.at(featureIndex_(precursor_feature_map_, key, OPENMS_PRETTY_FUNCTION));
  }

  void MRMFeature::IDScoresAsMetaValue(bool decoy, const OpenSwath_Ind_Scores& idscores)
  {
    std::string id = "id_target_";
    if (decoy)
    {
      id = "id_decoy_";
    }
    setMetaValue(id + "transition_names", idscores.ind_transition_names);
    setMetaValue(id + "num_transitions", idscores.ind_num_transitions);
    setMetaValue(id + "area_intensity", idscores.ind_area_intensity);
    setMetaValue(id + "total_area_intensity", idscores.ind_total_area_intensity);
    setMetaValue(id + "intensity_score", idscores.ind_intensity_score);
    setMetaValue(id + "intensity_ratio_score", idscores.ind_intensity_ratio);
    setMetaValue(id + "apex_intensity", idscores.ind_apex_intensity);
    setMetaValue(id + "peak_apex_position", idscores.ind_apex_position);
    setMetaValue(id + "width_at_50", idscores.ind_fwhm);
    setMetaValue(id + "total_mi", idscores.ind_total_mi);
    setMetaValue(id + "ind_log_intensity", idscores.ind_log_intensity);
    setMetaValue(id + "ind_xcorr_coelution", idscores.ind_xcorr_coelution_score);
    setMetaValue(id + "ind_xcorr_shape", idscores.ind_xcorr_shape_score);
    setMetaValue(id + "ind_log_sn_score", idscores.ind_log_sn_score);
    setMetaValue(id + "ind_isotope_correlation", idscores.ind_isotope_correlation);
    setMetaValue(id + "ind_isotope_overlap", idscores.ind_isotope_overlap);
    setMetaValue(id + "ind_massdev_score", idscores.ind_massdev_score);
    setMetaValue(id + "ind_mi_score", idscores.ind_mi_score);
    setMetaValue(id + "ind_mi_ratio_score", idscores.ind_mi_ratio);

    // Ion mobility scores
    setMetaValue(id + "ind_im_drift", idscores.ind_im_drift);
    setMetaValue(id + "ind_im_drift_left", idscores.ind_im_drift_left);
    setMetaValue(id + "ind_im_drift_right", idscores.ind_im_drift_right);
    setMetaValue(id + "ind_im_delta", idscores.ind_im_delta);
    setMetaValue(id + "ind_im_delta_score", idscores.ind_im_delta_score);
    setMetaValue(id + "ind_im_log_intensity", idscores.ind_im_log_intensity);
    setMetaValue(id + "ind_im_contrast_coelution", idscores.ind_im_contrast_coelution);
    setMetaValue(id + "ind_im_contrast_shape", idscores.ind_im_contrast_shape);
    setMetaValue(id + "ind_im_sum_contrast_coelution", idscores.ind_im_sum_contrast_coelution);
    setMetaValue(id + "ind_im_sum_contrast_shape", idscores.ind_im_sum_contrast_shape);

    // peak shape metrics
    setMetaValue(id + "ind_start_position_at_5", idscores.ind_start_position_at_5);
    setMetaValue(id + "ind_end_position_at_5", idscores.ind_end_position_at_5);
    setMetaValue(id + "ind_start_position_at_10", idscores.ind_start_position_at_10);
    setMetaValue(id + "ind_end_position_at_10", idscores.ind_end_position_at_10);
    setMetaValue(id + "ind_start_position_at_50", idscores.ind_start_position_at_50);
    setMetaValue(id + "ind_end_position_at_50", idscores.ind_end_position_at_50);
    setMetaValue(id + "ind_total_width", idscores.ind_total_width);
    setMetaValue(id + "ind_tailing_factor", idscores.ind_tailing_factor);
    setMetaValue(id + "ind_asymmetry_factor", idscores.ind_asymmetry_factor);
    setMetaValue(id + "ind_slope_of_baseline", idscores.ind_slope_of_baseline);
    setMetaValue(id + "ind_baseline_delta_2_height", idscores.ind_baseline_delta_2_height);
    setMetaValue(id + "ind_points_across_baseline", idscores.ind_points_across_baseline);
    setMetaValue(id + "ind_points_across_half_height", idscores.ind_points_across_half_height);


  }
}

