#pragma once
#include <arrow/api.h>
#ifdef _WIN32
#ifdef PROBE_LIBRARY
#define PROBE_API __declspec(dllexport)
#else
#define PROBE_API __declspec(dllimport)
#endif
#else
#define PROBE_API
#endif
PROBE_API std::shared_ptr<arrow::Table> read(const std::string& file, bool prebuffer);
