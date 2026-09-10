// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Justin Sing $
// $Authors: Justin Sing $
// --------------------------------------------------------------------------

#include <OpenMS/ANALYSIS/OPENSWATH/OpenSwathResultsExporter.h>

#include <OpenMS/CONCEPT/Exception.h>
#include <OpenMS/FORMAT/ArrowIOHelpers.h>
#include <OpenMS/FORMAT/ParquetFile.h>

#include <arrow/api.h>

#include <fstream>

namespace OpenMS
{
  namespace
  {
    std::string optionalToString_(const std::optional<double>& value)
    {
      return value.has_value() ? StringUtils::toStr(*value) : std::string();
    }

    std::string optionalToString_(const std::optional<Int64>& value)
    {
      return value.has_value() ? StringUtils::toStr(*value) : std::string();
    }

    void appendOptionalDouble_(arrow::DoubleBuilder& builder, const std::optional<double>& value, const char* column)
    {
      if (value.has_value())
      {
        ParquetFile::appendOrThrow(builder.Append(*value), column);
      }
      else
      {
        ParquetFile::appendOrThrow(builder.AppendNull(), column);
      }
    }

    void appendOptionalInt64_(arrow::Int64Builder& builder, const std::optional<Int64>& value, const char* column)
    {
      if (value.has_value())
      {
        ParquetFile::appendOrThrow(builder.Append(*value), column);
      }
      else
      {
        ParquetFile::appendOrThrow(builder.AppendNull(), column);
      }
    }

    void appendString_(arrow::StringBuilder& builder, const std::string& value, const char* column)
    {
      if (value.empty())
      {
        ParquetFile::appendOrThrow(builder.AppendNull(), column);
      }
      else
      {
        ParquetFile::appendOrThrow(builder.Append(value.c_str()), column);
      }
    }
  } // namespace

