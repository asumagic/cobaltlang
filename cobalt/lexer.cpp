#include "lexer.hpp"

#include <algorithm>
#include <array>
#include "utility.hpp"

namespace co
{
constexpr std::array<std::pair<std::string_view, TokenType>, 10> basic_tokens {{
	{"\n", TokenType::EndOfLine},
	{"(", TokenType::ParameterListBegin},
	{")", TokenType::ParameterListEnd},
	{"{", TokenType::FunctionBodyBegin},
	{"}", TokenType::FunctionBodyEnd},
	{"[", TokenType::PropertyBodyBegin},
	{"]", TokenType::PropertyBodyEnd},
	{"=", TokenType::Equal},
	{":", TokenType::TypeConstraintSeparator},
	{",", TokenType::Separator},
}};

bool Lexer::is_first_identifier_char(char c)
{
	return isalpha(c);
}

bool Lexer::is_identifier_char(char c)
{
	return is_first_identifier_char(c)
		|| isdigit(c);
}

bool Lexer::eof() const
{
	return _cursor == _source.end();
}

bool Lexer::match(std::string_view substring) const
{
	return size_t(std::distance(_cursor, _source.end())) >= substring.size()
		&& substring == std::string_view{_cursor, substring.size()};
}

bool Lexer::match(char c) const
{
	return *_cursor == c;
}

void Lexer::skip_until(const std::function<bool()>& stop_condition)
{
	while (!eof() && !stop_condition())
	{
		++_cursor;
	}
}

void Lexer::skip_until(char stop_char)
{
	skip_until([&] { return *_cursor == stop_char; });
}

void Lexer::skip_until(std::string_view stop_string)
{
	skip_until([&] { return match(stop_string); });
}

void Lexer::skip_beyond(const std::function<bool()>& stop_condition)
{
	skip_until(stop_condition);
	skip();
}

void Lexer::skip_beyond(char stop_char)
{
	skip_until(stop_char);
	skip();
}

void Lexer::skip_beyond(std::string_view stop_string)
{
	skip_until(stop_string);
	skip(stop_string.size());
}

void Lexer::skip(size_t count)
{
	if (!eof())
	{
		_cursor += count;
	}
}

std::optional<Token> Lexer::try_tokenize(std::string_view name, TokenType type)
{
	if (match(name))
	{
		Token token{type, {_cursor, name.size()}};
		skip(name.size());
		return token;
	}

	return {};
}

char Lexer::next_char()
{
	return *(++_cursor);
}

Lexer::Lexer(std::string_view source) :
	_source{source},
	_cursor{source.begin()}
{}

Token Lexer::next_token()
{
	skip_until([&]{ return !isspace(*_cursor); });

	if (match("//"))
	{
		skip_until('\n');
	}
	else if (match("/*"))
	{
		skip_beyond("*/");
	}

	if (eof())
	{
		return {TokenType::EndOfFile, {_cursor, 1}};
	}

	auto token_begin = _cursor;

	for (auto& t : basic_tokens)
	{
		if (auto token = try_tokenize(t.first, t.second); token)
		{
			return *token;
		}
	}

	if (is_first_identifier_char(*_cursor))
	{
		skip_until([&] { return !is_identifier_char(*_cursor); });
		std::string_view view = make_view(token_begin, _cursor);

		if (view == "return")
		{
			return {TokenType::Return, view};
		}
		else if (view == "break")
		{
			return {TokenType::Break, view};
		}
		else if (view == "continue")
		{
			return {TokenType::Continue, view};
		}

		value = std::string{view};
		return {TokenType::Identifier, view};
	}

	if (isdigit(*_cursor)) // numeric literal
	{
		skip_until([&] { return !isdigit(*_cursor); });

		if (match(".")) // floating point
		{
			skip(); // skip digit separator '.'
			skip_until([&] { return !isdigit(*_cursor); });
			value = std::stod(std::string{token_begin, _cursor});
			return {TokenType::LiteralFloat, make_view(token_begin, _cursor)};
		}
		else
		{
			value = std::stoll(std::string{token_begin, _cursor});
			return {TokenType::LiteralInt, make_view(token_begin, _cursor)};
		}
	}

	// TODO: handle string

	return {TokenType::Unexpected, {_cursor++, 1}};
}
}
