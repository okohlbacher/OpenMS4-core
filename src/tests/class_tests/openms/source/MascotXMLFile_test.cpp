// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Nico Pfeifer, Chris Bielow $
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/TestFileValidation.h>
#include <OpenMS/test_config.h>
#include <OpenMS/CONCEPT/FuzzyStringComparator.h>
#include <OpenMS/DATASTRUCTURES/ListUtils.h>


///////////////////////////

#include <OpenMS/DATASTRUCTURES/StringUtils.h>
#include <OpenMS/FORMAT/MascotXMLFile.h>
#include <OpenMS/FORMAT/HANDLERS/MascotXMLHandler.h>
#include <OpenMS/FORMAT/IdXMLFile.h>
#include <OpenMS/METADATA/ContactPerson.h>
#include <OpenMS/METADATA/ProteinIdentification.h>
#include <OpenMS/METADATA/PeptideIdentification.h>

#include <OpenMS/CONCEPT/LogStream.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string_view>
#include <vector>

///////////////////////////

namespace
{
  // writes a minimal Mascot XML file whose <mascot_search_results> element contains @p body
  void writeMascotXML(const std::string& path, const std::string& body)
  {
    std::ofstream out(path);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<mascot_search_results majorVersion=\"2\" minorVersion=\"1\">\n"
        << body
        << "</mascot_search_results>\n";
  }

  // the what() of the ParseError that XMLHandler::fatalError() throws for @p message while loading @p path
  std::string loadError(const std::string& path, const std::string& message)
  {
    return "While loading '" + path + "': " + message + " in: " + path;
  }

  const std::string show_header_hint = "(make sure to use the 'show_header=1' option in the ./export_dat.pl script)";

  // one hit for query @p query with an m/z and a sequence
  std::string peptideHit(const std::string& query)
  {
    return "<hits><hit number=\"1\"><protein accession=\"P1\"><peptide query=\"" + query + "\">"
           "<pep_exp_mz>500.0</pep_exp_mz><pep_seq>PEPTIDE</pep_seq>"
           "</peptide></protein></hit></hits>\n";
  }

  // loads @p path and returns the warnings about ignored elements that were logged meanwhile, one per line
  std::string loadCollectingIgnoredWarnings(const std::string& path, OpenMS::ProteinIdentification& proteins,
                                            OpenMS::PeptideIdentificationList& peptides, const OpenMS::SpectrumMetaDataLookup& lookup)
  {
    // LogStream writes a line only once while it is among the last few distinct lines, and reports the repetitions later
    // in a line '<...> occurred N times'. Flush that cache before and after the load, so that no line of an earlier load
    // suppresses or is reported with a line of this one.
    OPENMS_LOG_WARN->clearCache();
    std::ostringstream captured;
    OPENMS_LOG_WARN.insert(captured);
    try
    {
      OpenMS::MascotXMLFile().load(path, proteins, peptides, lookup);
    }
    catch (...)
    {
      OPENMS_LOG_WARN.remove(captured);
      OPENMS_LOG_WARN->clearCache();
      throw;
    }
    OPENMS_LOG_WARN.remove(captured);
    OPENMS_LOG_WARN->clearCache();

    std::string ignored;
    std::istringstream lines(captured.str());
    for (std::string line; std::getline(lines, line); )
    {
      const std::string::size_type start = line.find("While loading '");
      // skip the '<While loading ...> occurred N times' summaries of repeated lines (the first occurrence was collected)
      const bool repetition_summary = start != std::string::npos && start > 0 && line[start - 1] == '<' &&
                                      line.find("> occurred ", start) != std::string::npos;
      if (start != std::string::npos && !repetition_summary && line.find("Ignoring <", start) != std::string::npos)
      {
        const std::string::size_type end = line.find('\x1b', start); // the warning colour ends with an escape sequence
        ignored += line.substr(start, end == std::string::npos ? std::string::npos : end - start) + "\n";
      }
    }
    return ignored;
  }

  // the warning line for @p element (with its text), ignored for @p reason while loading @p path
  std::string ignoredWarning(const std::string& path, const std::string& element, const std::string& reason)
  {
    return "While loading '" + path + "': Ignoring " + element + ", " + reason + ".\n";
  }

  const std::string outside_peptide = "which is not inside a <peptide>, <u_peptide> or <q_peptide> element";
  const std::string outside_query = "which is not inside a <query> element";
  const std::string repeated_num_queries = "which repeats an earlier <NumQueries> element";

  // a MascotXMLHandler that replaces the identifications it fills by an empty list when a <drop_identifications/>
  // element closes, to check that the handler never relies on the list still holding the queries it checked before
  class DroppingMascotXMLHandler : public OpenMS::Internal::MascotXMLHandler
  {
  public:
    DroppingMascotXMLHandler(OpenMS::ProteinIdentification& proteins, OpenMS::PeptideIdentificationList& peptides,
                             const std::string& filename, std::map<std::string, std::vector<OpenMS::AASequence>>& modified_peptides,
                             const OpenMS::SpectrumMetaDataLookup& lookup) :
      MascotXMLHandler(proteins, peptides, filename, modified_peptides, lookup),
      peptides_(peptides)
    {
    }

    void onEndElement(const char16_t* qname) override
    {
      MascotXMLHandler::onEndElement(qname);
      if (std::u16string_view(qname) == u"drop_identifications")
      {
        OpenMS::PeptideIdentificationList().swap(peptides_); // also frees the storage, so no element is left past the end
      }
    }

  private:
    OpenMS::PeptideIdentificationList& peptides_;
  };

