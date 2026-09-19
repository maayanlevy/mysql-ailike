#include "typesafe.h"

#include <mysql.h>
#include <mysqld_error.h>
#include <mysql/components/services/mysql_runtime_error.h>
#include <mysql/components/services/udf_metadata.h>
#include <mysql/service_plugin_registry.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace {

struct Context {
  SERVICE_TYPE(registry)* registry = nullptr;
  my_h_service error_handle = nullptr;
  my_h_service metadata_handle = nullptr;
  std::unique_ptr<ailike_plugin::Client> client;

  ~Context() {
    if (registry) {
      if (metadata_handle) registry->release(metadata_handle);
      if (error_handle) registry->release(error_handle);
      mysql_plugin_registry_release(registry);
    }
  }

  void acquire_services(UDF_ARGS* args) {
    registry = mysql_plugin_registry_acquire();
    if (!registry || registry->acquire("mysql_runtime_error", &error_handle) ||
        registry->acquire("mysql_udf_metadata", &metadata_handle)) {
      throw std::runtime_error("MySQL runtime-error and UTF-8 metadata services are required");
    }
    const auto* metadata =
        reinterpret_cast<SERVICE_TYPE(mysql_udf_metadata)*>(metadata_handle);
    char charset[] = "utf8mb4";
    for (unsigned int i = 0; i != 2; ++i) {
      if (metadata->argument_set(args, "charset", i, charset)) {
        throw std::runtime_error("could not configure UTF-8 text arguments");
      }
    }
  }

  void emit_error(int code, ...) const noexcept {
    va_list arguments;
    va_start(arguments, code);
    reinterpret_cast<SERVICE_TYPE(mysql_runtime_error)*>(error_handle)
        ->emit(code, 0, arguments);
    va_end(arguments);
  }

  double score(UDF_ARGS* args) {
    if (!client) {
      client = std::make_unique<ailike_plugin::Client>(
          ailike_plugin::Settings::from_environment());
    }
    return client->score({args->args[0], args->lengths[0]},
                         {args->args[1], args->lengths[1]});
  }
};

bool initialize(UDF_INIT* initid, UDF_ARGS* args, char* message) noexcept {
  initid->ptr = nullptr;
  try {
    if (args->arg_count != 2) {
      throw std::runtime_error("ailike(text, prompt) requires 2 arguments");
    }
    if (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT) {
      throw std::runtime_error("text and prompt must be strings; use CAST(... AS CHAR) explicitly");
    }
    auto context = std::make_unique<Context>();
    context->acquire_services(args);
    initid->maybe_null = true;
    initid->const_item = false;
    initid->decimals = 0;
    initid->max_length = 1;
    initid->ptr = reinterpret_cast<char*>(context.release());
    return false;
  } catch (const std::exception& error) {
    std::snprintf(message, MYSQL_ERRMSG_SIZE, "%s", error.what());
  } catch (...) {
    std::snprintf(message, MYSQL_ERRMSG_SIZE, "unexpected ailike initialization failure");
  }
  return true;
}

bool any_null(const UDF_ARGS* args) noexcept {
  for (unsigned int i = 0; i != args->arg_count; ++i) {
    if (!args->args[i]) return true;
  }
  return false;
}

void fail(Context* context, char* is_null, char* error,
          const char* detail) noexcept {
  *is_null = 1;
  *error = 1;
  // Setting the UDF error byte alone merely produces NULL in MySQL. Emit
  // into the statement's error context so failed inference aborts the query.
  context->emit_error(ER_UDF_ERROR, "ailike", detail);
}

void deinitialize(UDF_INIT* initid) noexcept {
  delete reinterpret_cast<Context*>(initid->ptr);
  initid->ptr = nullptr;
}

}  // namespace

extern "C" {

bool ailike_init(UDF_INIT* initid, UDF_ARGS* args, char* message) {
  return initialize(initid, args, message);
}

void ailike_deinit(UDF_INIT* initid) { deinitialize(initid); }

long long ailike(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* error) {
  auto* context = reinterpret_cast<Context*>(initid->ptr);
  *is_null = 0;
  if (any_null(args)) {
    *is_null = 1;
    return 0;
  }
  try {
    return context->score(args) >= 0.5 ? 1 : 0;
  } catch (const std::exception& exception) {
    fail(context, is_null, error, exception.what());
  } catch (...) {
    fail(context, is_null, error, "unexpected inference failure");
  }
  return 0;
}

}  // extern "C"
