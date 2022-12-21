#include <cassert>
#include <unordered_map>
#include <sstream>
#include <string>
#include <iomanip>
#include "expression.h"
#include "../defs.h"
#include "../utils/comparer.h"

#define THROW_UNSUPPORTED_OPERATOR throw "[Error] unsupported operator.";
#define THROW_COLUMN_NOT_CACHED    throw "[Error] column not cached.";
#define THROW_COLUMN_NOT_UNIQUE    throw "[Error] column not unique.";
#define THROW_TYPE_INCOMPATIBLE    throw "[Error] operand type incompatible.";

static std::unordered_multimap<
	std::string,   // col_name
	std::pair<std::string, expression>  // table_name, expression
> __expr_column_cache;

void expression::cache_clear()
{
	__expr_column_cache.clear();
}

void expression::cache_clear(const char *table)
{
	for(auto it = __expr_column_cache.begin(); it != __expr_column_cache.end(); )
	{
		if(it->second.first == table)
			it = __expr_column_cache.erase(it);
		else ++it;
	}
}

void expression::cache_replace(const char *table, const char *col, expression expr)
{
	auto range = __expr_column_cache.equal_range(col);
	for(auto it = range.first; it != range.second; ++it)
	{
		if(it->second.first == table)
			it->second.second = expr;
	}
}

void expression::cache_column(const char *table, const char *col, const expression &expr)
{
	__expr_column_cache.insert(
		std::make_pair(
			std::string(col),
			std::make_pair( std::string(table), expr )
		)
	);
}

inline expression eval_terminal_column_ref(const expr_node_t *expr)
{
	assert(expr->term_type == TERM_COLUMN_REF);
	std::string col = expr->column_ref->column;
	auto num = __expr_column_cache.count(col);
	if(!num)
	{
		THROW_COLUMN_NOT_CACHED;
	} else if(num > 1 && !expr->column_ref->table) {
		THROW_COLUMN_NOT_UNIQUE;
	} else {
		for(auto it = __expr_column_cache.find(col); it != __expr_column_cache.end(); ++it)
		{
			if(!expr->column_ref->table || it->second.first == expr->column_ref->table)
				return it->second.second;
		}
	}

	THROW_COLUMN_NOT_CACHED;
}

inline int eval_date(const char *str)
{
	std::tm tm{};
	std::string date(str);
	std::stringstream ss(date);
	ss >> std::get_time(&tm, DATE_TEMPLATE);
	std::tm tm_orig = tm;
	if(ss.fail())
		return -1;

	int time = std::mktime(&tm);
	if(tm_orig.tm_mday != tm.tm_mday)
		return -1;

	return time;
}

inline expression eval_terminal(const expr_node_t *expr)
{
	expression ret;
	ret.type = expr->term_type;
	switch(expr->term_type)
	{
		case TERM_INT:
			ret.val_i = expr->val_i;
			break;
		case TERM_FLOAT:
			ret.val_f = expr->val_f;
			break;
		case TERM_STRING:
			ret.val_s = expr->val_s;
			break;
		case TERM_BOOL:
			ret.val_b = expr->val_b;
			break;
		case TERM_DATE:
			ret.val_i = eval_date(expr->val_s);
			if(ret.val_i == -1)
			{
				ret.type = TERM_STRING;
				ret.val_s = expr->val_s;
			}
			break;
		case TERM_NULL:
			break;
		case TERM_COLUMN_REF:
			ret = eval_terminal_column_ref(expr);
			break;
		case TERM_LITERAL_LIST:
			ret.literal_list = expr->literal_list;
			break;
		default:
			assert(0);
			break;
	}

	return ret;
}

inline expression eval_float_operands(operator_type_t op, float a, float b)
{
	expression ret;
	switch(op)
	{
		/* arithmetic */
		case OPERATOR_ADD:
			ret.val_f = a + b;
			ret.type  = TERM_FLOAT;
			break;
		case OPERATOR_MINUS:
			ret.val_f = a - b;
			ret.type  = TERM_FLOAT;
			break;
		case OPERATOR_DIV:
			ret.val_f = a / b;
			ret.type  = TERM_FLOAT;
			break;
		case OPERATOR_MUL:
			ret.val_f = a * b;
			ret.type  = TERM_FLOAT;
			break;
		case OPERATOR_NEGATE:
			ret.val_f = -a;
			ret.type  = TERM_FLOAT;
			break;
		/* compare */
		case OPERATOR_EQ:
			ret.val_b = a == b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_GEQ:
			ret.val_b = a >= b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_LEQ:
			ret.val_b = a <= b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_NEQ:
			ret.val_b = a != b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_GT:
			ret.val_b = a > b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_LT:
			ret.val_b = a < b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_ISNULL:
			ret.val_b = false;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_NOTNULL:
			ret.val_b = true;
			ret.type  = TERM_BOOL;
			break;
		default:
			THROW_UNSUPPORTED_OPERATOR;
			break;
	}

	return ret;
}