  // gives access to XMLFile::parse_(), to parse a file with a handler of the test's choice
  class ParsingMascotXMLFile : public OpenMS::MascotXMLFile
  {
  public:
    void parse(const std::string& filename, OpenMS::Internal::XMLHandler& handler)
    {
      parse_(filename, &handler);
    }
  };
}

START_TEST(MascotXMLFile, "$Id$")

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

using namespace OpenMS;
using namespace std;

MascotXMLFile xml_file;
MascotXMLFile* ptr;
ProteinIdentification protein_identification;
PeptideIdentificationList peptide_identifications;
PeptideIdentificationList peptide_identifications2;
DateTime date;
PeptideHit peptide_hit;
vector<std::string> references;

date.set("2006-03-09 11:31:52");

MascotXMLFile* nullPointer = nullptr;
START_SECTION((MascotXMLFile()))
  ptr = new MascotXMLFile();
  TEST_NOT_EQUAL(ptr, nullPointer)
  delete ptr;
END_SECTION

START_SECTION((static void initializeLookup(SpectrumMetaDataLookup& lookup, PeakMap& experiment, const std::string& scan_regex = "")))
{
  PeakMap exp;
  exp.getSpectra().resize(1);
  SpectrumMetaDataLookup lookup;
  xml_file.initializeLookup(lookup, exp);
  TEST_EQUAL(lookup.empty(), false);
}
END_SECTION

