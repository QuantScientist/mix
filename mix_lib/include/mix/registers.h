#pragma once
#include <mix/word.h>

namespace mix {

class Register : public Word
{
};

class IndexRegister : public Register
{
public:
	explicit IndexRegister();
	explicit IndexRegister(int two_bytes_with_sign);
	explicit IndexRegister(const Byte& b4, const Byte& b5);
	explicit IndexRegister(Sign sign, const Byte& b4, const Byte& b5);

	void set_byte(std::size_t index, const Byte& byte);
	WordValue value() const;

private:
	bool is_undefined_index(std::size_t index) const;
};

class AddressRegister : public IndexRegister
{
public:
	using IndexRegister::IndexRegister;

	// #TODO: `sing() == Sign::Positive` should be true always
};

} // namespace mix

