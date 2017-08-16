#pragma once
#include <mixal/expression_parser.h>
#include <mixal/field_parser.h>

namespace mixal {

class MIXAL_PARSER_LIB_EXPORT WValueParser final :
	public IParser
{
public:
	const WValue& value() const;

private:
	virtual std::size_t do_parse_stream(std::string_view str, std::size_t offset) override;
	virtual void do_clear() override;

	std::size_t parse_expr_with_field(std::string_view str, std::size_t offset);
	void add_token(ExpressionParser&& expr, FieldParser&& field);

private:
	WValue value_;
};

} // namespace mixal