START_SECTION((void load(const std::string& filename, ProteinIdentification& protein_identification, PeptideIdentificationList& id_data, SpectrumMetaDataLookup& lookup)))
{
  SpectrumMetaDataLookup lookup;
  xml_file.load(OPENMS_GET_TEST_DATA_PATH("MascotXMLFile_test_1.mascotXML"),
                protein_identification, peptide_identifications, lookup);

  {
    ProteinIdentification::SearchParameters search_parameters = protein_identification.getSearchParameters();
    TEST_EQUAL(search_parameters.missed_cleavages, 1);
    TEST_EQUAL(search_parameters.taxonomy, ". . Eukaryota (eucaryotes)");
    TEST_EQUAL(search_parameters.mass_type, ProteinIdentification::PeakMassType::AVERAGE);
    TEST_EQUAL(search_parameters.db, "MSDB_chordata");
    TEST_EQUAL(search_parameters.db_version, "MSDB_chordata_20070910.fasta");
    TEST_EQUAL(search_parameters.fragment_mass_tolerance, 0.2);
    TEST_EQUAL(search_parameters.precursor_mass_tolerance, 1.4);
    TEST_EQUAL(search_parameters.fragment_mass_tolerance_ppm, false);
    TEST_EQUAL(search_parameters.precursor_mass_tolerance_ppm, false);
    TEST_EQUAL(search_parameters.charges, "1+, 2+ and 3+");
    TEST_EQUAL(search_parameters.fixed_modifications.size(), 4);
    TEST_EQUAL(search_parameters.fixed_modifications[0], "Carboxymethyl (C)");
    TEST_EQUAL(search_parameters.fixed_modifications[1], "Deamidated (N)");
    TEST_EQUAL(search_parameters.fixed_modifications[2], "Deamidated (Q)");
    TEST_EQUAL(search_parameters.fixed_modifications[3], "Guanidinyl (K)");
    TEST_EQUAL(search_parameters.variable_modifications.size(), 3);
    TEST_EQUAL(search_parameters.variable_modifications[0], "Acetyl (Protein N-term)");
    TEST_EQUAL(search_parameters.variable_modifications[1], "Biotin (K)");
    TEST_EQUAL(search_parameters.variable_modifications[2], "Carbamyl (K)");
    TEST_EQUAL(peptide_identifications.size(), 3);
    TOLERANCE_ABSOLUTE(0.0001);
    TEST_REAL_SIMILAR(peptide_identifications[0].getMZ(), 789.83);
    TEST_REAL_SIMILAR(peptide_identifications[1].getMZ(), 135.29);
    TEST_REAL_SIMILAR(peptide_identifications[2].getMZ(), 982.58);
    TOLERANCE_ABSOLUTE(0.00001);
    TEST_EQUAL(protein_identification.getHits().size(), 2);
    TEST_EQUAL(protein_identification.getHits()[0].getAccession(), "AAN17824");
    TEST_EQUAL(protein_identification.getHits()[1].getAccession(), "GN1736");
    TEST_REAL_SIMILAR(protein_identification.getHits()[0].getScore(), 619);
    TEST_REAL_SIMILAR(protein_identification.getHits()[1].getScore(), 293);
    TEST_EQUAL(protein_identification.getScoreType(), "Mascot");
    TEST_EQUAL(protein_identification.getDateTime().get(), "2006-03-09 11:31:52");

    TEST_REAL_SIMILAR(peptide_identifications[0].getSignificanceThreshold(), 31.8621);
    TEST_EQUAL(peptide_identifications[0].getHits().size(), 2);

    peptide_hit = peptide_identifications[0].getHits()[0];
    set<std::string> ref_set = peptide_hit.extractProteinAccessionsSet();
    vector<std::string> references(ref_set.begin(), ref_set.end());
    TEST_EQUAL(references.size(), 2);
    TEST_EQUAL(references[0], "AAN17824");
    TEST_EQUAL(references[1], "GN1736");
    peptide_hit = peptide_identifications[0].getHits()[1];
    ref_set = peptide_hit.extractProteinAccessionsSet();
    references = vector<std::string>(ref_set.begin(), ref_set.end());
    TEST_EQUAL(references.size(), 1);
    TEST_EQUAL(references[0], "AAN17824");
    peptide_hit = peptide_identifications[1].getHits()[0];
    ref_set = peptide_hit.extractProteinAccessionsSet();
    references = vector<std::string>(ref_set.begin(), ref_set.end());
    TEST_EQUAL(references.size(), 1);
    TEST_EQUAL(references[0], "GN1736");

    TEST_EQUAL(peptide_identifications[1].getHits().size(), 1);
    TEST_REAL_SIMILAR(peptide_identifications[0].getHits()[0].getScore(), 33.85);
    TEST_REAL_SIMILAR(peptide_identifications[0].getHits()[1].getScore(), 33.12);
    TEST_REAL_SIMILAR(peptide_identifications[1].getHits()[0].getScore(), 43.9);
    TEST_EQUAL(peptide_identifications[0].getScoreType(), "Mascot");
    TEST_EQUAL(peptide_identifications[1].getScoreType(), "Mascot");
    TEST_EQUAL(protein_identification.getDateTime() == date, true);
    TEST_EQUAL(peptide_identifications[0].getHits()[0].getSequence(), AASequence::fromString("LHASGITVTEIPVTATN(MOD:00565)FK(MOD:00445)"));
    TEST_EQUAL(peptide_identifications[0].getHits()[1].getSequence(), AASequence::fromString("MRSLGYVAVISAVATDTDK(MOD:00445)"));
    TEST_EQUAL(peptide_identifications[1].getHits()[0].getSequence(), AASequence::fromString("HSK(MOD:00445)LSAK(MOD:00445)"));

    std::string identifier = protein_identification.getIdentifier();
    TEST_EQUAL(!identifier.empty(), true);
    for (Size i = 0; i < peptide_identifications.size(); ++i)
    {
      TEST_EQUAL(identifier, peptide_identifications[i].getIdentifier())
    }
  }


  /// for new MascotXML 2.1 as used by Mascot Server 2.3
  xml_file.load(OPENMS_GET_TEST_DATA_PATH("MascotXMLFile_test_2.mascotXML"),
                protein_identification, peptide_identifications, lookup);
  {
    ProteinIdentification::SearchParameters search_parameters = protein_identification.getSearchParameters();
    TEST_EQUAL(search_parameters.missed_cleavages, 7);
    TEST_EQUAL(search_parameters.taxonomy, "All entries");
    TEST_EQUAL(search_parameters.mass_type, ProteinIdentification::PeakMassType::MONOISOTOPIC);
    TEST_EQUAL(search_parameters.db, "IPI_human");
    TEST_EQUAL(search_parameters.db_version, "ipi.HUMAN.v3.61.fasta");
    TEST_EQUAL(search_parameters.fragment_mass_tolerance, 0.3);
    TEST_EQUAL(search_parameters.precursor_mass_tolerance, 3);
    TEST_EQUAL(search_parameters.fragment_mass_tolerance_ppm, false);
    TEST_EQUAL(search_parameters.precursor_mass_tolerance_ppm, false);
    TEST_EQUAL(search_parameters.charges, "");
    TEST_EQUAL(search_parameters.fixed_modifications.size(), 1);
    TEST_EQUAL(search_parameters.fixed_modifications[0], "Carbamidomethyl (C)");
    TEST_EQUAL(search_parameters.variable_modifications.size(), 3);
    TEST_EQUAL(search_parameters.variable_modifications[0], "Oxidation (M)");
    TEST_EQUAL(search_parameters.variable_modifications[1], "Acetyl (N-term)");
    TEST_EQUAL(search_parameters.variable_modifications[2], "Phospho (Y)");
  // not necessarily equal to numQueries as some hits might not be contained, e.g. peptide's might start with <peptide rank="10"...> so 9 peptides are missing
  // thus empty peptides are removed (see MascotXMLFile.cpp::load() ) after the handler() call
    TEST_EQUAL(peptide_identifications.size(), 1112);
    TOLERANCE_ABSOLUTE(0.0001);
    TEST_REAL_SIMILAR(peptide_identifications[0].getMZ(), 304.6967);
    TEST_REAL_SIMILAR(peptide_identifications[1].getMZ(), 314.1815);
    TEST_REAL_SIMILAR(peptide_identifications[1111].getMZ(), 583.7948);
    TOLERANCE_ABSOLUTE(0.00001);
    TEST_EQUAL(protein_identification.getHits().size(), 66);
    TEST_EQUAL(protein_identification.getHits()[0].getAccession(), "IPI00745872");
    TEST_EQUAL(protein_identification.getHits()[1].getAccession(), "IPI00908876");
    TEST_REAL_SIMILAR(protein_identification.getHits()[0].getScore(), 122);
    TEST_REAL_SIMILAR(protein_identification.getHits()[1].getScore(), 122);
    TEST_EQUAL(protein_identification.getScoreType(), "Mascot");
    TEST_EQUAL(protein_identification.getDateTime().get(), "2011-06-24 19:34:54");

    TEST_REAL_SIMILAR(peptide_identifications[0].getSignificanceThreshold(), 5);
    TEST_EQUAL(peptide_identifications[0].getHits().size(), 1);

    peptide_hit = peptide_identifications[0].getHits()[0];
    vector<PeptideEvidence> pes = peptide_hit.getPeptideEvidences();
    TEST_EQUAL(pes.size(), 0);
    pes = peptide_identifications[34].getHits()[0].getPeptideEvidences();
    set<std::string> accessions = peptide_identifications[34].getHits()[0].extractProteinAccessionsSet();
    references = vector<std::string>(accessions.begin(), accessions.end()); // corresponds to <peptide query="35" ...>
    ABORT_IF(references.size() != 5);
    TEST_EQUAL(references[0], "IPI00022434");
    TEST_EQUAL(references[1], "IPI00384697");
    TEST_EQUAL(references[2], "IPI00745872");
    TEST_EQUAL(references[3], "IPI00878517");
    TEST_EQUAL(references[4], "IPI00908876");

    TEST_REAL_SIMILAR(peptide_identifications[0].getHits()[0].getScore(), 5.34);
    TEST_REAL_SIMILAR(peptide_identifications[49].getHits()[0].getScore(), 14.83);
    TEST_REAL_SIMILAR(peptide_identifications[49].getHits()[1].getScore(), 17.5);
    TEST_EQUAL(peptide_identifications[0].getScoreType(), "Mascot");
    TEST_EQUAL(peptide_identifications[1].getScoreType(), "Mascot");
    TEST_EQUAL(protein_identification.getDateTime().get() == "2011-06-24 19:34:54", true);
    TEST_EQUAL(peptide_identifications[0].getHits()[0].getSequence(), AASequence::fromString("VVFIK"));
    TEST_EQUAL(peptide_identifications[49].getHits()[0].getSequence(), AASequence::fromString("LASYLDK"));
    TEST_EQUAL(peptide_identifications[49].getHits()[1].getSequence(), AASequence::fromString("(Acetyl)AAFESDK"));
  //for (int i=520;i<540;++i) std::cerr << "i: " << i << " " << peptide_identifications[i].getHits()[0].getSequence() << "\n";
    TEST_EQUAL(peptide_identifications[522].getHits()[0].getSequence(), AASequence::fromString("(Acetyl)GALM(Oxidation)NEIQAAK"));
    TEST_EQUAL(peptide_identifications[67].getHits()[0].getSequence(), AASequence::fromString("SHY(Phospho)GGSR"));

    std::string identifier = protein_identification.getIdentifier();
    TEST_EQUAL(!identifier.empty(), true);
    for (Size i = 0; i < peptide_identifications.size(); ++i)
    {
      TEST_EQUAL(identifier, peptide_identifications[i].getIdentifier())
    }
  }

  xml_file.load(OPENMS_GET_TEST_DATA_PATH("MascotXMLFile_test_3.mascotXML"),
                protein_identification, peptide_identifications, lookup);
  {
    std::vector<ProteinIdentification> pids;
    pids.push_back(protein_identification);
    std::string filename;
    NEW_TMP_FILE(filename)
    IdXMLFile().store(filename, pids, peptide_identifications);
    FuzzyStringComparator fuzzy;
    fuzzy.setWhitelist(ListUtils::create<std::string>("<?xml-stylesheet"));
    fuzzy.setAcceptableAbsolute(0.0001);
    bool result = fuzzy.compareFiles(filename, OPENMS_GET_TEST_DATA_PATH("MascotXMLFile_test_out_3.idXML"));
    TEST_EQUAL(result, true);
  }
}
END_SECTION

