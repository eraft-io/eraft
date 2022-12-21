#ifndef __TRIVIALDB_EXPRESSION__
#define __TRIVIALDB_EXPRESSION__

#include "../parser/defs.h"
#include <string>
#include <iostream>

struct expression
{
	union {
		char *val_s;
		int   val_i;
		float val_f;
		bool  val_b;
		linked_list_t *literal_list;
	};

	term_type_t type;

	static expression eval(const expr_node_t *expr);
	static std::string to_string(const expr_node_t *expr);
	static bool is_aggregate(const expr_node_t *expr);
	static void cache_clear();
	static void cache_clear(const char *table);
	static void cache_replace(const char *table, const char* col, expression expr);
	static void cache_column(const char *table, const char *col, const expression &expr);

	static void dump_exprnode(std::ostream &os, const expr_node_t *expr);
	static expr_node_t* load_exprnode(std::istream &is);
	static void free_exprnode(expr_node_t *expr);
};

#endif
