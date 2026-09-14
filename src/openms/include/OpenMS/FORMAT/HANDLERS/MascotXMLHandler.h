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
        @brief An open \<peptide\>, \<u_peptide\> or \<q_peptide\> element

        Each open element collects its own hit and evidence, so if a (malformed) file nests these elements, the pep_*
        elements of the inner one do not change the hit of the outer one. Valid files do not nest them.
      */
      struct OpenPeptideElement
      {
        std::string element; ///< the element name, e.g. "u_peptide" (used in error messages)
        Size query = 0; ///< the 1-based 'query' attribute, which was within \<NumQueries\> when the element opened
        PeptideHit hit; ///< filled by the pep_* elements inside this element, stored when it closes
        PeptideEvidence evidence; ///< filled by \<pep_res_before\> and \<pep_res_after\> inside this element, stored at \</peptide\>
      };

      /**
        @brief Checks that a 1-based query number read from the file refers to one of the \<NumQueries\> entries of id_data_

        @param number The query number
        @param element The element @p number was read from, e.g. "peptide" (used in the error message)
        @param attribute The attribute @p number was read from, e.g. "query" (used in the error message)

        @exception Exception::ParseError if @p number is not positive, if no \<NumQueries\> header was read, or if @p number exceeds \<NumQueries\>
      */
      void checkQueryNumber_(Int number, const std::string& element, const char* attribute) const;

      /**
        @brief Returns the innermost open \<peptide\>, \<u_peptide\> or \<q_peptide\> element

        @exception Exception::ParseError if no such element is open
      */
      OpenPeptideElement& openPeptide_();

      /**
        @brief Returns the identification that the innermost open \<peptide\>, \<u_peptide\> or \<q_peptide\> element refers to

        Its 'query' attribute was checked against \<NumQueries\> when the element opened. It is compared with the size
        of id_data_ again at every call, so that it is never used as an index past the end of id_data_.

        @exception Exception::ParseError if no such element is open, or if its query number exceeds the size of id_data_
      */
      PeptideIdentification& peptideIdentification_();

      /**
        @brief Returns the identification of the innermost open \<query number="..."\> element

        @exception Exception::ParseError if no \<query\> element is open, if no \<NumQueries\> header was read, or if the query number exceeds \<NumQueries\>
      */
      PeptideIdentification& queryIdentification_();

      /**
        @brief Warns that the element that just closed (tag_, with the text in character_buffer_) is ignored

        Used for elements that were found where they cannot be used, e.g. outside of the element they belong to.
        Valid files never contain such elements.

        @param reason Why the element is ignored, e.g. "which is not inside a <query> element" (used in the warning)
      */
      void warnIgnoredElement_(const std::string& reason) const;

      ProteinIdentification& protein_identification_; ///< the protein identifications
      PeptideIdentificationList& id_data_; ///< the identifications (storing the peptide hits)
      ProteinHit actual_protein_hit_;
      /// the open \<peptide\>, \<u_peptide\> and \<q_peptide\> elements, innermost last; empty outside of these elements
      std::vector<OpenPeptideElement> open_peptides_;
      std::string tag_;
      DateTime date_;
      std::string date_time_string_;
      /// numbers (1-based, positive) of the open \<query\> elements, innermost last; empty outside of \<query\> elements.
      /// \<StringTitle\> and \<RTINSECONDS\> update the query of the innermost one.
      std::vector<Int> open_query_numbers_;
      bool num_queries_read_; ///< id_data_ was sized from \<NumQueries\>; a repeated \<NumQueries\> is ignored
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

