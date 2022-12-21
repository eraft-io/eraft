#ifndef __TRIVIALDB_DBMS_TYPECAST__
#define __TRIVIALDB_DBMS_TYPECAST__
#include <cassert>
#include <iomanip>
#include "../expression/expression.h"
#include "../defs.h"
#include "../parser/defs.h"

namespace typecast
{
	inline term_type_t column_to_term(int type)
	{
		switch(type)
		{
			case COL_TYPE_INT:
				return TERM_INT;
			case COL_TYPE_FLOAT:
				return TERM_FLOAT;
			case COL_TYPE_DATE:
				return TERM_DATE;
			case COL_TYPE_VARCHAR:
				return TERM_STRING;
			default:
				throw "[Error] wrong datatype.";
		}
	}

	inline expression column_to_expr(char *data, int type)
	{
		expression expr;
		if(data == nullptr)
		{
			expr.type = TERM_NULL;
			return expr;
		}

		switch(type)
		{
			case COL_TYPE_INT:
				expr.val_i = *(int*)data;
				expr.type = TERM_INT;
				break;
			case COL_TYPE_FLOAT:
				expr.val_f = *(float*)data;
				expr.type = TERM_FLOAT;
				break;
			case COL_TYPE_DATE:
				expr.val_i = *(int*)data;
				expr.type = TERM_DATE;
				break;
			case COL_TYPE_VARCHAR:
				expr.val_s = data;
				expr.type = TERM_STRING;
				break;
			default:
				throw "[Error] wrong datatype.";
		}
		return expr;
	}

	inline bool expr_to_bool(const expression &expr)
	{
		switch(expr.type)
		{
			case TERM_INT:
				return expr.val_i != 0;
			case TERM_FLOAT:
				return expr.val_f != 0;
			case TERM_BOOL:
				return expr.val_b;
			case TERM_STRING:
				return expr.val_s[0] != 0;  // not empty string
			case TERM_NULL:
				return false;
			default:
				return false;
		}
	}

	inline char *expr_to_db(expression &expr, term_type_t desired)
	{
		static char buf[32];
		char *ret = nullptr;
		switch(expr.type)
		{
			case TERM_INT:
				if(desired == TERM_FLOAT)
				{
					expr.val_f = expr.val_i;
					ret = (char*)&expr.val_f;
				} else {
					ret = (char*)&expr.val_i;
				}
				break;
			case TERM_FLOAT:
				if(desired == TERM_INT)
				{
					expr.val_i = expr.val_f;
					ret = (char*)&expr.val_i;
				} else {
					ret = (char*)&expr.val_f;
				}
				break;
			case TERM_STRING:
				ret = expr.val_s;
				break;
			case TERM_NULL:
				ret = nullptr;
				break;
			case TERM_DATE:
				if(desired == TERM_STRING)
				{
					time_t time = expr.val_i;
					auto tm = std::localtime(&time);
					std::strftime(buf, 32, DATE_TEMPLATE, tm);
					ret = buf;
				} else {
					ret = (char*)&expr.val_i;
				}
				break;
			default:
				throw "[Error] unknown type.";
		}

		return ret;
	}

	inline bool type_compatible(int col_type, const expression &val)
	{
		switch(val.type)
		{
			case TERM_NULL:
				return true;
			case TERM_INT:
				return col_type == COL_TYPE_INT || col_type == COL_TYPE_FLOAT;
			case TERM_FLOAT:
				return col_type == COL_TYPE_FLOAT;
			case TERM_STRING:
				return col_type == COL_TYPE_VARCHAR;
			case TERM_DATE:
				return col_type == COL_TYPE_DATE || col_type == COL_TYPE_VARCHAR;
			default:
				return false;
		}
	}
}

#endif
