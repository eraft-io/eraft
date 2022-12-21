#include <cassert>
#include <cstring>
#include <string>
#include "expression.h"

void expression::dump_exprnode(std::ostream &os, const expr_node_t *expr)
{
	int count = 0;
	os << expr->op << " ";
	if(expr->op == OPERATOR_NONE)
	{
		os << expr->term_type << " ";
		switch(expr->term_type)
		{
			case TERM_INT:
				os << expr->val_i << " ";
				break;
			case TERM_FLOAT:
				os << expr->val_f << " ";
				break;
			case TERM_STRING:
				os << expr->val_s << " ";
				break;
			case TERM_BOOL:
				os << expr->val_b << " ";
				break;
			case TERM_NULL:
				break;
			case TERM_COLUMN_REF:
				if(expr->column_ref->table)
				{
					os << 2 << " "
					   << expr->column_ref->table << " "
					   << expr->column_ref->column << " ";
				} else {
					os << 1 << " "
					   << expr->column_ref->column << " ";
				}
				break;
			case TERM_LITERAL_LIST:
				for(linked_list_t *l_ptr = expr->literal_list; l_ptr; l_ptr = l_ptr->next)
					++count;
				os << count << " ";
				for(linked_list_t *l_ptr = expr->literal_list; l_ptr; l_ptr = l_ptr->next)
					dump_exprnode(os, (expr_node_t*)l_ptr->data);
				break;
			default:
				assert(0);
				break;
		}
	} else {
		bool is_unary = (expr->op & OPERATOR_UNARY);
		dump_exprnode(os, expr->left);
		if(!is_unary) dump_exprnode(os, expr->right);
	}
}

char *load_string(std::istream &is)
{
	std::string str;
	is >> str;
	char *buf = (char*)malloc(str.size() + 1);
	std::strcpy(buf, str.c_str());
	return buf;
}

expr_node_t* expression::load_exprnode(std::istream &is)
{
	expr_node_t *expr = (expr_node_t*)calloc(1, sizeof(expr_node_t));
	int tmp;
	is >> tmp;
	expr->op = (operator_type_t)tmp;
	if(expr->op == OPERATOR_NONE)
	{
		is >> tmp;
		expr->term_type = (term_type_t)tmp;
		switch(expr->term_type)
		{
			case TERM_INT:
				is >> expr->val_i;
				break;
			case TERM_FLOAT:
				is >> expr->val_f;
				break;
			case TERM_STRING:
				expr->val_s = load_string(is);
				break;
			case TERM_BOOL:
				is >> expr->val_b;
				break;
			case TERM_NULL:
				break;
			case TERM_COLUMN_REF:
				is >> tmp;
				expr->column_ref = (column_ref_t*)malloc(sizeof(column_ref_t));
				if(tmp == 2)
				{
					expr->column_ref->table = load_string(is);
					expr->column_ref->column = load_string(is);
				} else {
					expr->column_ref->table = nullptr;
					expr->column_ref->column = load_string(is);
				}
				break;
			case TERM_LITERAL_LIST:
				is >> tmp;
				expr->literal_list = NULL;
				for(int i = 0; i != tmp; ++i)
				{
					linked_list_t *l_ptr = (linked_list_t*)malloc(sizeof(linked_list_t));
					l_ptr->next = expr->literal_list;
					l_ptr->data = load_exprnode(is);
					expr->literal_list = l_ptr;
				}
				break;
			default:
				assert(0);
				break;
		}
	} else {
		bool is_unary = (expr->op & OPERATOR_UNARY);
		expr->left = load_exprnode(is);
		if(!is_unary) expr->right = load_exprnode(is);
	}

	return expr;
}