inline expression eval_date_operands(operator_type_t op, int a, int b)
{
	expression ret;
	switch(op)
	{
		/* compare */
		case OPERATOR_EQ:
			ret.val_b = a == b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_GEQ:
			ret.val_b = a >= b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_LEQ:
			ret.val_b = a <= b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_NEQ:
			ret.val_b = a != b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_GT:
			ret.val_b = a > b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_LT:
			ret.val_b = a < b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_ISNULL:
			ret.val_b = false;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_NOTNULL:
			ret.val_b = true;
			ret.type  = TERM_BOOL;
			break;
		default:
			THROW_UNSUPPORTED_OPERATOR;
			break;
	}

	return ret;
}

inline expression eval_int_operands(operator_type_t op, int a, int b)
{
	expression ret;
	switch(op)
	{
		/* arithmetic */
		case OPERATOR_ADD:
			ret.val_i = a + b;
			ret.type  = TERM_INT;
			break;
		case OPERATOR_MINUS:
			ret.val_i = a - b;
			ret.type  = TERM_INT;
			break;
		case OPERATOR_DIV:
			ret.val_i = a / b;
			ret.type  = TERM_INT;
			break;
		case OPERATOR_MUL:
			ret.val_i = a * b;
			ret.type  = TERM_INT;
			break;
		case OPERATOR_NEGATE:
			ret.val_i = -a;
			ret.type  = TERM_INT;
			break;
		/* compare */
		case OPERATOR_EQ:
			ret.val_b = a == b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_GEQ:
			ret.val_b = a >= b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_LEQ:
			ret.val_b = a <= b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_NEQ:
			ret.val_b = a != b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_GT:
			ret.val_b = a > b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_LT:
			ret.val_b = a < b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_ISNULL:
			ret.val_b = false;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_NOTNULL:
			ret.val_b = true;
			ret.type  = TERM_BOOL;
			break;
		default:
			THROW_UNSUPPORTED_OPERATOR;
			break;
	}

	return ret;
}

inline expression eval_bool_operands(operator_type_t op, bool a, bool b)
{
	expression ret;
	switch(op)
	{
		/* logical */
		case OPERATOR_AND:
			ret.val_b = a & b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_OR:
			ret.val_b = a | b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_EQ:
			ret.val_b = a == b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_NEQ:
			ret.val_b = a != b;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_ISNULL:
			ret.val_b = false;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_NOTNULL:
			ret.val_b = true;
			ret.type  = TERM_BOOL;
			break;
		default:
			THROW_UNSUPPORTED_OPERATOR;
			break;
	}

	return ret;
}

inline expression eval_string_operands(operator_type_t op, const char* a, const char* b)
{
	expression ret;
	switch(op)
	{
		/* compare */
		case OPERATOR_EQ:
			ret.val_b = strcasecmp(a, b) == 0;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_NEQ:
			ret.val_b = strcasecmp(a, b) != 0;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_LIKE:
			ret.val_b = strlike(a, b);
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_ISNULL:
			ret.val_b = false;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_NOTNULL:
			ret.val_b = true;
			ret.type  = TERM_BOOL;
			break;
		default:
			THROW_UNSUPPORTED_OPERATOR;
			break;
	}

	return ret;
}

inline expression eval_null_operands(operator_type_t op)
{
	expression ret;
	switch(op)
	{
		/* compare */
		case OPERATOR_ISNULL:
			ret.val_b = true;
			ret.type  = TERM_BOOL;
			break;
		case OPERATOR_NOTNULL:
			ret.val_b = false;
			ret.type  = TERM_BOOL;
			break;
		default:
			ret.type  = TERM_NULL;
			break;
	}

	return ret;
}