START_SECTION((void load(const std::string& filename, ProteinIdentification& protein_identification, PeptideIdentificationList& id_data, std::map<std::string, std::vector<AASequence> >& peptides, SpectrumMetaDataLookup& lookup)))
  std::map<std::string, vector<AASequence> > modified_peptides;
  AASequence aa_sequence_1;
  AASequence aa_sequence_2;
  AASequence aa_sequence_3;
  vector<AASequence> temp;

  aa_sequence_1 = AASequence::fromString("LHASGITVTEIPVTATNFK");
  aa_sequence_1.setModification(16, "Deamidated");
  aa_sequence_2 = AASequence::fromString("MRSLGYVAVISAVATDTDK");
  aa_sequence_2.setModification(2, "Phospho");
  aa_sequence_3 = AASequence::fromString("HSKLSAK");
  aa_sequence_3.setModification(4, "Phospho");
  temp.push_back(aa_sequence_1);
  temp.push_back(aa_sequence_2);
  modified_peptides.insert(make_pair("789.83", temp));
  temp.clear();
  temp.push_back(aa_sequence_3);
  modified_peptides.insert(make_pair("135.29", temp));

  SpectrumMetaDataLookup lookup;
  xml_file.load(OPENMS_GET_TEST_DATA_PATH("MascotXMLFile_test_1.mascotXML"),
                protein_identification, peptide_identifications, 
                modified_peptides, lookup);

  TEST_EQUAL(peptide_identifications.size(), 3)
  TOLERANCE_ABSOLUTE(0.0001)
  TEST_REAL_SIMILAR(peptide_identifications[0].getMZ(), 789.83)
  TEST_REAL_SIMILAR(peptide_identifications[1].getMZ(), 135.29)
  TEST_REAL_SIMILAR(peptide_identifications[2].getMZ(), 982.58)
  TOLERANCE_ABSOLUTE(0.00001)
  TEST_EQUAL(protein_identification.getHits().size(), 2)
  TEST_EQUAL(protein_identification.getHits()[0].getAccession(), "AAN17824")
  TEST_EQUAL(protein_identification.getHits()[1].getAccession(), "GN1736")
  TEST_REAL_SIMILAR(protein_identification.getHits()[0].getScore(), 619)
  TEST_REAL_SIMILAR(protein_identification.getHits()[1].getScore(), 293)
  TEST_EQUAL(protein_identification.getScoreType(), "Mascot")
  TEST_EQUAL(protein_identification.getDateTime().get(), "2006-03-09 11:31:52")

  TEST_REAL_SIMILAR(peptide_identifications[0].getSignificanceThreshold(), 31.8621)
  TEST_EQUAL(peptide_identifications[0].getHits().size(), 2)

  peptide_hit = peptide_identifications[0].getHits()[0];  
  set<std::string> accessions = peptide_hit.extractProteinAccessionsSet();
  references = vector<std::string>(accessions.begin(), accessions.end());
  TEST_EQUAL(references.size(), 2)
  TEST_EQUAL(references[0], "AAN17824")
  TEST_EQUAL(references[1], "GN1736")  
  peptide_hit = peptide_identifications[0].getHits()[1];
  accessions = peptide_hit.extractProteinAccessionsSet();
  references = vector<std::string>(accessions.begin(), accessions.end());
  TEST_EQUAL(references.size(), 1)
  TEST_EQUAL(references[0], "AAN17824")
  peptide_hit = peptide_identifications[1].getHits()[0];
  accessions = peptide_hit.extractProteinAccessionsSet();
  references = vector<std::string>(accessions.begin(), accessions.end());
  TEST_EQUAL(references.size(), 1)
  TEST_EQUAL(references[0], "GN1736")

  TEST_EQUAL(peptide_identifications[1].getHits().size(), 1)
  TEST_REAL_SIMILAR(peptide_identifications[0].getHits()[0].getScore(), 33.85)
  TEST_REAL_SIMILAR(peptide_identifications[0].getHits()[1].getScore(), 33.12)
  TEST_REAL_SIMILAR(peptide_identifications[1].getHits()[0].getScore(), 43.9)
  TEST_EQUAL(peptide_identifications[0].getScoreType(), "Mascot")
  TEST_EQUAL(peptide_identifications[1].getScoreType(), "Mascot")
  TEST_EQUAL(protein_identification.getDateTime() == date, true)
  TEST_EQUAL(peptide_identifications[0].getHits()[0].getSequence(), aa_sequence_1)
  TEST_EQUAL(peptide_identifications[0].getHits()[1].getSequence(), aa_sequence_2)
  TEST_EQUAL(peptide_identifications[1].getHits()[0].getSequence(), aa_sequence_3)
