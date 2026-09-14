// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Nico Pfeifer, Chris Bielow $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/CHEMISTRY/AASequence.h>
#include <OpenMS/CHEMISTRY/ModificationsDB.h>
#include <OpenMS/FORMAT/HANDLERS/XMLHandler.h>
#include <OpenMS/METADATA/PeptideIdentification.h>
#include <OpenMS/METADATA/PeptideIdentificationList.h>
#include <OpenMS/METADATA/PeptideEvidence.h>
#include <OpenMS/METADATA/ProteinIdentification.h>
#include <OpenMS/METADATA/SpectrumMetaDataLookup.h>

#include <vector>

namespace OpenMS
{
  namespace Internal
  {
    /**
      @brief Handler that is used for parsing MascotXML data
    */
    class OPENMS_DLLAPI MascotXMLHandler :
      public XMLHandler
    {
public:
      /// Constructor
      MascotXMLHandler(ProteinIdentification& protein_identification,
                       PeptideIdentificationList& identifications,
                       const std::string& filename,
                       std::map<std::string, std::vector<AASequence> >& peptides,
                       const SpectrumMetaDataLookup& lookup);

      /// Destructor
      ~MascotXMLHandler() override;

      // Docu in base class
      void onEndElement(const char16_t* qname) override;

      // Docu in base class
      void onStartElement(const char16_t* qname, const XMLAttributes& attributes) override;

      // Docu in base class
      void onCharacters(const char16_t* chars, Size /*length*/) override;
      
      /// Split modification search parameter if for more than one amino acid specified e.g. Phospho (ST)
      static std::vector<std::string> splitModificationBySpecifiedAA(const std::string& mod);

private:

      /**
        @brief Checks that a 1-based query number read from the file refers to one of the <NumQueries> entries of id_data_

        @param number The query number
        @param source Where @p number was read from, e.g. "<peptide> 'query' attribute" (used in the error message)

        @exception Exception::ParseError if @p number is not positive, if no <NumQueries> header was read, or if @p number exceeds <NumQueries>
      */
      void checkQueryNumber_(Int number, const std::string& source) const;

      /**
        @brief Returns the identification that the enclosing <peptide>, <u_peptide> or <q_peptide> element refers to

        The 'query' attribute is checked when that element opens. The index is checked again at every use, because an
        element outside of such a peptide element still sees the initial or a previous index.

        @param element Name of the element being read (used in the error message)

        @exception Exception::ParseError if the index is not within id_data_
      */
      PeptideIdentification& peptideIdentification_(const std::string& element);

      /**
        @brief Returns the identification of the enclosing <query number="..."> element

        @param element Name of the element being read (used in the error message)

        @exception Exception::ParseError if no <query> element was read yet, or if its number is not within the <NumQueries> entries
      */
      PeptideIdentification& queryIdentification_(const std::string& element);

      ProteinIdentification& protein_identification_; ///< the protein identifications
      PeptideIdentificationList& id_data_; ///< the identifications (storing the peptide hits)
      ProteinHit actual_protein_hit_;
      PeptideHit actual_peptide_hit_;
      PeptideEvidence actual_peptide_evidence_;
      UInt peptide_identification_index_;
      std::string tag_;
      DateTime date_;
      std::string date_time_string_;
      UInt actual_query_; ///< number of the current <query> element (1-based); 0 before the first one
      bool num_queries_read_; ///< id_data_ was sized from <NumQueries>; a repeated <NumQueries> is ignored
      ProteinIdentification::SearchParameters search_parameters_;
      std::string identifier_;
      std::string actual_title_;
      std::map<std::string, std::vector<AASequence> >& modified_peptides_;

      StringList tags_open_; ///< tracking the current XML tree
      std::string character_buffer_; ///< filled by MascotXMLHandler::characters
      std::string major_version_;
      std::string minor_version_;
      
      // list of modifications, which cannot be set as fixed and needs
      // to be removed, because added from mascot as variable modification
      std::vector<std::string> remove_fixed_mods_;

      /// Helper object for looking up RT information
      const SpectrumMetaDataLookup& lookup_;

      /// Error for missing RT information already reported?
      bool no_rt_error_;
    };

  } // namespace Internal
} // namespace OpenMS

