#include <arrow/api.h>
#include <arrow/io/file.h>
#include <arrow/io/api.h>
#include <arrow/util/thread_pool.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <parquet/file_reader.h>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#endif

const char* current_stage = "entry";
void stage(const char* name)
{
  current_stage = name;
  if (!std::getenv("PROBE_QUIET")) std::cerr << name << '\n' << std::flush;
}
void check(const arrow::Status& status)
{
  if (!status.ok()) throw std::runtime_error(status.ToString());
}
void trace()
{
#ifdef _WIN32
  HANDLE process = GetCurrentProcess();
  SymInitialize(process, nullptr, TRUE);
  void* frames[64];
  const auto count = CaptureStackBackTrace(0, 64, frames, nullptr);
  alignas(SYMBOL_INFO) char data[sizeof(SYMBOL_INFO) + 1024]{};
  auto* symbol = reinterpret_cast<SYMBOL_INFO*>(data);
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = 1023;
  for (unsigned i = 0; i < count; ++i)
  {
    DWORD64 offset = 0;
    if (SymFromAddr(process, reinterpret_cast<DWORD64>(frames[i]), &offset, symbol))
      std::cerr << frames[i] << ' ' << symbol->Name << '+' << offset << '\n';
    else std::cerr << frames[i] << '\n';
  }
#endif
}
std::shared_ptr<arrow::Table> read(const std::string& file, bool prebuffer)
{
  auto input = arrow::io::ReadableFile::Open(file).ValueOrDie();
  parquet::arrow::FileReaderBuilder builder;
  check(builder.Open(input));
  parquet::ArrowReaderProperties properties;
  properties.set_pre_buffer(prebuffer);
  builder.properties(properties);
  auto reader = builder.Build().ValueOrDie();
  std::shared_ptr<arrow::Table> table;
  check(reader->ReadTable(&table));
  return table;
}
int main(int argc, char** argv)
{
  std::set_terminate([] { std::cerr << "TERMINATE at " << current_stage << '\n' << std::flush; trace(); std::abort(); });
  const std::string mode = argc > 1 ? argv[1] : "default";
  try
  {
    stage("building");
    arrow::DoubleBuilder values;
    check(values.Append(100.25));
    check(values.Append(200.5));
    auto array = values.Finish().ValueOrDie();
    const auto table = arrow::Table::Make(arrow::schema({arrow::field("mz", arrow::float64())}), {array});
    const std::string file = "probe.parquet";
    stage("write");
    {
      auto output = arrow::io::FileOutputStream::Open(file).ValueOrDie();
      check(parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), output, 1024));
      check(output->Close());
    }
    stage("read");
    const auto restored = read(file, mode != "no-prebuffer");
    stage("count");
    int64_t rows;
    { auto reader = parquet::ParquetFileReader::OpenFile(file, false); rows = reader->metadata()->num_rows(); }
    stage("remove");
    std::filesystem::remove(file);
    if (rows != 2 || restored->num_columns() != 1) return 1;
    const auto column = std::static_pointer_cast<arrow::DoubleArray>(restored->column(0)->chunk(0));
    if (column->Value(0) != 100.25 || column->Value(1) != 200.5) return 2;
    stage("release");
  }
  catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 3; }
  stage("objects released");
  if (mode == "shutdown")
  {
    auto* pool = dynamic_cast<arrow::internal::ThreadPool*>(arrow::io::default_io_context().executor());
    if (!pool) throw std::runtime_error("Default I/O executor is not a thread pool");
    check(pool->Shutdown());
    check(arrow::internal::GetCpuThreadPool()->Shutdown());
    stage("pools shut down");
  }
  return 0;
}
