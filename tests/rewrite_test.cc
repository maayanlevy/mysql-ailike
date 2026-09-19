#include "rewrite.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {
int failures = 0;

void expect(std::string_view sql, std::string_view wanted) {
  const auto result = ailike::rewrite(sql);
  if (!result.error.empty() || !result.changed || result.query != wanted) {
    std::cerr << "rewrite failed: " << sql << "\nactual: " << result.query
              << "\nerror: " << result.error << '\n';
    ++failures;
  }
}

void unchanged(std::string_view sql) {
  const auto result = ailike::rewrite(sql);
  if (result.changed || !result.error.empty()) {
    std::cerr << "unexpected rewrite: " << sql << "\nerror: " << result.error << '\n';
    ++failures;
  }
}

void rejected(std::string_view sql, std::string_view error_text = {}) {
  const auto result = ailike::rewrite(sql);
  if (result.error.empty() || result.changed ||
      result.error.find(error_text) == std::string::npos) {
    std::cerr << "expected rejection: " << sql << '\n';
    ++failures;
  }
}
}  // namespace

int main() {
  expect("SELECT * FROM films WHERE description AILIKE 'about space'",
         "SELECT * FROM films WHERE ailike(description, 'about space')");
  expect("SELECT * FROM films WHERE f.description aIlIkE 'space'",
         "SELECT * FROM films WHERE ailike(f.description, 'space')");
  expect("SELECT * FROM films WHERE `db`.`weird``table`.`description` NOT AILIKE 'war'",
         "SELECT * FROM films WHERE (NOT ailike(`db`.`weird``table`.`description`, 'war'))");
  expect("SELECT description AILIKE 'a person''s story' AS matches FROM films",
         "SELECT ailike(description, 'a person''s story') AS matches FROM films");
  expect("SELECT * FROM films WHERE (a AILIKE 'a') OR b NOT AILIKE 'b' LIMIT 3",
         "SELECT * FROM films WHERE (ailike(a, 'a')) OR (NOT ailike(b, 'b')) LIMIT 3");
  expect("SELECT * FROM t WHERE a /* before */ AILIKE /* after */ 'p'",
         "SELECT * FROM t WHERE ailike(a /* before */ , /* after */ 'p')");
  expect("SELECT * FROM t WHERE a NOT /* keep */ AILIKE 'space'",
         "SELECT * FROM t WHERE (NOT ailike(a  /* keep */ , 'space'))");
  expect("SELECT * FROM t WHERE a AILIKE NULL",
         "SELECT * FROM t WHERE ailike(a, NULL)");
  expect("SELECT * FROM t WHERE a AILIKE 'é 🌍' AND b = 1",
         "SELECT * FROM t WHERE ailike(a, 'é 🌍') AND b = 1");
  expect("SELECT * FROM t WHERE ailike AILIKE 'interesting'",
         "SELECT * FROM t WHERE ailike(ailike, 'interesting')");
  expect("SELECT * FROM t WHERE a -- keep AILIKE 'x'\n AILIKE 'y'",
         "SELECT * FROM t WHERE ailike(a -- keep AILIKE 'x'\n , 'y')");
  expect("SELECT * FROM t WHERE a /* ? */ AILIKE 'a question?'",
         "SELECT * FROM t WHERE ailike(a /* ? */ , 'a question?')");
  expect("SELECT * FROM t WHERE `column?` AILIKE 'space'",
         "SELECT * FROM t WHERE ailike(`column?`, 'space')");

  unchanged("SELECT ailike(description, 'about space') FROM films");
  unchanged("SELECT ailike(description, ?) FROM films WHERE id = ?");
  unchanged("PREPARE s FROM 'SELECT id FROM films WHERE ailike(description, ?)'");
  unchanged("SELECT another_function(description, 'about space') FROM films");
  unchanged("SELECT 'column AILIKE ''prompt''' AS example");
  unchanged("SELECT \"column AILIKE 'prompt'\" AS example");
  unchanged("SELECT `column AILIKE 'prompt'` FROM t");
  unchanged("SELECT 1 /* WHERE a AILIKE 'prompt' */");
  unchanged("SELECT 1 -- WHERE a AILIKE 'prompt'\n");
  unchanged("SELECT 1 # WHERE a AILIKE 'prompt'\n");
  unchanged("SELECT ailike, t.ailike, a AS ailike FROM t");
  unchanged("SELECT a ailike FROM t");
  unchanged("SELECT ailike 'label' FROM t");
  unchanged("SELECT ailike \"label\" FROM t");
  unchanged("SELECT DISTINCT ailike 'label' FROM t");
  unchanged("SELECT ALL ailike 'label' FROM t");
  unchanged("SELECT SQL_NO_CACHE ailike 'label' FROM t");
  unchanged("SELECT a, ailike 'label' FROM t");
  unchanged("SELECT @ailike 'label'");
  unchanged("SELECT 1 + ailike 'label' FROM t");
  unchanged("SELECT NOT ailike 'label' FROM t");
  unchanged("SELECT BINARY ailike 'label' FROM t");
  unchanged("SELECT * FROM t WHERE ailike = 1");
  unchanged("SELECT 'a\\nb' AS text");
  unchanged("SELECT ailike(a, 'a\\nb') FROM t");
  unchanged(R"(SELECT 'it\'s a AILIKE ''x''')");
  unchanged(R"(SELECT '\' AS a, 'text AILIKE ''x''' AS b)");
  unchanged("SELECT * FROM t WHERE a AILIKE 'a\\'b'");
  unchanged("SELECT 'a\\nb' FROM t WHERE a AILIKE 'p'");
  unchanged("SELECT * FROM t WHERE a AILIKE other_column");

  rejected("SELECT * FROM t WHERE LOWER(a) AILIKE 'p'");
  rejected("SELECT * FROM t WHERE a + b AILIKE 'p'");
  rejected("SELECT * FROM t WHERE a AILIKE 'p' + 'q'");
  rejected("SELECT * FROM t WHERE a AILIKE 'p' 'q'");
  rejected("SELECT * FROM t WHERE a AILIKE 'p' COLLATE utf8mb4_bin");
  rejected("SELECT * FROM t WHERE a AILIKE \"p\"");
  rejected("SELECT * FROM t WHERE \"a\" AILIKE 'p'");
  rejected("SELECT * FROM t WHERE a /*! + b */ AILIKE 'p'");
  rejected("SELECT * FROM t WHERE a AILIKE 'p' AILIKE 'q'");
  rejected("SELECT * FROM t WHERE a AILIKE 'unterminated");
  rejected("SELECT * FROM t WHERE a AILIKE 'p' /* unterminated");
  rejected("SELECT * FROM t WHERE NOT a AILIKE 'p'");
  rejected("SELECT * FROM t WHERE a.b.c.d AILIKE 'p'");
  rejected(std::string("SELECT * FROM t WHERE a AILIKE 'p'") + '\0');
  rejected("SELECT * FROM films WHERE f.description aIlIkE ?", "ailike(column, ?)");
  rejected("SELECT * FROM t WHERE a NOT AILIKE ?", "ailike(column, ?)");
  rejected("SELECT ? FROM t WHERE a AILIKE 'p'", "bound parameters");
  rejected("SELECT * FROM t WHERE id = ? AND a AILIKE 'p'", "bound parameters");
  rejected("SELECT * FROM t WHERE a AILIKE 'p' AND id = ?", "bound parameters");
  rejected("SELECT ailike(a, ?) FROM t WHERE b AILIKE 'p'", "bound parameters");

  if (failures) return EXIT_FAILURE;
  std::cout << "rewrite tests passed\n";
  return EXIT_SUCCESS;
}
