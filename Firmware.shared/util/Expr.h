#pragma once

#include "util/util.h"		// address_t

//! Evaluator for monitor-command address/value expressions: numeric literals
//! (hex 0x.., decimal, or octal via a leading 0), the binary operators
//! + - * / %, unary minus and parentheses. \c address_t is the project value
//! type, in scope via stdproj.h.
class Expr
{
public:
	//! Parse and evaluate \p text. On success writes the result to \p value and
	//! returns true. On a syntax error or divide-by-zero, logs the reason via
	//! Error() and returns false.
	static bool Eval(const char *text, address_t &value);

private:
	explicit Expr(const char *text) : cur_(text) {}

	//! Recursive-descent levels, lowest precedence first.
	bool ParseSum(address_t &out);		//!< term (('+' | '-') term)*
	bool ParseProduct(address_t &out);	//!< factor (('*' | '/' | '%') factor)*
	bool ParseFactor(address_t &out);	//!< '-' factor | '(' sum ')' | number
	void SkipSpaces();

	const char *cur_;
};
