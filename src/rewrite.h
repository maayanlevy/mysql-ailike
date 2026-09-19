#pragma once

#include <string>
#include <string_view>

namespace ailike {

struct RewriteResult {
  bool changed = false;
  std::string query;
  std::string error;
};

// A deliberately small SQL extension, not a general SQL parser. Supported
// operands are a column reference and a single-quoted literal or NULL.
// Queries containing parameter markers must use the native function instead:
// MySQL resets prepared-statement parser mode after a preparse rewrite.
// Everything inside quoted tokens and ordinary comments is left untouched.
RewriteResult rewrite(std::string_view sql);

}  // namespace ailike
