#include "stdproj.h"

#include "Expr.h"


void Expr::SkipSpaces()
{
	while (*cur_ == ' ' || *cur_ == '\t')
		++cur_;
}


bool Expr::ParseFactor(address_t &out)
{
	SkipSpaces();

	if (*cur_ == '-')			// unary minus
	{
		++cur_;
		if (!ParseFactor(out))
			return false;
		out = static_cast<address_t>(-out);
		return true;
	}

	if (*cur_ == '(')			// parenthesised sub-expression
	{
		++cur_;
		if (!ParseSum(out))
			return false;
		SkipSpaces();
		if (*cur_ != ')')
		{
			Error() << "expr: expected ')'\n";
			return false;
		}
		++cur_;
		return true;
	}

	// numeric literal: hex (0x..), octal (0..) or decimal, via strtoul base 0
	if (*cur_ < '0' || *cur_ > '9')
	{
		Error() << "expr: expected a value\n";
		return false;
	}
	char *end = nullptr;
	out = static_cast<address_t>(strtoul(cur_, &end, 0));
	cur_ = end;
	return true;
}


bool Expr::ParseProduct(address_t &out)
{
	if (!ParseFactor(out))
		return false;

	for (;;)
	{
		SkipSpaces();
		const char op = *cur_;
		if (op != '*' && op != '/' && op != '%')
			return true;
		++cur_;

		address_t rhs;
		if (!ParseFactor(rhs))
			return false;

		if ((op == '/' || op == '%') && rhs == 0)
		{
			Error() << "expr: divide by zero\n";
			return false;
		}

		if (op == '*')
			out *= rhs;
		else if (op == '/')
			out /= rhs;
		else
			out %= rhs;
	}
}


bool Expr::ParseSum(address_t &out)
{
	if (!ParseProduct(out))
		return false;

	for (;;)
	{
		SkipSpaces();
		const char op = *cur_;
		if (op != '+' && op != '-')
			return true;
		++cur_;

		address_t rhs;
		if (!ParseProduct(rhs))
			return false;

		if (op == '+')
			out += rhs;
		else
			out -= rhs;
	}
}


bool Expr::Eval(const char *text, address_t &value)
{
	Expr e(text);

	address_t result;
	if (!e.ParseSum(result))
	{
		Error() << "expr: bad expression: " << text << '\n';
		return false;
	}

	e.SkipSpaces();
	if (*e.cur_ != 0)
	{
		Error() << "expr: trailing characters: " << e.cur_ << '\n';
		return false;
	}

	value = result;
	return true;
}
