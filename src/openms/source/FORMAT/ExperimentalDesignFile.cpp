// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg, Lukas Zimmermann $
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/LogStream.h>
#include <OpenMS/DATASTRUCTURES/ListUtils.h>
#include <OpenMS/FORMAT/ExperimentalDesignFile.h>
#include <OpenMS/FORMAT/TextFile.h>
#include <OpenMS/KERNEL/StandardTypes.h>
#include <OpenMS/METADATA/ExperimentalDesign.h>
#include <OpenMS/METADATA/ProteinIdentification.h>
#include <OpenMS/SYSTEM/File.h>
#include <OpenMS/SYSTEM/PathUtils.h>
#include <filesystem>
#include <iostream>

using namespace std;

namespace OpenMS
{
    std::string findSpectraFile(const std::string &spec_file, const std::string &tsv_file, const bool require_spectra_files)
    {
      std::string result;
      namespace fs = std::filesystem;
      // On Windows, std::filesystem treats "/data/foo" as relative (no drive letter),
      // but Qt treated it as absolute. Check for '/' prefix to match Qt behavior.
      bool is_relative = to_path(spec_file).is_relative();
#ifdef OPENMS_WINDOWSPLATFORM
      if (!spec_file.empty() && spec_file[0] == '/') is_relative = false;
#endif
      if (is_relative)
      {
        // file name is relative, so we need to figure out the correct folder

        // first check folder relative to folder of design file
        // to allow, for example, a design in ./design.tsv and spectra in ./spectra/a.mzML
        // where ./ is the same folder
        std::string design_file_dir = fs::absolute(to_path(tsv_file)).parent_path().generic_string();
        std::string design_file_relative = design_file_dir + "/" + spec_file;

        if (File::exists(design_file_relative))
        {
          result = design_file_relative;
        }
        else
        {
          // check current folder
          std::string f = File::absolutePath(spec_file);
          if (File::exists(f))
          {
            result = f;
          }
        }

        // if result still empty, just use the provided value
        if (result.empty())
        {
          result = spec_file;
        }
      }
      else
      {
        // set to absolute path
        result = spec_file;
      }

      // Fail if the existence of the spectra files is required but they do not exist
      if (require_spectra_files && !File::exists(result))
      {
        throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, tsv_file,
                                    "Error: Spectra file does not exist: '" + result + "'");
      }
      return result;
    }

    // Parse Error of filename if test holds
    void ExperimentalDesignFile::parseErrorIf_(const bool test, const std::string &filename, const std::string &message)
    {
      if (test)
      {
        throw Exception::ParseError(
          __FILE__,
          __LINE__,
          OPENMS_PRETTY_FUNCTION,
          filename,
          "Error: " + message);
      }
    }

    void ExperimentalDesignFile::parseHeader_(
      const StringList &header,
      const std::string &filename,
      std::map <std::string, Size> &column_map,
      const std::set <std::string> &required,
      const std::set <std::string> &optional,
      const bool allow_other_header)
    {
      // Headers as set
      std::set <std::string> header_set(header.begin(), header.end());
      parseErrorIf_(header_set.size() != header.size(), filename, "Some column headers of the table appear multiple times!");

      // Check that all required headers are there
      for (const std::string &req_header : required)
      {
        parseErrorIf_(!ListUtils::contains(header, req_header), filename, "Missing column header: " + req_header);
      }
      // Assign index in column map and check for weird headers
      for (Size i = 0; i < header.size(); ++i)
      {
        const std::string &h = header[i];

        // A header is unexpected if it is neither required nor optional and we do not allow other headers
        const bool header_unexpected = (!required.contains(h)) && (!optional.contains(h));
        parseErrorIf_(!allow_other_header && header_unexpected, filename, "Header not allowed in this section of the Experimental Design: " + h);
        column_map[h] = i;
      }
    }

    bool ExperimentalDesignFile::isOneTableFile_(const TextFile& text_file)
    {
      // To determine if we have a *separate* sample table, we check if a row is present
      // that contains "Sample" but no "Fraction_Group".
      for (std::string s : text_file)
      {
        const std::string line(StringUtils::trim(s));
        if (line.empty()) { continue; }
        StringList cells;
        StringUtils::split(line, "\t", cells);
        // check if we are outside of run section (no Fraction_Group column) 
        // but in sample section (Sample column present)
        if (std::count(cells.begin(), cells.end(), "Fraction_Group") == 0 
         && std::count(cells.begin(), cells.end(), "Sample") == 1)
        {
          return false;
        }
      }
      return true;
    }

    ExperimentalDesign ExperimentalDesignFile::parseOneTableFile_(const TextFile& text_file, const std::string& tsv_file, bool require_spectra_file)
    {
      ExperimentalDesign::MSFileSection msfile_section;
      bool has_sample(false);
      bool has_label(false);

      /// Content of the sample section (vector of rows with column contents as strings)
      std::vector< std::vector < std::string > > sample_content_;
      /// Sample name to sample (row) index
      std::map< unsigned, Size > sample_sample_to_rowindex_;
      /// Inside each row, the value for a column can be accessed by name with this map
      std::map< std::string, Size > sample_columnname_to_columnindex_;

      /// Maps the column header string to the column index for
      /// the file section
      std::map <std::string, Size> fs_column_header_to_index;

      /// Maps the sample name to all values of sample-related columns
      std::map <std::string, std::vector<std::string>> sample_content_map;

      /// Maps the sample name to its index (i.e., the order how it gets added to the sample section)
      std::map<std::string, Size> samplename_to_index;
      enum ParseState { RUN_HEADER, RUN_CONTENT };

      ParseState state(RUN_HEADER);
      Size n_col = 0;

      for (std::string s : text_file)
      {
        const std::string line(StringUtils::trim(s));
	
        if (StringUtils::hasPrefix(line, "#") || line.empty()) { continue; }

        // Now split the line into individual cells
        StringList cells;
        StringUtils::split(line, "\t", cells);

        // Trim whitespace from all cells (so , foo , and  ,foo, is the same)
        std::transform(cells.begin(), cells.end(), cells.begin(),
                       [](std::string cell) -> std::string { return StringUtils::trim(cell); });

        if (state == RUN_HEADER)
        {
          state = RUN_CONTENT;
          parseHeader_(
            cells,
            tsv_file,
            fs_column_header_to_index,
            {"Fraction_Group", "Fraction", "Spectra_Filepath"},
            {"Label", "Sample"}, true
          );
          has_label = fs_column_header_to_index.contains("Label");
          has_sample = fs_column_header_to_index.contains("Sample");

          // Width of a content row as written, i.e. without the Label/Sample columns appended below:
          // RUN_CONTENT checks each row against it before it reads a cell or appends those columns.
          n_col = cells.size();

          if (!has_label) // add label column to end of header
          {
            size_t hs = fs_column_header_to_index.size();
            fs_column_header_to_index["Label"] = hs;
            cells.push_back("Label");
          }

          if (!has_sample) // add sample column to end of header
          {
            size_t hs = fs_column_header_to_index.size();
            fs_column_header_to_index["Sample"] = hs;
            cells.push_back("Sample");
          }

          // determine columns with sample metainfo like condition or replication
          for (size_t i = 0; i != cells.size(); ++i)
          {
            const std::string& c = cells[i];
            if (c != "Fraction_Group" && c != "Fraction" 
             && c != "Spectra_Filepath" && c != "Label")
            { 
              sample_columnname_to_columnindex_[c] = i;
            }
          }
        }
        else if (state == RUN_CONTENT)
        {
          // Check the width before any cell is read: the lookups below index the row unchecked,
          // so a row shorter than the header would otherwise be read past its end.
          parseErrorIf_(n_col != cells.size(), tsv_file, "Wrong number of records in line");

          // if no label column exists -> label free
          // -> add label column with label 1 at the end of every row
          if (!has_label) { cells.push_back("1"); }

          // Assign label
          int label = StringUtils::toInt32(cells[fs_column_header_to_index["Label"]]);
          int fraction = StringUtils::toInt32(cells[fs_column_header_to_index["Fraction"]]);
          int fraction_group = StringUtils::toInt32(cells[fs_column_header_to_index["Fraction_Group"]]);
          // All three are stored in unsigned members, where a negative value would wrap to ~4e9
          // instead of failing. Reject it before the signed 'Label > 1' check, which -1 would pass.
          parseErrorIf_(label < 0, tsv_file, "Label must not be negative, found " + StringUtils::toStr(label));
          parseErrorIf_(fraction < 0, tsv_file, "Fraction must not be negative, found " + StringUtils::toStr(fraction));
          parseErrorIf_(fraction_group < 0, tsv_file, "Fraction_Group must not be negative, found " + StringUtils::toStr(fraction_group));
          parseErrorIf_(!has_sample && (label > 1), tsv_file,
                        "Column 'Sample' is required for multiplexed one-table designs (Label > 1).");

          // read sample column
          Size sample = 1;
          std::string samplename;
          if (!has_sample) 
          {
            samplename = StringUtils::toStr(fraction_group); // deducing the sample in the case of multiplexed could be done if label > 1 information is there (e.g., max(label) * (fraction_group - 1) + label
            cells.push_back(samplename);
          }

          samplename = cells[fs_column_header_to_index["Sample"]];

          const auto& [it, inserted] = samplename_to_index.emplace(samplename, samplename_to_index.size());
          sample = it->second;

          // get indices of sample metadata and store content in sample cells
          StringList sample_cells;
          for (const auto & c2i : sample_columnname_to_columnindex_)
          {
            sample_cells.push_back(cells[c2i.second]);
          }

          if (inserted)
          {
            sample_content_.push_back(sample_cells);
          }
          else
          {
            if (sample_content_[it->second] != sample_cells)
            {
              OPENMS_LOG_WARN << "Warning: Factors for the same sample do not match." << std::endl;
            }
          }

          ExperimentalDesign::MSFileSectionEntry e;

          // Assign fraction group and fraction
          e.fraction_group = fraction_group;
          e.fraction = fraction;
          e.label = label;
          e.sample = sample;
          e.sample_name = samplename;

          // Spectra files
          e.path = findSpectraFile(
            cells[fs_column_header_to_index["Spectra_Filepath"]],
            tsv_file,
            require_spectra_file);
       
          msfile_section.push_back(e);
        }
      }

      // Assign correct position for columns in sample section subset
      // (i.e., without "Fraction_Group", "Fraction", "Spectra_Filepath", "Label")
      int sample_index = 0;
      for (auto & c : sample_columnname_to_columnindex_)
      {
        c.second = sample_index++;
      }

      // Create Sample Section and set in design
      ExperimentalDesign::SampleSection sample_section(
        sample_content_,
        samplename_to_index,
        sample_columnname_to_columnindex_);

      // Create experimentalDesign
      ExperimentalDesign design(msfile_section, sample_section);

      return design;
    }

    ExperimentalDesign ExperimentalDesignFile::parseTwoTableFile_(const TextFile& text_file, const std::string& tsv_file, bool require_spectra_file)
    {
      ExperimentalDesign::MSFileSection msfile_section;
      bool has_sample(false);
      bool has_label(false);

      // Attributes of the sample section
      std::vector< std::vector < std::string > > sample_content_;
      std::map< std::string, Size > sample_sample_to_rowindex_;
      std::map< std::string, Size > sample_columnname_to_columnindex_;

      // Maps the column header string to the column index for
      // the file section
      std::map <std::string, Size> fs_column_header_to_index;

      unsigned line_number(0);

      enum ParseState { RUN_HEADER, RUN_CONTENT, SAMPLE_HEADER, SAMPLE_CONTENT };

      ParseState state(RUN_HEADER);
      Size n_col = 0;
      // Blank cells in front of the first sample header name. A sample table exported with an empty first
      // column has them in the header and in every row; they are dropped from both, so the columns line up.
      Size sample_lead = 0;

      for (const std::string& raw_line : text_file)
      {
        // skip empty lines (except in state RUN_CONTENT, where the sample table is read)
        const std::string line = StringUtils::trimmed(raw_line);
	      // also skip comment lines
        if (StringUtils::hasPrefix(line, "#") || (line.empty() && state != RUN_CONTENT))
        {
          continue;
        }

        // Now split the line into individual cells
        StringList cells;
        // Split the sample header and its rows before trimming them: trimming the line would also drop a blank
        // first or last cell (its tab is whitespace), and every later value would then move into the wrong
        // column. Both are split the same way, so they stay aligned. Cells are trimmed below.
        const bool sample_table = state == SAMPLE_HEADER || state == SAMPLE_CONTENT;
        StringUtils::split(sample_table ? raw_line : line, "\t", cells);

        // Trim whitespace from all cells (so , foo , and  ,foo, is the same)
        std::transform(cells.begin(), cells.end(), cells.begin(),
                       [](std::string cell) -> std::string { return StringUtils::trim(cell); });

        if (state == RUN_HEADER)
        {
          state = RUN_CONTENT;
          parseHeader_(
            cells,
            tsv_file,
            fs_column_header_to_index,
            {"Fraction_Group", "Fraction", "Spectra_Filepath"},
            {"Label", "Sample"}, false
          );
          has_label = fs_column_header_to_index.contains("Label");
          has_sample = fs_column_header_to_index.contains("Sample");
          
          n_col = fs_column_header_to_index.size();
        }
        // End of file section lines, empty line separates file and sample section
        else if (state == RUN_CONTENT && line.empty())
        {
          // Next line is header of Sample table
          state = SAMPLE_HEADER;
        }
        // Line is file section line
        else if (state == RUN_CONTENT)
        {
          parseErrorIf_(n_col != cells.size(), tsv_file, "Wrong number of records in line");

          ExperimentalDesign::MSFileSectionEntry e;

          // Read through signed ints first: the members are unsigned, so a negative value would
          // wrap to ~4e9 and then count as a real fraction group, fraction or label.
          const int fraction_group = StringUtils::toInt32(cells[fs_column_header_to_index["Fraction_Group"]]);
          const int fraction = StringUtils::toInt32(cells[fs_column_header_to_index["Fraction"]]);
          const int label = has_label ? StringUtils::toInt32(cells[fs_column_header_to_index["Label"]]) : 1;
          parseErrorIf_(fraction_group < 0, tsv_file, "Fraction_Group must not be negative, found " + StringUtils::toStr(fraction_group));
          parseErrorIf_(fraction < 0, tsv_file, "Fraction must not be negative, found " + StringUtils::toStr(fraction));
          parseErrorIf_(label < 0, tsv_file, "Label must not be negative, found " + StringUtils::toStr(label));

          // Assign fraction group and fraction
          e.fraction_group = fraction_group;
          e.fraction = fraction;

          // Assign label
          e.label = label;

          // Assign sample number
          if (has_sample)
          {
            //e.sample has to be filled after the sample section was read
            e.sample_name = cells[fs_column_header_to_index["Sample"]];
          }
          else
          {
            e.sample = e.fraction_group; // TODO: deducing the sample in the case of multiplexed
                                         //  could be done if label > 1 information is there
                                         //  (e.g., max(label) * (fraction_group - 1) + label
            e.sample_name = "Fraction group " + StringUtils::toStr(e.fraction_group);
          }

          // Spectra files
          e.path = findSpectraFile(
            cells[fs_column_header_to_index["Spectra_Filepath"]],
            tsv_file,
            require_spectra_file);
          msfile_section.push_back(e);
        }
        // Parse header of the Condition Table
        else if (state == SAMPLE_HEADER)
        {
          state = SAMPLE_CONTENT;
          line_number = 0;
          // Only named columns count: drop the blank cells before the first name and after the last one.
          while (sample_lead < cells.size() && cells[sample_lead].empty())
          {
            ++sample_lead;
          }
          cells.erase(cells.begin(), cells.begin() + sample_lead);
          while (!cells.empty() && cells.back().empty())
          {
            cells.pop_back();
          }
          parseHeader_(
            cells,
            tsv_file,
            sample_columnname_to_columnindex_,
            {"Sample"}, {}, true
          );
          n_col = sample_columnname_to_columnindex_.size();
        }
        // Parse Sample Row
        else if (state == SAMPLE_CONTENT)
        {
          // Blank cells keep their column (see the split above), but a row may still lack trailing cells, e.g. when
          // an editor drops the tabs after a blank last value. Pad it with empty values and ignore cells beyond
          // the header, so every stored row has exactly one value per column and getFactorValue() stays inside it.
          // Only the sample name itself must be present.
          const Size sample_column = sample_columnname_to_columnindex_.at("Sample");
          const auto lead_end = cells.begin() + std::min(sample_lead, cells.size());
          const auto non_empty = [](const std::string& cell) { return !cell.empty(); };
          // A value outside the header's columns is ignored, but it may mean that the row is shifted (e.g. by a
          // stray tab), so its other values could be in the wrong columns as well.
          if (std::any_of(cells.begin(), lead_end, non_empty) ||
              (cells.size() > sample_lead + n_col && std::any_of(cells.begin() + sample_lead + n_col, cells.end(), non_empty)))
          {
            OPENMS_LOG_WARN << "Warning: row " << line_number + 1 << " of the sample table in '" << tsv_file
                            << "' has values outside the columns of its header. They are ignored; check that its other "
                               "values are in the right columns." << std::endl;
          }
          cells.erase(cells.begin(), lead_end);
          cells.resize(n_col);
          parseErrorIf_(cells[sample_column].empty(), tsv_file, "Missing sample name in a row of the sample table");

          // Parse Error if sample appears multiple times
          const std::string& sample = cells[sample_column];
          parseErrorIf_(sample_sample_to_rowindex_.contains(sample),
                        tsv_file,
                        "Sample: " + std::string(sample) + " appears multiple times in the sample table");
          sample_sample_to_rowindex_[sample] = line_number++;
          sample_content_.push_back(cells);
        }
      }

      for (auto& e : msfile_section)
      {
        // Every sample the file section uses needs a row in the sample section. Name a missing one, instead of
        // letting the lookup throw a bare std::out_of_range that does not say which sample it was.
        const auto row = sample_sample_to_rowindex_.find(e.sample_name);
        parseErrorIf_(row == sample_sample_to_rowindex_.end(), tsv_file,
                      "Sample '" + e.sample_name + "' of the MS file section is missing from the sample section" +
                      (has_sample ? "" : " (the file section has no Sample column, so it names each sample after its fraction group)"));
        e.sample = row->second;
      }

      // Create Sample Section and set in design
      ExperimentalDesign::SampleSection sample_section(
        sample_content_,
        sample_sample_to_rowindex_,
        sample_columnname_to_columnindex_);

      // Create experimentalDesign
      ExperimentalDesign design(msfile_section, sample_section);

      return design;
    }

    // static
    ExperimentalDesign ExperimentalDesignFile::load(const TextFile &text_file, const bool require_spectra_file, std::string filename = "--no design file provided--")
    {
      // check if we have information stored in one or two files
      bool has_one_table = isOneTableFile_(text_file);

      if (has_one_table)
      {
        return parseOneTableFile_(text_file, filename, require_spectra_file);
      }
      else // two tables
      {
        return parseTwoTableFile_(text_file, filename, require_spectra_file);
      }
    }

    ExperimentalDesign ExperimentalDesignFile::load(const std::string &tsv_file, const bool require_spectra_file)
    {
      // Lines are not trimmed here: the parsers trim them, except the sample header and rows, which need their tabs.
      const TextFile text_file(tsv_file);
      return load(text_file, require_spectra_file, tsv_file);
    }

}
