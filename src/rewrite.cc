#include "rewrite.h"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <vector>

namespace ailike {
namespace {

enum class Kind { word, identifier, string, double_quoted, parameter, symbol };

struct Token {
  Kind kind;
  std::size_t begin;
  std::size_t end;
};

struct Lexed {
  std::vector<Token> tokens;
  bool executable_comment = false;
  bool backslash = false;
  bool malformed = false;
};

bool space(unsigned char c) {
  return c == ' ' || (c >= '\t' && c <= '\r');
}

bool word_start(unsigned char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
         c == '$' || c >= 0x80;
}

bool word_part(unsigned char c) {
  return word_start(c) || (c >= '0' && c <= '9');
}

bool equal(std::string_view text, std::string_view expected) {
  if (text.size() != expected.size()) return false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    const char upper = c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
    if (upper != expected[i]) return false;
  }
  return true;
}

std::string_view view(std::string_view sql, const Token &token) {
  return sql.substr(token.begin, token.end - token.begin);
}

bool keyword(std::string_view sql, const Token &token, std::string_view name) {
  return token.kind == Kind::word && equal(view(sql, token), name);
}

bool symbol(std::string_view sql, const Token &token, char c) {
  return token.kind == Kind::symbol && token.end == token.begin + 1 &&
         sql[token.begin] == c;
}

Lexed tokenize(std::string_view sql) {
  Lexed result;
  for (std::size_t i = 0; i < sql.size();) {
    const std::size_t begin = i;
    const char c = sql[i];
    if (space(static_cast<unsigned char>(c))) {
      ++i;
    } else if (c == '#' ||
               (c == '-' && i + 2 < sql.size() && sql[i + 1] == '-' &&
                (static_cast<unsigned char>(sql[i + 2]) <= 0x20 ||
                 sql[i + 2] == 0x7f))) {
      while (i < sql.size() && sql[i] != '\n' && sql[i] != '\r') ++i;
    } else if (c == '/' && i + 1 < sql.size() && sql[i + 1] == '*') {
      if (i + 2 < sql.size() && sql[i + 2] == '!')
        result.executable_comment = true;
      const auto end = sql.find("*/", i + 2);
      if (end == std::string_view::npos) {
        result.malformed = true;
        break;
      }
      i = end + 2;
    } else if (c == '\'' || c == '"' || c == '`') {
      ++i;
      bool closed = false;
      while (i < sql.size()) {
        if (sql[i] == '\\' && c != '`') {
          result.backslash = true;
          if (i + 1 < sql.size()) {
            i += 2;
            continue;
          }
        }
        if (sql[i++] != c) continue;
        if (i < sql.size() && sql[i] == c) {
          ++i;
          continue;
        }
        closed = true;
        break;
      }
      const auto kind = c == '\'' ? Kind::string
                        : c == '`' ? Kind::identifier
                                   : Kind::double_quoted;
      result.tokens.push_back({kind, begin, i});
      if (!closed) {
        result.malformed = true;
        break;
      }
    } else if (word_start(static_cast<unsigned char>(c))) {
      while (++i < sql.size() && word_part(static_cast<unsigned char>(sql[i]))) {}
      result.tokens.push_back({Kind::word, begin, i});
    } else {
      result.tokens.push_back(
          {c == '?' ? Kind::parameter : Kind::symbol, begin, ++i});
    }
  }
  return result;
}

bool one_of(std::string_view sql, const Token &token,
            std::initializer_list<std::string_view> names) {
  return std::any_of(names.begin(), names.end(), [&](std::string_view name) {
    return keyword(sql, token, name);
  });
}

bool column(std::string_view sql, const Token &token) {
  if (token.kind == Kind::identifier) return true;
  if (token.kind != Kind::word) return false;
  return !one_of(sql, token,
                 {"SELECT", "FROM", "WHERE", "ON", "HAVING", "AND", "OR",
                  "XOR", "NOT", "AS", "WHEN", "THEN", "ELSE", "END", "CASE",
                  "NULL", "TRUE", "FALSE", "IS", "IN", "LIKE", "BETWEEN",
                  "LIMIT", "OFFSET", "ORDER", "GROUP", "BY", "SET", "JOIN",
                  "UNION", "ALL", "DISTINCT", "UPDATE", "INSERT", "DELETE"});
}

bool left_boundary(std::string_view sql, const Token &token) {
  return symbol(sql, token, '(') || symbol(sql, token, ',') ||
         one_of(sql, token, {"SELECT", "WHERE", "ON", "HAVING", "AND", "OR",
                             "XOR", "WHEN", "THEN", "ELSE", "DISTINCT"});
}

bool right_boundary(std::string_view sql, const Token &token) {
  return symbol(sql, token, ')') || symbol(sql, token, ',') ||
         symbol(sql, token, ';') ||
         one_of(sql, token,
                {"AND", "OR", "XOR", "IS", "AS", "FROM", "WHERE", "HAVING",
                 "GROUP", "ORDER", "LIMIT", "OFFSET", "FOR", "LOCK", "UNION",
                 "INTO", "THEN", "ELSE", "END", "WINDOW", "ASC", "DESC"});
}

bool starts_expression(std::string_view sql, const Token &token) {
  if (left_boundary(sql, token) ||
      one_of(sql, token,
             {"NOT", "BINARY", "ALL", "DISTINCTROW", "HIGH_PRIORITY",
              "STRAIGHT_JOIN", "SQL_SMALL_RESULT", "SQL_BIG_RESULT",
              "SQL_BUFFER_RESULT", "SQL_NO_CACHE", "SQL_CALC_FOUND_ROWS"}))
    return true;
  if (token.kind != Kind::symbol) return false;
  const char c = sql[token.begin];
  return c == '@' || c == '+' || c == '-' || c == '*' || c == '/' ||
         c == '%' || c == '=' || c == '<' || c == '>' || c == '!' ||
         c == '~' || c == '&' || c == '|' || c == '^';
}

struct Replacement {
  std::size_t begin;
  std::size_t end;
  std::string text;
};

bool has_trivia_comment(std::string_view text) {
  return std::any_of(text.begin(), text.end(),
                     [](unsigned char c) { return !space(c); });
}

RewriteResult rewrite_tokens(std::string_view sql, const Lexed &lexed) {
  const auto &tokens = lexed.tokens;
  std::vector<Replacement> replacements;
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    if (!keyword(sql, tokens[i], "AILIKE")) continue;
    // Function calls, qualified names and ordinary aliases are not operators.
    if (i + 1 < tokens.size() && symbol(sql, tokens[i + 1], '(')) continue;
    if ((i && symbol(sql, tokens[i - 1], '.')) ||
        (i + 1 < tokens.size() && symbol(sql, tokens[i + 1], '.')))
      continue;
    if (i == 0 || i + 1 == tokens.size()) continue;
    const Token &rhs = tokens[i + 1];
    const bool prompt = rhs.kind == Kind::string || rhs.kind == Kind::parameter ||
                        rhs.kind == Kind::double_quoted ||
                        keyword(sql, rhs, "NULL");
    if (!prompt) continue;  // Unsupported SQL is left for MySQL's parser.

    bool negated = keyword(sql, tokens[i - 1], "NOT");
    if (negated && i < 2)
      return {false, {}, "AILIKE requires a column reference on its left"};
    const std::size_t last = i - (negated ? 2 : 1);
    std::size_t first = last;
    // A column or user variable named ailike can itself be a SELECT
    // expression with a quoted alias: SELECT ailike 'label' FROM t.
    // There is no left operand in this position, so leave it to MySQL.
    if (starts_expression(sql, tokens[last])) continue;
    if (!column(sql, tokens[last]))
      return {false, {},
              "AILIKE accepts column references; use ailike(expression, prompt) "
              "for expressions"};
    while (first >= 2 && symbol(sql, tokens[first - 1], '.') &&
           column(sql, tokens[first - 2]))
      first -= 2;
    if (last - first > 4 ||
        (first && !left_boundary(sql, tokens[first - 1])))
      return {false, {},
              "AILIKE requires a standalone column predicate; use "
              "ailike(expression, prompt) for expressions"};
    if (i + 2 < tokens.size() && !right_boundary(sql, tokens[i + 2]))
      return {false, {},
              "AILIKE accepts one prompt literal; use "
              "ailike(column, prompt_expression) for expressions"};
    if (rhs.kind == Kind::double_quoted)
      return {false, {}, "AILIKE prompts must use single quotes"};
    if (lexed.executable_comment)
      return {false, {},
              "AILIKE infix syntax cannot be combined with executable comments; "
              "use ailike(column, prompt)"};
    if (lexed.malformed || sql.find('\0') != std::string_view::npos)
      return {false, {}, "AILIKE cannot rewrite malformed SQL"};

    const auto &lhs = tokens[first];
    std::string trivia(sql.substr(tokens[last].end,
                                  tokens[i - (negated ? 1 : 0)].begin -
                                      tokens[last].end));
    if (negated)
      trivia += sql.substr(tokens[i - 1].end,
                           tokens[i].begin - tokens[i - 1].end);
    if (!has_trivia_comment(trivia)) trivia.clear();
    const auto after = sql.substr(tokens[i].end, rhs.begin - tokens[i].end);
    std::string text = negated ? "(NOT ailike(" : "ailike(";
    text += sql.substr(lhs.begin, tokens[last].end - lhs.begin);
    text += trivia;
    text += ',';
    text += has_trivia_comment(after) ? std::string(after) : " ";
    text += view(sql, rhs);
    text += negated ? "))" : ")";
    replacements.push_back({lhs.begin, rhs.end, std::move(text)});
    ++i;
  }
  if (replacements.empty()) return {};
  // MySQL reinitializes its lexer after a preparse rewrite, clearing the flag
  // that recognizes '?' as a bound parameter. This affects every parameter
  // in the query, even one outside an AILIKE predicate. Function-only queries
  // are left unchanged and retain normal prepared-statement behavior.
  if (std::any_of(tokens.begin(), tokens.end(), [](const Token &token) {
        return token.kind == Kind::parameter;
      }))
    return {false, {},
            "MySQL preparse rewriting does not support bound parameters; "
            "use ailike(column, ?) instead of infix AILIKE"};
  RewriteResult result;
  result.changed = true;
  std::size_t copied = 0;
  for (const auto &replacement : replacements) {
    if (replacement.begin < copied)
      return {false, {}, "AILIKE predicates cannot overlap"};
    result.query += sql.substr(copied, replacement.begin - copied);
    result.query += replacement.text;
    copied = replacement.end;
  }
  result.query += sql.substr(copied);
  return result;
}

}  // namespace

RewriteResult rewrite(std::string_view sql) {
  bool mentioned = false;
  for (std::size_t i = 0; i + 6 <= sql.size(); ++i) {
    if (equal(sql.substr(i, 6), "AILIKE")) {
      mentioned = true;
      break;
    }
  }
  if (!mentioned) return {};
  const auto lexed = tokenize(sql);
  // Backslash quoting depends on SQL mode. Passing ambiguous SQL through
  // preserves ordinary queries in either mode; unsupported infix syntax will
  // receive MySQL's normal syntax error instead of an unsafe partial rewrite.
  if (lexed.backslash) return {};
  return rewrite_tokens(sql, lexed);
}

}  // namespace ailike
