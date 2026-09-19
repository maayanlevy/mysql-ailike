#include "rewrite.h"

#include <cstdarg>
#include <cstring>
#include <exception>
#include <new>
#include <mysqld_error.h>

#include <mysql/attribute.h>
#include <mysql/components/services/mysql_runtime_error.h>
// Use the published ABI declarations without internal server implementation
// headers. This is the same mode MySQL uses to verify its plugin ABI.
struct CHARSET_INFO;
#define MYSQL_ABI_CHECK
#include <mysql/plugin.h>
#include <mysql/plugin_audit.h>
#undef MYSQL_ABI_CHECK
#include <mysql/service_mysql_alloc.h>
#include <mysql/service_plugin_registry.h>

namespace {

SERVICE_TYPE(registry) *registry = nullptr;
my_h_service error_handle = nullptr;
SERVICE_TYPE(mysql_runtime_error) *runtime_error = nullptr;

void report_error(int code, ...) noexcept {
  if (!runtime_error) return;
  va_list args;
  va_start(args, code);
  runtime_error->emit(code, 0, args);
  va_end(args);
}

int initialize(MYSQL_PLUGIN) {
  registry = mysql_plugin_registry_acquire();
  if (!registry) return 1;
  if (registry->acquire("mysql_runtime_error", &error_handle)) {
    mysql_plugin_registry_release(registry);
    registry = nullptr;
    return 1;
  }
  runtime_error = reinterpret_cast<SERVICE_TYPE(mysql_runtime_error) *>(error_handle);
  return 0;
}

int deinitialize(MYSQL_PLUGIN) {
  runtime_error = nullptr;
  if (error_handle) registry->release(error_handle);
  error_handle = nullptr;
  if (registry) mysql_plugin_registry_release(registry);
  registry = nullptr;
  return 0;
}

int rewrite_query(MYSQL_THD, mysql_event_class_t event_class,
                  const void *event) noexcept {
  if (event_class != MYSQL_AUDIT_PARSE_CLASS) return 0;
  const auto *parse = static_cast<const mysql_event_parse *>(event);
  if (parse->event_subclass != MYSQL_AUDIT_PARSE_PREPARSE) return 0;
  try {
    auto result = ailike::rewrite({parse->query.str, parse->query.length});
    if (!result.error.empty()) {
      report_error(ER_UDF_ERROR, "ailike_rewrite", result.error.c_str());
      return 1;
    }
    if (!result.changed) return 0;
    auto *buffer = static_cast<char *>(my_malloc(0, result.query.size() + 1, 0));
    if (!buffer) {
      report_error(ER_UDF_ERROR, "ailike_rewrite", "could not allocate rewritten query");
      return 1;
    }
    std::memcpy(buffer, result.query.data(), result.query.size());
    buffer[result.query.size()] = '\0';
    parse->rewritten_query->str = buffer;
    parse->rewritten_query->length = result.query.size();
    *parse->flags = static_cast<mysql_event_parse_rewrite_plugin_flag>(
        static_cast<int>(*parse->flags) |
        MYSQL_AUDIT_PARSE_REWRITE_PLUGIN_QUERY_REWRITTEN);
    return 0;
  } catch (const std::exception &) {
    report_error(ER_UDF_ERROR, "ailike_rewrite", "could not rewrite query");
    return 1;
  } catch (...) {
    report_error(ER_UDF_ERROR, "ailike_rewrite", "unexpected rewrite failure");
    return 1;
  }
}

st_mysql_audit descriptor = {
    MYSQL_AUDIT_INTERFACE_VERSION,
    nullptr,
    rewrite_query,
    {0, 0, MYSQL_AUDIT_PARSE_PREPARSE},
};

}  // namespace

mysql_declare_plugin(ailike_rewrite){
    MYSQL_AUDIT_PLUGIN,
    &descriptor,
    "ailike_rewrite",
    "AILIKE contributors",
    "Translate column AILIKE prompt into the ailike loadable function",
    PLUGIN_LICENSE_GPL,
    initialize,
    nullptr,
    deinitialize,
    0x0100,
    nullptr,
    nullptr,
    nullptr,
    0,
} mysql_declare_plugin_end;