bool eval_in_expression(expression left, linked_list_t *literal_list)
{
	for(linked_list_t *l_ptr = literal_list; l_ptr; l_ptr = l_ptr->next)
	{
		expr_node_t *val = (expr_node_t*)l_ptr->data;
		assert(val->op == OPERATOR_NONE);
		switch(left.type)
		{
			case TERM_INT:
				if(val->term_type == TERM_INT)
				{
					if(left.val_i == val->val_i)
						return true;
				} else {
					THROW_TYPE_INCOMPATIBLE;
				}
				break;
			case TERM_FLOAT:
				if(val->term_type == TERM_FLOAT)
				{
					if(left.val_f == val->val_f)
						return true;
				} else {
					THROW_TYPE_INCOMPATIBLE;
				}
				break;
			case TERM_STRING:
				if(val->term_type == TERM_STRING || val->term_type == TERM_DATE)
				{
					if(std::strcmp(left.val_s, val->val_s) == 0)
						return true;
				} else {
					THROW_TYPE_INCOMPATIBLE;
				}
				break;
			case TERM_DATE:
				if(val->term_type == TERM_DATE)
				{
					expression right = eval_terminal(val);
					if(left.val_i == right.val_i)
						return true;
				} else {
					THROW_TYPE_INCOMPATIBLE;
				}
				break;
			case TERM_NULL:
				return false;
			default: THROW_TYPE_INCOMPATIBLE;
		}
	}

	return false;
}

expression expression::eval(const expr_node_t *expr)
{
	assert(expr != nullptr);
	if(expr->op == OPERATOR_NONE)
	{
		// terminator
		return eval_terminal(expr);
	}

	assert(expr->term_type == TERM_NONE);

	// non-terminator
	bool is_unary = (expr->op & OPERATOR_UNARY);
	expression left  = eval(expr->left);
	expression right = is_unary ? expression() : eval(expr->right);

	if(!is_unary && right.type == TERM_NULL)
	{
		expression ret;
		ret.type = TERM_NULL;
		return ret;
	}

	if(expr->op == OPERATOR_IN)
	{
		bool ret = eval_in_expression(left, right.literal_list);
		expression expr;
		expr.type = TERM_BOOL;
		expr.val_b = ret;
		return expr;
	}

	if(!is_unary && left.type != right.type && left.type != TERM_NULL)
		throw "[Error] operand type mismatch.";
	
	switch(left.type)
	{
		case TERM_INT:
			return eval_int_operands(expr->op, left.val_i, right.val_i);
		case TERM_FLOAT:
			return eval_float_operands(expr->op, left.val_f, right.val_f);
		case TERM_DATE:
			return eval_date_operands(expr->op, left.val_i, right.val_i);
		case TERM_BOOL:
			return eval_bool_operands(expr->op, left.val_b, right.val_b);
		case TERM_STRING:
			return eval_string_operands(expr->op, left.val_s, right.val_s);
		case TERM_NULL:
			return eval_null_operands(expr->op);
		default:
			throw "[Error] unknown type.";
			return expression();
	}
}

bool expression::is_aggregate(const expr_node_t* expr)
{
	return expr->op == OPERATOR_COUNT
		|| expr->op == OPERATOR_SUM
		|| expr->op == OPERATOR_AVG
		|| expr->op == OPERATOR_MIN
		|| expr->op == OPERATOR_MAX;
}

std::string expression::to_string(const expr_node_t *expr)
{
	if(!expr) return "*";
	if(expr->op == OPERATOR_NONE)
	{
		// terminator
		switch(expr->term_type)
		{
			case TERM_INT: {
				std::ostringstream ss;
				ss << expr->val_f;
				return ss.str(); }
			case TERM_FLOAT: {
				std::ostringstream ss;
				ss << expr->val_i;
				return ss.str(); }
			case TERM_BOOL:
				return expr->val_b ? "TRUE" : "FALSE";
			case TERM_STRING:
				return "'" + std::string(expr->val_s) + "'";
			case TERM_COLUMN_REF:
				if(expr->column_ref->table)
				{
					return std::string(expr->column_ref->table)
						+ "." + expr->column_ref->column;
				} else {
					return expr->column_ref->column;
				}
			case TERM_NULL:
				return "NULL";
			default:
				return "";
		}
	} else {
		// non-terminator
		std::string str = to_string(expr->left);
		if(expr->op & OPERATOR_UNARY)
		{
			switch(expr->op)
			{
				case OPERATOR_SUM:
					return "SUM(" + str + ")";
				case OPERATOR_AVG:
					return "AVG(" + str + ")";
				case OPERATOR_MIN:
					return "MIN(" + str + ")";
				case OPERATOR_MAX:
					return "MAX(" + str + ")";
				case OPERATOR_COUNT:
					return "COUNT(" + str + ")";
				default:
					return str;
			}
		}

		switch(expr->op)
		{
			case OPERATOR_ADD:
				str += '+';
				break;
			case OPERATOR_MINUS:
				str += '-';
				break;
			case OPERATOR_DIV:
				str += '/';
				break;
			case OPERATOR_MUL:
				str += '*';
				break;
			default:
				str += ' ';
				break;
		}

		return str + to_string(expr->right);
	}
}

