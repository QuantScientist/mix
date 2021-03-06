#pragma once
#include <mixal_parse/parser_base.h>
#include <mixal_parse/address_parser.h>
#include <mixal_parse/index_parser.h>
#include <mixal_parse/field_parser.h>
#include <mixal_parse/w_value_parser.h>

#include <mixal_parse/types/text.h>
#include <mixal_parse/types/operation_id.h>

namespace mixal_parse {

struct MIXOpParser
{
	AddressParser address_parser;
	IndexParser index_parser;
	FieldParser field_parser;
};

struct MIXALOpParser
{
	std::optional<WValueParser> w_value_parser;
	std::optional<Text> alf_text;
};

// Operation-dependent parsing of MIXAL ADDRESS column
class MIXAL_PARSE_LIB_EXPORT OperationAddressParser final :
	public ParserBase
{
public:
	explicit OperationAddressParser(OperationId operation);

	ConstOptionalRef<MIXOpParser> mix() const;
	ConstOptionalRef<MIXALOpParser> mixal() const;

	bool is_mix_operation() const;
	bool is_mixal_operation() const;

private:
	virtual void do_clear() override;
	virtual std::size_t do_parse_stream(std::string_view str, std::size_t offset) override;

	std::size_t parse_as_mix(const std::string_view& str, std::size_t offset);
	std::size_t parse_as_mixal(const std::string_view& str, std::size_t offset);

	std::size_t parse_mixal_op_as_wvalue(const std::string_view& str, std::size_t offset);
	std::size_t parse_mixal_alf_op(const std::string_view& str, std::size_t offset);

	std::size_t try_parse_alf_with_quotes(const std::string_view& str, std::size_t offset);
	std::size_t try_parse_standard_alf(const std::string_view& str, std::size_t offset);
	std::size_t set_alf_text(const std::string_view& str, std::size_t offset);

private:
	OperationId op_;
	std::optional<MIXOpParser> mix_;
	std::optional<MIXALOpParser> mixal_;
};

} // namespace mixal_parse
