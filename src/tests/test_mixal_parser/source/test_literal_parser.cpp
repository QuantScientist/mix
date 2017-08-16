#include <mixal/literal_parser.h>
#include <mixal/w_value_parser.h>

#include "parser_test_fixture.h"

using namespace mixal;

namespace {

class LiteralParserTest :
	public ParserTest<LiteralParser>
{
};

std::string MakeLongWExpression()
{
	std::string wexpr;

	std::string expr{"10(1:5)"};
	for (int i = 0; i < 3; ++i)
	{
		wexpr += expr;
		wexpr += ",";
	}
	wexpr += expr;

	return wexpr;
}

} // namespace

TEST_F(LiteralParserTest, Fails_If_Not_Surrounded_With_Equal_Char)
{
	parse_error("1000");
}

TEST_F(LiteralParserTest, Fails_If_Expression_In_Equal_Chars_Is_Empty)
{
	parse_error("==");
}

TEST_F(LiteralParserTest, Parse_Expression_Inside_Equal_Chars_Like_W_Expression)
{
	parse("=11(1:1),77=");

	WValueParser w_parser;
	w_parser.parse_stream("11(1:1),77");

	ASSERT_EQ(w_parser.value(), parser_.value());
}

TEST_F(LiteralParserTest, Fails_For_Too_Long_Expressions)
{
	std::string wexpr = MakeLongWExpression();
	ASSERT_LT(k_max_symbol_length, wexpr.size());

	std::string literal = "=" + wexpr + "=";
	parse_error(literal);
}

TEST_F(LiteralParserTest, Parses_Only_Constant_Literal)
{
	parse("=8= (1:1)");
	reminder_stream_is(" (1:1)");
}