END_SECTION

START_SECTION(([EXTRA] query numbers that are not within <NumQueries> are rejected with a ParseError))
{
  // Query numbers are 1-based indices into the list of identifications sized by <NumQueries>. Numbers outside of it
  // used to index outside of that list (heap out-of-bounds reads and writes, or a crash).
  const std::string header = "<header><NumQueries>1</NumQueries></header>\n";
  SpectrumMetaDataLookup lookup;
  std::string filename;

  // control: the minimal file loads
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header + peptideHit("1") +
    "<queries><query number=\"1\"><StringTitle>scan1</StringTitle><RTINSECONDS>12.5</RTINSECONDS></query></queries>\n");
  xml_file.load(filename, protein_identification, peptide_identifications, lookup);
  TEST_EQUAL(peptide_identifications.size(), 1)
  ABORT_IF(peptide_identifications.size() != 1)
  TEST_REAL_SIMILAR(peptide_identifications[0].getMZ(), 500.0)
  TEST_REAL_SIMILAR(peptide_identifications[0].getRT(), 12.5)
  TEST_EQUAL(peptide_identifications[0].getHits().size(), 1)
  TEST_EQUAL(peptide_identifications[0].getHits()[0].getSequence(), AASequence::fromString("PEPTIDE"))

  // an export without header (no <NumQueries>) whose first peptide is query 1
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, peptideHit("1"));
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, xml_file.load(filename, protein_identification, peptide_identifications, lookup),
    loadError(filename, "No or conflicting header information present " + show_header_hint))

  // a peptide query above <NumQueries> (one past the end)
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header + peptideHit("2"));
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, xml_file.load(filename, protein_identification, peptide_identifications, lookup),
    loadError(filename, "<peptide> 'query' attribute '2' exceeds <NumQueries> (1)."))

  // a peptide query of 0
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header + peptideHit("0"));
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, xml_file.load(filename, protein_identification, peptide_identifications, lookup),
    loadError(filename, "Invalid <peptide> 'query' attribute '0': query numbers start at 1."))

  // a <u_peptide> query above <NumQueries>
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header +
    "<unassigned><u_peptide query=\"2\"><pep_exp_mz>500.0</pep_exp_mz><pep_seq>PEPTIDE</pep_seq></u_peptide></unassigned>\n");
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, xml_file.load(filename, protein_identification, peptide_identifications, lookup),
    loadError(filename, "<u_peptide> 'query' attribute '2' exceeds <NumQueries> (1)."))

  // a <q_peptide> query of 0
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header +
    "<queries><query number=\"1\"><q_peptide query=\"0\"><pep_seq>PEPTIDE</pep_seq></q_peptide></query></queries>\n");
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, xml_file.load(filename, protein_identification, peptide_identifications, lookup),
    loadError(filename, "Invalid <q_peptide> 'query' attribute '0': query numbers start at 1."))

  // <query number="0">
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header + "<queries><query number=\"0\"><StringTitle>scan1</StringTitle></query></queries>\n");
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, xml_file.load(filename, protein_identification, peptide_identifications, lookup),
    loadError(filename, "Invalid <query> 'number' attribute '0': query numbers start at 1."))

  // <query number> out of range, used by <StringTitle> and by <RTINSECONDS>
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header + "<queries><query number=\"3\"><StringTitle>scan3</StringTitle></query></queries>\n");
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, xml_file.load(filename, protein_identification, peptide_identifications, lookup),
    loadError(filename, "<query> 'number' attribute '3' exceeds <NumQueries> (1)."))
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header + "<queries><query number=\"3\"><RTINSECONDS>12.5</RTINSECONDS></query></queries>\n");
  TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, xml_file.load(filename, protein_identification, peptide_identifications, lookup),
    loadError(filename, "<query> 'number' attribute '3' exceeds <NumQueries> (1)."))
}
END_SECTION

