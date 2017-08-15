#pragma once
#include <mixal/expression_parser.h>
#include <mixal/word_field_parser.h>

namespace mixal {

class ConstantWordExpressionParser :
	public IParser
{
public:
	const WordExpression& expression() const;

private:
	virtual std::size_t do_parse_stream(std::string_view str, std::size_t offset);
	virtual void do_clear() override;

	std::size_t parse_expr_with_field(std::string_view str, std::size_t offset);
	void add_token(ExpressionParser&& expr, WordFieldParser&& field);

private:
	WordExpression expression_;
};

} // namespace mixal



