#include <cstring>
#include <cstdio>
#include <sstream>
#include "table_header.h"
#include "../utils/type_cast.h"
#include "../expression/expression.h"

bool fill_table_header(table_header_t *header, const table_def_t *table)
{
	std::memset(header, 0, sizeof(table_header_t));
	std::strncpy(header->table_name, table->name, MAX_NAME_LEN);
	int offset = 8;  // 4 bytes for __rowid__, and 4 bytes for not null
	for(field_item_t *field = table->fields; field; field = field->next)
	{
		int index = header->col_num++;
		std::strncpy(header->col_name[index], field->name, MAX_NAME_LEN);
		switch(field->type)
		{
			case FIELD_TYPE_INT:
				header->col_type[index]   = COL_TYPE_INT;
				header->col_length[index] = 4;
				break;
			case FIELD_TYPE_FLOAT:
				header->col_type[index]   = COL_TYPE_FLOAT;
				header->col_length[index] = 4;
				break;
			case FIELD_TYPE_DATE:
				header->col_type[index]   = COL_TYPE_DATE;
				header->col_length[index] = 4;
				break;
			case FIELD_TYPE_CHAR:
			case FIELD_TYPE_VARCHAR:
				header->col_type[index]   = COL_TYPE_VARCHAR;
				header->col_length[index] = field->width + 1;
				break;
			default:
				std::fprintf(stderr, "[Error] Unsupported type.\n");
				break;
		}

		header->col_offset[index] = offset;
		offset += header->col_length[index];
		if(field->flags & FIELD_FLAG_NOTNULL)
			header->flag_notnull |= 1 << index;
		if(field->flags & FIELD_FLAG_PRIMARY)
			header->flag_primary |= 1 << index;
		if(field->flags & FIELD_FLAG_UNIQUE)
			header->flag_unique |= 1 << index;
		if(field->default_value != nullptr)
		{
			if(header->col_length[index] > MAX_DEFAULT_LEN)
			{
				std::fprintf(stderr, "[Error] Field too long to have default value.\n");
				return false;
			}

			header->flag_default |= 1 << index;
			expression expr = expression::eval(field->default_value);
			auto desired_type = typecast::column_to_term(header->col_type[index]);
			char *data = typecast::expr_to_db(expr, desired_type);
			if(desired_type == TERM_STRING)
			{
				std::strcpy(header->default_values[index], data);
			} else {
				std::memcpy(header->default_values[index], data, header->col_length[index]);
			}
		}
	}

	auto lookup_column = [&](const char *name) -> int {
		for(int i = 0; i != header->col_num; ++i)
		{
			if(std::strcmp(name, header->col_name[i]) == 0)
				return i;
		}

		std::fprintf(stderr, "[Error] Column `%s` not found.\n", name);
		return -1;
	};

	/* resolve constraint field */
	for(linked_list_t *link_ptr = table->constraints; link_ptr; link_ptr = link_ptr->next)
	{
		int cid;
		table_constraint_t *constraint = (table_constraint_t*)link_ptr->data;
		std::ostringstream os;
		switch(constraint->type)
		{
			case TABLE_CONSTRAINT_UNIQUE:
				cid = lookup_column(constraint->column_ref->column);
				if(cid < 0) return false;
				header->flag_unique |= 1 << cid;
				break;
			case TABLE_CONSTRAINT_PRIMARY_KEY:
				cid = lookup_column(constraint->column_ref->column);
				if(cid < 0) return false;
				header->flag_primary |= 1 << cid;
				break;
			case TABLE_CONSTRAINT_FOREIGN_KEY:
				cid = lookup_column(constraint->column_ref->column);
				if(cid < 0) return false;
				header->foreign_key[header->foreign_key_num] = cid;
				std::strcpy(header->foreign_key_ref_table[header->foreign_key_num],
						constraint->foreign_column_ref->table);
				std::strcpy(header->foreign_key_ref_column[header->foreign_key_num],
						constraint->foreign_column_ref->column);
				++header->foreign_key_num;
				break;
			case TABLE_CONSTRAINT_CHECK:
				if(header->check_constaint_num > MAX_CHECK_CONSTRAINT_NUM)
				{
					std::fprintf(stderr, "[Error] Too many check constraints.\n");
					return false;
				}

				expression::dump_exprnode(os, constraint->check_cond);
				if(os.str().size() >= MAX_CHECK_CONSTRAINT_LEN)
				{
					std::fprintf(stderr, "[Error] Check constraint too long.\n");
					return false;
				}

				std::strcpy(header->check_constaints[header->check_constaint_num++], os.str().c_str());
				break;
			default:
				std::fprintf(stderr, "[Error] Unsupported constraint.\n");
				return false;
		}
	}

	/* add '__rowid__' column (with highest index) */
	int index = header->col_num++;
	std::strcpy(header->col_name[index], "__rowid__");
	header->col_type[index]   = COL_TYPE_INT;
	header->col_length[index] = 4;
	header->col_offset[index] = 0;
	header->main_index = index;
	header->is_main_index_additional = 1;
	header->flag_indexed |= 1 << index;
	if(!header->flag_primary)
		header->flag_primary |= 1 << index;
	// add index to unique constrainted columns
	header->flag_indexed |= header->flag_unique;
	header->flag_notnull |= header->flag_primary;

	// add index to the first primary key column
	int first_primary = 0;
	for(; !(header->flag_primary & (1u << first_primary)); ++first_primary);
	header->flag_indexed |= 1u << first_primary;
	header->auto_inc = 1;

	header->primary_key_num = 0;
	for(int i = 0; i != header->col_num; ++i)
		if(header->flag_primary & (1u << i))
			++header->primary_key_num;

	return true;
}

void table_header_t::dump()
{
	std::printf("======== Table Info Begin ========\n");
	std::printf("Table name  = %s\n", table_name);
	std::printf("Column size = %d\n", col_num);
	std::printf("Record size = %d\n", records_num);
	for(int i = 0; i != col_num; ++i)
	{
		std::printf("  [column] name = %s, type = ", col_name[i]);
		switch(col_type[i])
		{
			case COL_TYPE_INT:
				std::printf("INT");
				break;
			case COL_TYPE_FLOAT:
				std::printf("FLOAT");
				break;
			case COL_TYPE_DATE:
				std::printf("DATE");
				break;
			case COL_TYPE_VARCHAR:
				std::printf("VARCHAR(%d)", col_length[i]);
				break;
			default:
				std::printf("UNKNOWN");
				break;
		}

		std::printf(", flags = ");
		if(flag_notnull & (1 << i))
			std::printf("NOT_NULL ");
		if(flag_primary & (1 << i))
			std::printf("PRIMARY ");
		if(flag_unique & (1 << i))
			std::printf("UNIQUE ");
		if(flag_indexed & (1 << i))
			std::printf("INDEXED ");
		std::puts("");
	}

	for(int i = 0; i != foreign_key_num; ++i)
	{
		std::printf("  [foreign key] %s references %s.%s\n",
			col_name[foreign_key[i]],
			foreign_key_ref_table[i],
			foreign_key_ref_column[i]
		);
	}

	std::printf("======== Table Info End   ========\n");
}