  void OpenSwathResultsExporter::write(const std::string& filename,
                                       const std::vector<OpenSwathExportRow>& rows,
                                       const OpenSwathResultsExportConfig& config)
  {
    if (config.format == OpenSwathExportFileFormat::TSV)
    {
      std::ofstream os(filename.c_str());
      if (!os)
      {
        throw Exception::FileNotWritable(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, filename);
      }

      os << "run_id\tfilename\trun_name\tfeature_id\tpeptide_id\ttransition_group_id\tprecursor_id\tdecoy\tSequence\tFullPeptideName\tProteinName\tGeneName\tCharge\tmz\tRT\tassay_rt\tdelta_rt\tiRT\tassay_iRT\tdelta_iRT\tIntensity\taggr_prec_Peak_Area\taggr_prec_Peak_Apex\tleftWidth\trightWidth\tEXP_IM\tIM_leftWidth\tIM_rightWidth\tms1_pep\tms2_pep\tprecursor_pep\tipf_pep\tpeak_group_rank\td_score\tm_score\tpep\tms2_m_score\tm_score_peptide_run_specific\tm_score_peptide_experiment_wide\tm_score_peptide_global\tm_score_protein_run_specific\tm_score_protein_experiment_wide\tm_score_protein_global\tm_score_gene_run_specific\tm_score_gene_experiment_wide\tm_score_gene_global\talignment_group_id\talignment_reference_feature_id\talignment_reference_rt\talignment_pep\talignment_qvalue\tfrom_alignment\taggr_Peak_Area\taggr_Peak_Apex\taggr_Fragment_Annotation\tipf_FullUniModPeptideName\tipf_precursor_peakgroup_pep\tipf_peptidoform_pep\tipf_peptidoform_m_score\n";
      for (const auto& row : rows)
      {
        os << row.run_id << '\t'
           << row.filename << '\t'
           << row.run_name << '\t'
           << row.feature_id << '\t'
           << row.peptide_id << '\t'
           << row.transition_group_id << '\t'
           << row.precursor_id << '\t'
           << (row.decoy ? 1 : 0) << '\t'
           << row.sequence << '\t'
           << row.full_peptide_name << '\t'
           << row.protein_name << '\t'
           << row.gene_name << '\t'
           << row.charge << '\t'
           << row.mz << '\t'
           << row.rt << '\t'
           << row.assay_rt << '\t'
           << row.delta_rt << '\t'
           << row.irt << '\t'
           << row.assay_irt << '\t'
           << row.delta_irt << '\t'
           << row.intensity << '\t'
           << optionalToString_(row.aggr_prec_peak_area) << '\t'
           << optionalToString_(row.aggr_prec_peak_apex) << '\t'
           << row.left_width << '\t'
           << row.right_width << '\t'
           << optionalToString_(row.exp_im) << '\t'
           << optionalToString_(row.im_left_width) << '\t'
           << optionalToString_(row.im_right_width) << '\t'
           << optionalToString_(row.ms1_pep) << '\t'
           << optionalToString_(row.ms2_pep) << '\t'
           << optionalToString_(row.precursor_pep) << '\t'
           << optionalToString_(row.ipf_pep) << '\t'
           << row.peak_group_rank << '\t'
           << row.d_score << '\t'
           << row.m_score << '\t'
           << optionalToString_(row.pep) << '\t'
           << optionalToString_(row.ms2_m_score) << '\t'
           << optionalToString_(row.peptide_run_specific_qvalue) << '\t'
           << optionalToString_(row.peptide_experiment_wide_qvalue) << '\t'
           << optionalToString_(row.peptide_global_qvalue) << '\t'
           << optionalToString_(row.protein_run_specific_qvalue) << '\t'
           << optionalToString_(row.protein_experiment_wide_qvalue) << '\t'
           << optionalToString_(row.protein_global_qvalue) << '\t'
           << optionalToString_(row.gene_run_specific_qvalue) << '\t'
           << optionalToString_(row.gene_experiment_wide_qvalue) << '\t'
           << optionalToString_(row.gene_global_qvalue) << '\t'
           << optionalToString_(row.alignment_group_id) << '\t'
           << optionalToString_(row.alignment_reference_feature_id) << '\t'
           << optionalToString_(row.alignment_reference_rt) << '\t'
           << optionalToString_(row.alignment_pep) << '\t'
           << optionalToString_(row.alignment_qvalue) << '\t'
           << (row.from_alignment ? 1 : 0) << '\t'
           << row.aggr_peak_area << '\t'
           << row.aggr_peak_apex << '\t'
           << row.aggr_fragment_annotation << '\t'
           << row.ipf_full_peptide_name << '\t'
           << optionalToString_(row.ipf_precursor_peakgroup_pep) << '\t'
           << optionalToString_(row.ipf_peptidoform_pep) << '\t'
           << optionalToString_(row.ipf_peptidoform_m_score) << '\n';
      }
      return;
    }

    arrow::Int64Builder run_id_b;
    arrow::StringBuilder filename_b, run_name_b, tg_b, seq_b, fullpep_b, prot_b, gene_b;
    arrow::Int64Builder feature_id_b, peptide_id_b, precursor_id_b, charge_b;
    arrow::BooleanBuilder decoy_b, from_alignment_b;
    arrow::DoubleBuilder mz_b, rt_b, assay_rt_b, delta_rt_b, irt_b, assay_irt_b, delta_irt_b, intensity_b;
    arrow::DoubleBuilder left_width_b, right_width_b;
    arrow::DoubleBuilder aggr_prec_area_b, aggr_prec_apex_b, exp_im_b, im_left_b, im_right_b;
    arrow::DoubleBuilder ms1_pep_b, ms2_pep_b, precursor_pep_b, ipf_pep_b, d_score_b, m_score_b, pep_b, ms2_m_score_b;
    arrow::Int64Builder peak_group_rank_b, alignment_group_id_b, alignment_reference_feature_id_b;
    arrow::DoubleBuilder pep_rs_b, pep_ew_b, pep_global_b, prot_rs_b, prot_ew_b, prot_global_b, gene_rs_b, gene_ew_b, gene_global_b;
    arrow::DoubleBuilder alignment_reference_rt_b, alignment_pep_b, alignment_qvalue_b;
    arrow::StringBuilder aggr_peak_area_b, aggr_peak_apex_b, aggr_fragment_b, ipf_fullpep_b;
    arrow::DoubleBuilder ipf_precursor_peakgroup_pep_b, ipf_peptidoform_pep_b, ipf_peptidoform_m_score_b;

    for (const auto& row : rows)
    {
      ParquetFile::appendOrThrow(run_id_b.Append(row.run_id), "run_id");
      appendString_(filename_b, row.filename, "filename");
      appendString_(run_name_b, row.run_name, "run_name");
      ParquetFile::appendOrThrow(feature_id_b.Append(row.feature_id), "feature_id");
      ParquetFile::appendOrThrow(peptide_id_b.Append(row.peptide_id), "peptide_id");
      appendString_(tg_b, row.transition_group_id, "transition_group_id");
      ParquetFile::appendOrThrow(precursor_id_b.Append(row.precursor_id), "precursor_id");
      ParquetFile::appendOrThrow(decoy_b.Append(row.decoy), "decoy");
      appendString_(seq_b, row.sequence, "Sequence");
      appendString_(fullpep_b, row.full_peptide_name, "FullPeptideName");
      appendString_(prot_b, row.protein_name, "ProteinName");
      appendString_(gene_b, row.gene_name, "GeneName");
      ParquetFile::appendOrThrow(charge_b.Append(row.charge), "Charge");
      ParquetFile::appendOrThrow(mz_b.Append(row.mz), "mz");
      ParquetFile::appendOrThrow(rt_b.Append(row.rt), "RT");
      ParquetFile::appendOrThrow(assay_rt_b.Append(row.assay_rt), "assay_rt");
      ParquetFile::appendOrThrow(delta_rt_b.Append(row.delta_rt), "delta_rt");
      ParquetFile::appendOrThrow(irt_b.Append(row.irt), "iRT");
      ParquetFile::appendOrThrow(assay_irt_b.Append(row.assay_irt), "assay_iRT");
      ParquetFile::appendOrThrow(delta_irt_b.Append(row.delta_irt), "delta_iRT");
      ParquetFile::appendOrThrow(intensity_b.Append(row.intensity), "Intensity");
      appendOptionalDouble_(aggr_prec_area_b, row.aggr_prec_peak_area, "aggr_prec_Peak_Area");
      appendOptionalDouble_(aggr_prec_apex_b, row.aggr_prec_peak_apex, "aggr_prec_Peak_Apex");
      ParquetFile::appendOrThrow(left_width_b.Append(row.left_width), "leftWidth");
      ParquetFile::appendOrThrow(right_width_b.Append(row.right_width), "rightWidth");
      appendOptionalDouble_(exp_im_b, row.exp_im, "EXP_IM");
      appendOptionalDouble_(im_left_b, row.im_left_width, "IM_leftWidth");
      appendOptionalDouble_(im_right_b, row.im_right_width, "IM_rightWidth");
      appendOptionalDouble_(ms1_pep_b, row.ms1_pep, "ms1_pep");
      appendOptionalDouble_(ms2_pep_b, row.ms2_pep, "ms2_pep");
      appendOptionalDouble_(precursor_pep_b, row.precursor_pep, "precursor_pep");
      appendOptionalDouble_(ipf_pep_b, row.ipf_pep, "ipf_pep");
      ParquetFile::appendOrThrow(peak_group_rank_b.Append(row.peak_group_rank), "peak_group_rank");
      ParquetFile::appendOrThrow(d_score_b.Append(row.d_score), "d_score");
      ParquetFile::appendOrThrow(m_score_b.Append(row.m_score), "m_score");
      appendOptionalDouble_(pep_b, row.pep, "pep");
      appendOptionalDouble_(ms2_m_score_b, row.ms2_m_score, "ms2_m_score");
      appendOptionalDouble_(pep_rs_b, row.peptide_run_specific_qvalue, "m_score_peptide_run_specific");
      appendOptionalDouble_(pep_ew_b, row.peptide_experiment_wide_qvalue, "m_score_peptide_experiment_wide");
      appendOptionalDouble_(pep_global_b, row.peptide_global_qvalue, "m_score_peptide_global");
      appendOptionalDouble_(prot_rs_b, row.protein_run_specific_qvalue, "m_score_protein_run_specific");
      appendOptionalDouble_(prot_ew_b, row.protein_experiment_wide_qvalue, "m_score_protein_experiment_wide");
      appendOptionalDouble_(prot_global_b, row.protein_global_qvalue, "m_score_protein_global");
      appendOptionalDouble_(gene_rs_b, row.gene_run_specific_qvalue, "m_score_gene_run_specific");
      appendOptionalDouble_(gene_ew_b, row.gene_experiment_wide_qvalue, "m_score_gene_experiment_wide");
      appendOptionalDouble_(gene_global_b, row.gene_global_qvalue, "m_score_gene_global");
      appendOptionalInt64_(alignment_group_id_b, row.alignment_group_id, "alignment_group_id");
      appendOptionalInt64_(alignment_reference_feature_id_b, row.alignment_reference_feature_id, "alignment_reference_feature_id");
      appendOptionalDouble_(alignment_reference_rt_b, row.alignment_reference_rt, "alignment_reference_rt");
      appendOptionalDouble_(alignment_pep_b, row.alignment_pep, "alignment_pep");
      appendOptionalDouble_(alignment_qvalue_b, row.alignment_qvalue, "alignment_qvalue");
      ParquetFile::appendOrThrow(from_alignment_b.Append(row.from_alignment), "from_alignment");
      appendString_(aggr_peak_area_b, row.aggr_peak_area, "aggr_Peak_Area");
      appendString_(aggr_peak_apex_b, row.aggr_peak_apex, "aggr_Peak_Apex");
      appendString_(aggr_fragment_b, row.aggr_fragment_annotation, "aggr_Fragment_Annotation");
      appendString_(ipf_fullpep_b, row.ipf_full_peptide_name, "ipf_FullUniModPeptideName");
      appendOptionalDouble_(ipf_precursor_peakgroup_pep_b, row.ipf_precursor_peakgroup_pep, "ipf_precursor_peakgroup_pep");
      appendOptionalDouble_(ipf_peptidoform_pep_b, row.ipf_peptidoform_pep, "ipf_peptidoform_pep");
      appendOptionalDouble_(ipf_peptidoform_m_score_b, row.ipf_peptidoform_m_score, "ipf_peptidoform_m_score");
    }

    auto schema = arrow::schema({
      arrow::field("run_id", arrow::int64()),
      arrow::field("filename", arrow::utf8()),
      arrow::field("run_name", arrow::utf8()),
      arrow::field("feature_id", arrow::int64()),
      arrow::field("peptide_id", arrow::int64()),
      arrow::field("transition_group_id", arrow::utf8()),
      arrow::field("precursor_id", arrow::int64()),
      arrow::field("decoy", arrow::boolean()),
      arrow::field("Sequence", arrow::utf8()),
      arrow::field("FullPeptideName", arrow::utf8()),
      arrow::field("ProteinName", arrow::utf8()),
      arrow::field("GeneName", arrow::utf8()),
      arrow::field("Charge", arrow::int64()),
      arrow::field("mz", arrow::float64()),
      arrow::field("RT", arrow::float64()),
      arrow::field("assay_rt", arrow::float64()),
      arrow::field("delta_rt", arrow::float64()),
      arrow::field("iRT", arrow::float64()),
      arrow::field("assay_iRT", arrow::float64()),
      arrow::field("delta_iRT", arrow::float64()),
      arrow::field("Intensity", arrow::float64()),
      arrow::field("aggr_prec_Peak_Area", arrow::float64()),
      arrow::field("aggr_prec_Peak_Apex", arrow::float64()),
      arrow::field("leftWidth", arrow::float64()),
      arrow::field("rightWidth", arrow::float64()),
      arrow::field("EXP_IM", arrow::float64()),
      arrow::field("IM_leftWidth", arrow::float64()),
      arrow::field("IM_rightWidth", arrow::float64()),
      arrow::field("ms1_pep", arrow::float64()),
      arrow::field("ms2_pep", arrow::float64()),
      arrow::field("precursor_pep", arrow::float64()),
      arrow::field("ipf_pep", arrow::float64()),
      arrow::field("peak_group_rank", arrow::int64()),
      arrow::field("d_score", arrow::float64()),
      arrow::field("m_score", arrow::float64()),
      arrow::field("pep", arrow::float64()),
      arrow::field("ms2_m_score", arrow::float64()),
      arrow::field("m_score_peptide_run_specific", arrow::float64()),
      arrow::field("m_score_peptide_experiment_wide", arrow::float64()),
      arrow::field("m_score_peptide_global", arrow::float64()),
      arrow::field("m_score_protein_run_specific", arrow::float64()),
      arrow::field("m_score_protein_experiment_wide", arrow::float64()),
      arrow::field("m_score_protein_global", arrow::float64()),
      arrow::field("m_score_gene_run_specific", arrow::float64()),
      arrow::field("m_score_gene_experiment_wide", arrow::float64()),
      arrow::field("m_score_gene_global", arrow::float64()),
      arrow::field("alignment_group_id", arrow::int64()),
      arrow::field("alignment_reference_feature_id", arrow::int64()),
      arrow::field("alignment_reference_rt", arrow::float64()),
      arrow::field("alignment_pep", arrow::float64()),
      arrow::field("alignment_qvalue", arrow::float64()),
      arrow::field("from_alignment", arrow::boolean()),
      arrow::field("aggr_Peak_Area", arrow::utf8()),
      arrow::field("aggr_Peak_Apex", arrow::utf8()),
      arrow::field("aggr_Fragment_Annotation", arrow::utf8()),
      arrow::field("ipf_FullUniModPeptideName", arrow::utf8()),
      arrow::field("ipf_precursor_peakgroup_pep", arrow::float64()),
      arrow::field("ipf_peptidoform_pep", arrow::float64()),
      arrow::field("ipf_peptidoform_m_score", arrow::float64())
    });

    auto table = arrow::Table::Make(schema, {
      ParquetFile::finishArray(run_id_b, "run_id"),
      ParquetFile::finishArray(filename_b, "filename"),
      ParquetFile::finishArray(run_name_b, "run_name"),
      ParquetFile::finishArray(feature_id_b, "feature_id"),
      ParquetFile::finishArray(peptide_id_b, "peptide_id"),
      ParquetFile::finishArray(tg_b, "transition_group_id"),
      ParquetFile::finishArray(precursor_id_b, "precursor_id"),
      ParquetFile::finishArray(decoy_b, "decoy"),
      ParquetFile::finishArray(seq_b, "Sequence"),
      ParquetFile::finishArray(fullpep_b, "FullPeptideName"),
      ParquetFile::finishArray(prot_b, "ProteinName"),
      ParquetFile::finishArray(gene_b, "GeneName"),
      ParquetFile::finishArray(charge_b, "Charge"),
      ParquetFile::finishArray(mz_b, "mz"),
      ParquetFile::finishArray(rt_b, "RT"),
      ParquetFile::finishArray(assay_rt_b, "assay_rt"),
      ParquetFile::finishArray(delta_rt_b, "delta_rt"),
      ParquetFile::finishArray(irt_b, "iRT"),
      ParquetFile::finishArray(assay_irt_b, "assay_iRT"),
      ParquetFile::finishArray(delta_irt_b, "delta_iRT"),
      ParquetFile::finishArray(intensity_b, "Intensity"),
      ParquetFile::finishArray(aggr_prec_area_b, "aggr_prec_Peak_Area"),
      ParquetFile::finishArray(aggr_prec_apex_b, "aggr_prec_Peak_Apex"),
      ParquetFile::finishArray(left_width_b, "leftWidth"),
      ParquetFile::finishArray(right_width_b, "rightWidth"),
      ParquetFile::finishArray(exp_im_b, "EXP_IM"),
      ParquetFile::finishArray(im_left_b, "IM_leftWidth"),
      ParquetFile::finishArray(im_right_b, "IM_rightWidth"),
      ParquetFile::finishArray(ms1_pep_b, "ms1_pep"),
      ParquetFile::finishArray(ms2_pep_b, "ms2_pep"),
      ParquetFile::finishArray(precursor_pep_b, "precursor_pep"),
      ParquetFile::finishArray(ipf_pep_b, "ipf_pep"),
      ParquetFile::finishArray(peak_group_rank_b, "peak_group_rank"),
      ParquetFile::finishArray(d_score_b, "d_score"),
      ParquetFile::finishArray(m_score_b, "m_score"),
      ParquetFile::finishArray(pep_b, "pep"),
      ParquetFile::finishArray(ms2_m_score_b, "ms2_m_score"),
      ParquetFile::finishArray(pep_rs_b, "m_score_peptide_run_specific"),
      ParquetFile::finishArray(pep_ew_b, "m_score_peptide_experiment_wide"),
      ParquetFile::finishArray(pep_global_b, "m_score_peptide_global"),
      ParquetFile::finishArray(prot_rs_b, "m_score_protein_run_specific"),
      ParquetFile::finishArray(prot_ew_b, "m_score_protein_experiment_wide"),
      ParquetFile::finishArray(prot_global_b, "m_score_protein_global"),
      ParquetFile::finishArray(gene_rs_b, "m_score_gene_run_specific"),
      ParquetFile::finishArray(gene_ew_b, "m_score_gene_experiment_wide"),
      ParquetFile::finishArray(gene_global_b, "m_score_gene_global"),
      ParquetFile::finishArray(alignment_group_id_b, "alignment_group_id"),
      ParquetFile::finishArray(alignment_reference_feature_id_b, "alignment_reference_feature_id"),
      ParquetFile::finishArray(alignment_reference_rt_b, "alignment_reference_rt"),
      ParquetFile::finishArray(alignment_pep_b, "alignment_pep"),
      ParquetFile::finishArray(alignment_qvalue_b, "alignment_qvalue"),
      ParquetFile::finishArray(from_alignment_b, "from_alignment"),
      ParquetFile::finishArray(aggr_peak_area_b, "aggr_Peak_Area"),
      ParquetFile::finishArray(aggr_peak_apex_b, "aggr_Peak_Apex"),
      ParquetFile::finishArray(aggr_fragment_b, "aggr_Fragment_Annotation"),
      ParquetFile::finishArray(ipf_fullpep_b, "ipf_FullUniModPeptideName"),
      ParquetFile::finishArray(ipf_precursor_peakgroup_pep_b, "ipf_precursor_peakgroup_pep"),
      ParquetFile::finishArray(ipf_peptidoform_pep_b, "ipf_peptidoform_pep"),
      ParquetFile::finishArray(ipf_peptidoform_m_score_b, "ipf_peptidoform_m_score")
    });

    if (!ArrowIOHelpers::writeTableToParquet(table, filename))
    {
      throw Exception::FileNotWritable(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, filename);
    }
  }
} // namespace OpenMS