START_SECTION(([EXTRA] pep_*, <StringTitle> and <RTINSECONDS> elements outside of their peptide or query element are ignored with a warning))
{
  // These elements only describe the enclosing <peptide>, <u_peptide> or <q_peptide> element, or the enclosing <query>.
  // Outside of it they used to update the identification of the first or of the previous such element.
  SpectrumMetaDataLookup lookup;
  lookup.addReferenceFormat("rt=(?<RT>[0-9.]+)"); // lets a <pep_scan_title> set an RT, so that an applied stray title shows
  std::string filename;

  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename,
    "<header><NumQueries>3</NumQueries></header>\n"
    "<hits>\n"
    "<pep_exp_mz>777.0</pep_exp_mz><pep_ident>22.0</pep_ident>\n" // before the first <peptide>
    "<hit number=\"1\"><protein accession=\"P1\">\n"
    "<peptide query=\"1\"><pep_seq>PEPTIDE</pep_seq></peptide>\n"
    "<pep_exp_mz>999.0</pep_exp_mz><pep_homol>33.0</pep_homol><pep_expect>0.01</pep_expect>\n" // after </peptide>
    "</protein></hit>\n"
    "</hits>\n"
    "<unassigned>\n"
    "<u_peptide query=\"2\"><pep_exp_mz>600.0</pep_exp_mz><pep_seq>SAMPLER</pep_seq></u_peptide>\n"
    "<pep_scan_title>rt=77</pep_scan_title>\n" // after </u_peptide>
    "</unassigned>\n"
    "<queries>\n"
    "<query number=\"1\"><StringTitle>scan1</StringTitle><RTINSECONDS>12.5</RTINSECONDS></query>\n"
    "<RTINSECONDS>99</RTINSECONDS>\n" // after </query>
    "<query number=\"3\"><q_peptide query=\"3\"><pep_exp_mz>700.0</pep_exp_mz><pep_seq>ELVISLIVESK</pep_seq></q_peptide>\n"
    "<pep_ident>44.0</pep_ident></query>\n" // after </q_peptide>, still inside <query>
    "<StringTitle>stray_55</StringTitle>\n" // after the last </query>
    "</queries>\n");
  std::string ignored = loadCollectingIgnoredWarnings(filename, protein_identification, peptide_identifications, lookup);
  TEST_EQUAL(ignored,
    ignoredWarning(filename, "<pep_exp_mz>777.0</pep_exp_mz>", outside_peptide) +
    ignoredWarning(filename, "<pep_ident>22.0</pep_ident>", outside_peptide) +
    ignoredWarning(filename, "<pep_exp_mz>999.0</pep_exp_mz>", outside_peptide) +
    ignoredWarning(filename, "<pep_homol>33.0</pep_homol>", outside_peptide) +
    ignoredWarning(filename, "<pep_expect>0.01</pep_expect>", outside_peptide) +
    ignoredWarning(filename, "<pep_scan_title>rt=77</pep_scan_title>", outside_peptide) +
    ignoredWarning(filename, "<RTINSECONDS>99</RTINSECONDS>", outside_query) +
    ignoredWarning(filename, "<pep_ident>44.0</pep_ident>", outside_peptide) +
    ignoredWarning(filename, "<StringTitle>stray_55</StringTitle>", outside_query))
  TEST_EQUAL(peptide_identifications.size(), 3)
  ABORT_IF(peptide_identifications.size() != 3)
  const bool one_hit_each = std::all_of(peptide_identifications.begin(), peptide_identifications.end(),
    [](const PeptideIdentification& id) { return id.getHits().size() == 1; });
  TEST_TRUE(one_hit_each)
  ABORT_IF(!one_hit_each)
  // query 1: no m/z and no threshold of its own; its RT comes from its <query>
  TEST_FALSE(peptide_identifications[0].hasMZ())
  TEST_REAL_SIMILAR(peptide_identifications[0].getSignificanceThreshold(), 0.0)
  TEST_REAL_SIMILAR(peptide_identifications[0].getRT(), 12.5)
  TEST_EQUAL(peptide_identifications[0].getHits()[0].getSequence(), AASequence::fromString("PEPTIDE"))
  TEST_FALSE(peptide_identifications[0].getHits()[0].metaValueExists("identity_threshold"))
  // query 2
  TEST_REAL_SIMILAR(peptide_identifications[1].getMZ(), 600.0)
  TEST_FALSE(peptide_identifications[1].hasRT())
  TEST_EQUAL(peptide_identifications[1].getHits()[0].getSequence(), AASequence::fromString("SAMPLER"))
  TEST_FALSE(peptide_identifications[1].getHits()[0].metaValueExists("EValue"))
  // query 3
  TEST_REAL_SIMILAR(peptide_identifications[2].getMZ(), 700.0)
  TEST_REAL_SIMILAR(peptide_identifications[2].getSignificanceThreshold(), 0.0)
  TEST_FALSE(peptide_identifications[2].hasRT())
  TEST_EQUAL(peptide_identifications[2].getHits()[0].getSequence(), AASequence::fromString("ELVISLIVESK"))

  // an export without header: the elements are ignored before any query number could be checked
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename,
    "<hits><pep_scan_title>rt=5</pep_scan_title><pep_homol>1.0</pep_homol></hits>\n"
    "<queries><StringTitle>scan_7</StringTitle><RTINSECONDS>7</RTINSECONDS></queries>\n");
  ignored = loadCollectingIgnoredWarnings(filename, protein_identification, peptide_identifications, lookup);
  TEST_EQUAL(ignored,
    ignoredWarning(filename, "<pep_scan_title>rt=5</pep_scan_title>", outside_peptide) +
    ignoredWarning(filename, "<pep_homol>1.0</pep_homol>", outside_peptide) +
    ignoredWarning(filename, "<StringTitle>scan_7</StringTitle>", outside_query) +
    ignoredWarning(filename, "<RTINSECONDS>7</RTINSECONDS>", outside_query))
  TEST_EQUAL(peptide_identifications.size(), 0)
}
END_SECTION

