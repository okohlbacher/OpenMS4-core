// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Chris Bielow $
// $Authors: Clemens Groepl $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/CONCEPT/UniqueIdInterface.h>
#include <OpenMS/CONCEPT/Exception.h>

#ifdef _MSC_VER // disable some BOOST warnings that distract from ours
#   pragma warning( push ) // save warning state
#   pragma warning( disable : 4396 )
#endif

#include <unordered_map>
#include <sstream>

#ifdef _MSC_VER
#   pragma warning( pop )  // restore old warning state
#endif


namespace OpenMS
{

/**@brief A base class for containers with elements derived from UniqueIdInterface.
 * This adds functionality to convert a unique id into an index into the container.
 * 
 * The derived class needs a data member called @p data_ which holds the actual elements derived from UniqueIdInterface.
 * This is classical CRTP with the additional requirement that the derived class needs to declare a .getData() member function.
 *
 * See FeatureMap and ConsensusMap for living examples.
 * The RandomAccessContainer returned by .getData() must support operator[], at(), and size().
 */
  template<typename T>
  class UniqueIdIndexer
  {
public:
    typedef std::unordered_map<UInt64, Size> UniqueIdMap;

    /**
     @brief Returns the index of the feature with the given unique id, or Size(-1) if none exists in this random access container.

     The complexity is expected constant upon success, linear upon failure.

     The lookup actually performs the following steps:
     - consult the internal hash map (i.e., uniqueid_to_index_)
     - if an index is found and that element indeed has this unique id, then return the index
     - if no index is found or the unique ids do not match, then update the hash map (using updateUniqueIdToIndex()) and lookup the index again
     - if an index is found this time, return it, otherwise return Size(-1)
     .

     @note that subordinate elements are not considered here.

     */
    Size
    uniqueIdToIndex(UInt64 unique_id) const
    {
      Size index;
      try
      {
        index = uniqueid_to_index_.at(unique_id);
        if (getBase_().at(index).getUniqueId() != unique_id)
        {
          throw std::out_of_range("unique_id_to_index_");
        }
      }
      catch (std::out_of_range &)
      {
        try
        {
          this->updateUniqueIdToIndex();
          index = uniqueid_to_index_.at(unique_id);
        }
        catch (std::out_of_range &)
        {
          index = -1;   // which means: invalid
        }
      }
      return index;
    }

    /**@brief Updates the hash map from unique id to index.

     @sa uniqueIdToIndex()
     */
    void
    updateUniqueIdToIndex() const
    {
      Size num_valid_unique_id = 0;
      // add or update unique id of existing features
      for (Size index = 0; index < getBase_().size(); ++index)
      {
        UInt64 unique_id = getBase_()[index].getUniqueId();
        if (UniqueIdInterface::isValid(unique_id))
        {
          uniqueid_to_index_[unique_id] = index;
          ++num_valid_unique_id;
        }
      }
      // remove invalid or outdated entries
      uniqueid_to_index_.erase(UniqueIdInterface::INVALID);
      for (UniqueIdMap::iterator iter = uniqueid_to_index_.begin(); iter != uniqueid_to_index_.end(); /* see loop */)
      {
        if (iter->second >= getBase_().size() || getBase_()[iter->second].getUniqueId() != iter->first)
        {
          iter = uniqueid_to_index_.erase(iter);
        }
        else
        {
          ++iter;
        }
      }
      if (uniqueid_to_index_.size() != num_valid_unique_id)
      {
        std::stringstream ss;
        ss << "Duplicate valid unique ids detected!   RandomAccessContainer has size()==" << getBase_().size();
        ss << ", num_valid_unique_id==" << num_valid_unique_id;
        ss << ", uniqueid_to_index_.size()==" << uniqueid_to_index_.size();
        throw Exception::Postcondition(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, ss.str());
      }
      return;
    }

    /**
              @brief Assign new UID's to doubly occurring UID's

              Assign new UID's to non-unique UID's. This usually occurs in merging of
              'old' feature files, which have sequentially increasing UID's.
              Conflicting entries receive a new UID, such that all UID's are unique in the container.

              @note Subordinate features are not checked and may remain non-unique. However,
              they are associated to their parent which makes identification 'unique'.

              @return The number of unique ids that were drawn to break a conflict. This counts
                      draws, not elements: an element that needed two draws counts twice, and an
                      element which simply had no valid id yet is given one without being counted.

              @throw Exception::Postcondition if a single element does not obtain a free unique id
                     within (number of ids in use + 64) draws. A generator that does not repeat
                     itself can collide with each id in use at most once, so this bound holds
                     even for a seeded generator whose ids overlap a map written under the same
                     seed; it only ends a generator that keeps returning the same ids, which
                     would otherwise hang the caller.

    */
    Size
    resolveUniqueIdConflicts()
    {
      Size invalid_uids(0);
      uniqueid_to_index_.clear();
      // add unique id of existing features
      for (Size index = 0; index < getBase_().size(); ++index)
      {
        UInt64 unique_id = getBase_()[index].getUniqueId();
        if (!UniqueIdInterface::isValid(unique_id))
        {
          getBase_()[index].ensureUniqueId();
          unique_id = getBase_()[index].getUniqueId();
        }

        // see if UID already present
        // Bound the redraws: the loop below has no other exit, so a generator that keeps producing
        // ids already in use would hang a merge. A fixed small cap is not enough -- TOPP -test runs
        // seed the generator, so merging two maps written under that seed collides once per id of
        // the overlapping block before a free id comes up.
        const Size max_redraws = uniqueid_to_index_.size() + 64;
        Size redraws(0);
        while (uniqueid_to_index_.contains(unique_id)) // double entry!
        {
          if (redraws == max_redraws)
          {
            std::stringstream ss;
            ss << "Could not assign a unique id that is not already in use to the element at index " << index
               << " after " << max_redraws << " attempts.";
            throw Exception::Postcondition(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, ss.str());
          }
          getBase_()[index].setUniqueId();
          unique_id = getBase_()[index].getUniqueId();
          ++redraws;
          ++invalid_uids;
        }

        uniqueid_to_index_[unique_id] = index;

      }

      return invalid_uids;
    }

    /**@brief Swap.
     *
     *  @note obviously we can swap only with indices for the same type.
     */
    void
    swap(UniqueIdIndexer & rhs)
    {
      std::swap(uniqueid_to_index_, rhs.uniqueid_to_index_);
      return;
    }

protected:

    /**@brief A little helper to get access to the base (!) class RandomAccessContainer.
     */
    const auto& getBase_() const
    {
      const T& derived = static_cast<const T&>(*this);  // using CRTP
      return derived.getData();
    }

    /**@brief A little helper to get access to the base (!) class RandomAccessContainer.
     */
    auto& getBase_()
    {
      T& derived = static_cast<T&>(*this);  // using CRTP
      return derived.getData();
    }

    /**@brief hash map from unique id to index of features
     *
     * This is mutable because the hash map is updated on demand, even if the underlying container is const.
     */
    mutable UniqueIdMap uniqueid_to_index_;

  };

} //namespace OpenMS