START_SECTION(([EXTRA] elements of nested peptide or query elements belong to the innermost open element))
{
  // Valid files do not nest these elements. If a file does, the elements inside the inner one describe the inner
  // one's query and hit only, and after the inner one closes they describe the outer one again. The hit of the outer
  // element used to be shared with the inner one: the inner one overwrote its sequence and score, stored it for its
  // own query and reset it, so the outer query lost its hit.
  SpectrumMetaDataLookup lookup;
  std::string filename;
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename,
    "<header><NumQueries>2</NumQueries></header>\n"
    "<hits><hit number=\"1\"><protein accession=\"P1\"><peptide query=\"1\">"
    "<pep_exp_mz>500.0</pep_exp_mz><pep_seq>PEPTIDE</pep_seq><pep_score>10</pep_score><pep_res_before>K</pep_res_before>"
    "<u_peptide query=\"2\"><pep_exp_mz>600.0</pep_exp_mz><pep_seq>SAMPLER</pep_seq><pep_score>20</pep_score></u_peptide>"
    "<pep_exp_z>2</pep_exp_z>"
    "</peptide></protein></hit></hits>\n"
    "<queries><query number=\"1\"><query number=\"2\"><RTINSECONDS>20</RTINSECONDS></query>"
    "<RTINSECONDS>10</RTINSECONDS></query></queries>\n");
  const std::string ignored = loadCollectingIgnoredWarnings(filename, protein_identification, peptide_identifications, lookup);
  TEST_EQUAL(ignored, "")
  TEST_EQUAL(peptide_identifications.size(), 2)
  ABORT_IF(peptide_identifications.size() != 2)
  const bool one_hit_each = std::all_of(peptide_identifications.begin(), peptide_identifications.end(),
    [](const PeptideIdentification& id) { return id.getHits().size() == 1; });
  TEST_TRUE(one_hit_each)
  ABORT_IF(!one_hit_each)
  TEST_REAL_SIMILAR(peptide_identifications[0].getMZ(), 500.0)
  TEST_REAL_SIMILAR(peptide_identifications[0].getRT(), 10.0)
  const PeptideHit& outer_hit = peptide_identifications[0].getHits()[0];
  TEST_EQUAL(outer_hit.getSequence(), AASequence::fromString("PEPTIDE"))
  TEST_REAL_SIMILAR(outer_hit.getScore(), 10.0)
  TEST_EQUAL(outer_hit.getCharge(), 2)
  TEST_EQUAL(outer_hit.getPeptideEvidences().size(), 1)
  ABORT_IF(outer_hit.getPeptideEvidences().size() != 1)
  TEST_EQUAL(outer_hit.getPeptideEvidences()[0].getAABefore(), 'K')
  TEST_EQUAL(outer_hit.getPeptideEvidences()[0].getProteinAccession(), "P1")
  TEST_REAL_SIMILAR(peptide_identifications[1].getMZ(), 600.0)
  TEST_REAL_SIMILAR(peptide_identifications[1].getRT(), 20.0)
  const PeptideHit& inner_hit = peptide_identifications[1].getHits()[0];
  TEST_EQUAL(inner_hit.getSequence(), AASequence::fromString("SAMPLER"))
  TEST_REAL_SIMILAR(inner_hit.getScore(), 20.0)
  TEST_EQUAL(inner_hit.getCharge(), 0)
}
END_SECTION

START_SECTION(([EXTRA] a repeated <NumQueries> does not shrink the identifications after query numbers were checked))
{
  // The 'query' attribute of <peptide> is checked against <NumQueries> when the element opens. A second, smaller
  // <NumQueries> inside that peptide used to shrink the list, so the following pep_* elements wrote past its end.
  SpectrumMetaDataLookup lookup;
  std::string filename;
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename,
    "<header><NumQueries>2</NumQueries></header>\n"
    "<hits><hit number=\"1\"><protein accession=\"P1\"><peptide query=\"2\">"
    "<NumQueries>1</NumQueries><pep_exp_mz>500.0</pep_exp_mz><pep_seq>PEPTIDE</pep_seq>"
    "</peptide></protein></hit></hits>\n");
  const std::string ignored = loadCollectingIgnoredWarnings(filename, protein_identification, peptide_identifications, lookup);
  TEST_EQUAL(ignored, ignoredWarning(filename, "<NumQueries>1</NumQueries>", repeated_num_queries))
  TEST_EQUAL(peptide_identifications.size(), 1)
  ABORT_IF(peptide_identifications.size() != 1)
  TEST_REAL_SIMILAR(peptide_identifications[0].getMZ(), 500.0)
  TEST_EQUAL(peptide_identifications[0].getHits().size(), 1)
  TEST_EQUAL(peptide_identifications[0].getHits()[0].getSequence(), AASequence::fromString("PEPTIDE"))
}
END_SECTION

START_SECTION(([EXTRA] an ignored element that repeats is warned about once in every load))
{
  // LogStream writes a repeated line once and reports the repetitions later ('<...> occurred N times'). The warnings of
  // each load are checked on their own, whatever an earlier load logged.
  SpectrumMetaDataLookup lookup;
  std::string filename;
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename,
    "<hits><pep_homol>1.0</pep_homol><pep_homol>1.0</pep_homol><pep_homol>1.0</pep_homol>"
    "<pep_exp_mz>2.0</pep_exp_mz><pep_seq>PEPTIDE</pep_seq></hits>\n");
  // the third distinct warning makes LogStream report the repetitions of the first one
  const std::string expected =
    ignoredWarning(filename, "<pep_homol>1.0</pep_homol>", outside_peptide) +
    ignoredWarning(filename, "<pep_exp_mz>2.0</pep_exp_mz>", outside_peptide) +
    ignoredWarning(filename, "<pep_seq>PEPTIDE</pep_seq>", outside_peptide);
  TEST_EQUAL(loadCollectingIgnoredWarnings(filename, protein_identification, peptide_identifications, lookup), expected)
  TEST_EQUAL(loadCollectingIgnoredWarnings(filename, protein_identification, peptide_identifications, lookup), expected)

  // one warning repeated only: loading the file again logs it again
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, "<hits><pep_homol>3.0</pep_homol><pep_homol>3.0</pep_homol></hits>\n");
  const std::string expected_repeated = ignoredWarning(filename, "<pep_homol>3.0</pep_homol>", outside_peptide);
  TEST_EQUAL(loadCollectingIgnoredWarnings(filename, protein_identification, peptide_identifications, lookup), expected_repeated)
  TEST_EQUAL(loadCollectingIgnoredWarnings(filename, protein_identification, peptide_identifications, lookup), expected_repeated)
}
END_SECTION

START_SECTION(([EXTRA] the query of a peptide element is compared with the identifications again at every use))
{
  // The 'query' attribute of a peptide element is checked against <NumQueries> when the element opens. Its later uses
  // must not rely on that check alone: here the identifications are replaced by an empty list after it, which made
  // pep_* elements and the closing tag read and write past the end of the list.
  SpectrumMetaDataLookup lookup;
  std::map<std::string, std::vector<AASequence>> modified;
  ParsingMascotXMLFile parsing_file;
  const std::string header = "<header><NumQueries>2</NumQueries></header>\n";
  std::string filename;

  // control: without <drop_identifications/>, the handler fills query 2
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header + peptideHit("2"));
  {
    ProteinIdentification proteins;
    PeptideIdentificationList ids;
    DroppingMascotXMLHandler handler(proteins, ids, filename, modified, lookup);
    parsing_file.parse(filename, handler);
    TEST_EQUAL(ids.size(), 2)
    ABORT_IF(ids.size() != 2)
    TEST_REAL_SIMILAR(ids[1].getMZ(), 500.0)
    TEST_EQUAL(ids[1].getHits().size(), 1)
  }

  // <pep_exp_mz> in a <peptide>
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header +
    "<hits><hit number=\"1\"><protein accession=\"P1\"><peptide query=\"2\"><drop_identifications/>"
    "<pep_exp_mz>500.0</pep_exp_mz><pep_seq>PEPTIDE</pep_seq></peptide></protein></hit></hits>\n");
  {
    ProteinIdentification proteins;
    PeptideIdentificationList ids;
    DroppingMascotXMLHandler handler(proteins, ids, filename, modified, lookup);
    TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, parsing_file.parse(filename, handler),
      loadError(filename, "The open <peptide> element refers to query 2, but there are only 0 identifications."))
  }

  // <pep_ident> in a <q_peptide>
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header +
    "<queries><query number=\"1\"><q_peptide query=\"2\"><drop_identifications/>"
    "<pep_ident>30.0</pep_ident></q_peptide></query></queries>\n");
  {
    ProteinIdentification proteins;
    PeptideIdentificationList ids;
    DroppingMascotXMLHandler handler(proteins, ids, filename, modified, lookup);
    TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, parsing_file.parse(filename, handler),
      loadError(filename, "The open <q_peptide> element refers to query 2, but there are only 0 identifications."))
  }

  // </peptide>
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header +
    "<hits><hit number=\"1\"><protein accession=\"P1\"><peptide query=\"1\"><pep_seq>PEPTIDE</pep_seq>"
    "<drop_identifications/></peptide></protein></hit></hits>\n");
  {
    ProteinIdentification proteins;
    PeptideIdentificationList ids;
    DroppingMascotXMLHandler handler(proteins, ids, filename, modified, lookup);
    TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, parsing_file.parse(filename, handler),
      loadError(filename, "The open <peptide> element refers to query 1, but there are only 0 identifications."))
  }

  // </u_peptide>
  NEW_TMP_FILE_EXT(filename, ".mascotXML")
  writeMascotXML(filename, header +
    "<unassigned><u_peptide query=\"1\"><pep_seq>PEPTIDE</pep_seq><drop_identifications/></u_peptide></unassigned>\n");
  {
    ProteinIdentification proteins;
    PeptideIdentificationList ids;
    DroppingMascotXMLHandler handler(proteins, ids, filename, modified, lookup);
    TEST_EXCEPTION_WITH_MESSAGE(Exception::ParseError, parsing_file.parse(filename, handler),
      loadError(filename, "The open <u_peptide> element refers to query 1, but there are only 0 identifications."))
  }
}
END_SECTION

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/// check the temporary files written above against their XML schema (types without a validator are skipped)
VALIDATE_TMP_FILES

END_TEST
