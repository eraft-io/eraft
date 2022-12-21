#include "dbms.h"
#include "database.h"
#include "../table/table.h"
#include "../index/index.h"
#include "../expression/expression.h"
#include "../utils/type_cast.h"
#include "../table/record.h"
#include "../../network/field.h"
#include "../../network/eof.h"
#include "../../network/ok.h"

#include <vector>
#include <limits>
#include <algorithm>
#include <stdio.h>
#include <iostream>
struct __cache_clear_guard
{
	~__cache_clear_guard() { expression::cache_clear(); }
};

dbms::dbms()
	: output_file(stdout), cur_db(nullptr)
{

}

dbms::~dbms()
{
	close_database();
}

void dbms::switch_select_output(const char *filename)
{
	// 存在 data 子目录下
	std::string PATH = "data/" + std::string(filename);
	if(output_file != stdout)
		std::fclose(output_file);
	if(std::strcmp(filename, "stdout") == 0)
		output_file = stdout;
	else output_file = std::fopen(PATH.c_str(), "w");
}

template<typename Callback>
void dbms::iterate(
	std::vector<table_manager*> required_tables,
	expr_node_t *cond,
	Callback callback)
{
	if(required_tables.size() == 1)
	{
		std::vector<record_manager*> rm_list(1);
		std::vector<int> rid_list(1);
		iterate_one_table_with_index(required_tables[0], cond, [&](table_manager *, record_manager *rm, int rid) -> bool {
			rm_list[0] = rm;
			rid_list[0] = rid;
			return callback(required_tables, rm_list, rid_list);
		} );
	} else {
		iterate_many_tables(required_tables, cond, callback);
		std::puts("[Info] Join many tables by enumerating.");
	}
}

template<typename Callback>
bool dbms::iterate_one_table_with_index(
		table_manager* table,
		expr_node_t *cond,
		Callback callback)
{
	std::vector<expr_node_t*> and_cond;
	extract_and_cond(cond, and_cond);
	expr_node_t *index_cond = nullptr;
	index_manager *index = nullptr;

	auto get_index = [&](column_ref_t *col) -> index_manager*
	{
		int cid = table->lookup_column(col->column);
		if(cid < 0) return nullptr;
		return table->get_index(cid);
	};

	for(expr_node_t *expr : and_cond)
	{
		if(expr->op == OPERATOR_EQ)
		{
			if(expr->right->term_type == TERM_COLUMN_REF)
				std::swap(expr->right, expr->left);

			if(expr->left->term_type == TERM_COLUMN_REF && expr->right->term_type != TERM_COLUMN_REF)
			{
				index = get_index(expr->left->column_ref);
				if(index)
				{
					index_cond = expr;
					break;
				}
			}
		}
	}

	if(!index_cond)
	{
		iterate_one_table(table, cond, callback);
		return false;
	}

	char *key = nullptr;
	switch(index_cond->right->term_type)
	{
		case TERM_INT:
		case TERM_DATE:
			key = (char*)&index_cond->right->val_i;
			break;
		case TERM_FLOAT:
			key = (char*)&index_cond->right->val_f;
			break;
		case TERM_STRING:
			key = index_cond->right->val_s;
			break;
		case TERM_BOOL:
			key = (char*)&index_cond->right->val_b;
			break;
		default:
			break;
	}

	auto it = index->get_iterator_lower_bound(key);
	for(; !it.is_end(); it.next())
	{
		int rid;
		record_manager rm = table->open_record_from_index_lower_bound(it.get(), &rid);
		table->cache_record(&rm);

		bool join_ret = false;
		try {
			join_ret = typecast::expr_to_bool(expression::eval(index_cond));
		} catch(const char *msg) {
			std::puts(msg);
			iterate_one_table(table, cond, callback);
			return false;
		}

		if(!join_ret) break;

		if(!callback(table, &rm, rid))
			break;
	}

	return true;
}

template<typename Callback>
void dbms::iterate_one_table(
		table_manager* table,
		expr_node_t *cond,
		Callback callback)
{
	auto bit = table->get_record_iterator_lower_bound(0);
	for(; !bit.is_end(); bit.next())
	{
		int rid;
		record_manager rm(bit.get_pager());
		rm.open(bit.get(), false);
		rm.read(&rid, 4);
		table->cache_record(&rm);
		if(cond)
		{
			bool result = false;
			try {
				result = typecast::expr_to_bool(expression::eval(cond));
			} catch(const char *msg) {
				std::puts(msg);
				return;
			}

			if(!result) continue;
		}

		if(!callback(table, &rm, rid))
			break;
	}
}

void dbms::extract_and_cond(expr_node_t *cond, std::vector<expr_node_t*> &and_cond)
{
	if(!cond) return;
	if(cond->op == OPERATOR_AND)
	{
		extract_and_cond(cond->left, and_cond);
		extract_and_cond(cond->right, and_cond);
	} else {
		and_cond.push_back(cond);
	}
}

template<typename Callback>
void dbms::iterate_many_tables(
	const std::vector<table_manager*> &table_list,
	expr_node_t *cond, Callback callback)
{
	std::vector<record_manager*> record_list(table_list.size());
	std::vector<int> rid_list(table_list.size());
	std::vector<expr_node_t*> and_cond;
	extract_and_cond(cond, and_cond);
	auto lookup_table = [&](const char *name) {
		for(int i = 0; i < (int)table_list.size(); ++i)
			if(std::strcmp(name, table_list[i]->get_table_name()) == 0)
				return i;
		return -1;
	};

	// edge
	std::vector<std::vector<int>> E(table_list.size());
	// join cond
	std::vector<std::vector<expr_node_t*>> J(table_list.size());
	for(auto &v : E) v.resize(table_list.size());
	for(auto &v : J) v.resize(table_list.size());

	// setup join condition graph
	for(expr_node_t *c : and_cond)
	{
		if(c->op == OPERATOR_EQ &&
				c->left->term_type == TERM_COLUMN_REF &&
				c->right->term_type == TERM_COLUMN_REF)
		{
			int tid1 = lookup_table(c->left->column_ref->table);
			int tid2 = lookup_table(c->right->column_ref->table);
			if(tid1 == -1 || tid2 == -1)
			{
				std::fprintf(stderr, "[Error] Table not found!\n");
				return;
			}

			table_manager *tb1 = table_list[tid1];
			table_manager *tb2 = table_list[tid2];

			int cid1 = tb1->lookup_column(c->left->column_ref->column);
			int cid2 = tb2->lookup_column(c->right->column_ref->column);
			if(cid1 == -1 || cid2 == -1)
			{
				std::fprintf(stderr, "[Error] Column not found!\n");
				return;
			}

			index_manager *idx1 = tb1->get_index(cid1);
			index_manager *idx2 = tb2->get_index(cid2);
			if(!idx1 && !idx2)
				continue;

			if(idx2)
			{
				E[tid2][tid1] = 1;
				J[tid2][tid1] = c;
			}

			if(idx1)
			{
				E[tid1][tid2] = 1;
				J[tid1][tid2] = c;
			}
		}
	}

	// find the longest path
	int *mark = new int[table_list.size()];
	int *path = new int[table_list.size()];
	int max_depth = 0, start = 0;
	for(int i = 0; i < (int)table_list.size(); ++i)
	{
		int m = 0;
		std::memset(mark, 0, table_list.size() * sizeof(int));
		find_longest_path(i, 0, mark, path, E, ~0u >> 1, m);
		if(m > max_depth)
		{
			max_depth = m;
			start = i;
		}
	}

	int _;
	std::memset(mark, 0, table_list.size() * sizeof(int));
	_ = find_longest_path(start, 0, mark, path, E, max_depth, _);
	assert(_);

	// generate iteration sequence
	std::memset(mark, 0, table_list.size() * sizeof(int));
	for(int i = 0; i <= max_depth; ++i)
		mark[path[i]] = 1;

	int cur = max_depth, len = table_list.size();
	for(int i = 0; i < len; ++i)
		if(!mark[i])
			path[++cur] = i;

	// setup iteration variable
	index_manager **index_ref = new index_manager*[len];
	int *index_cid = new int[len];
	std::fill(index_ref, index_ref + len, nullptr);
	std::fill(index_cid, index_cid + len, -1);

	for(int i = 0; i < max_depth; ++i)
	{
		expr_node_t *join_node = J[path[i]][path[i + 1]];
		if(std::strcmp(join_node->left->column_ref->table, table_list[path[i]]->get_table_name()) == 0)
		{
			index_cid[i] = table_list[path[i + 1]]->lookup_column(
					join_node->right->column_ref->column);
			index_ref[i] = table_list[path[i]]->get_index(
					table_list[path[i]]->lookup_column(
						join_node->left->column_ref->column)
					);
		} else {
			index_cid[i] = table_list[path[i + 1]]->lookup_column(
					join_node->left->column_ref->column);
			index_ref[i] = table_list[path[i]]->get_index(
					table_list[path[i]]->lookup_column(
						join_node->right->column_ref->column)
					);
		}

		assert(index_ref[i]);
	}

	iterate_many_tables_impl(
		table_list, record_list, rid_list,
		J, path, index_cid, index_ref,
		cond, callback, len - 1);

	// debug info
	std::printf("[Info] Iteration order: ");
	for(int i = 0; i < len; ++i)
	{
		if(i != 0) std::printf(", ");
		std::printf("%s", table_list[path[len - i - 1]]->get_table_name());
	}

	std::printf("\n[Info] Index use: ");
	for(int i = 0; i < max_depth; ++i)
	{
		if(i != 0) std::printf(", ");
		expr_node_t *node = J[path[i]][path[i + 1]];
		std::printf("%s.%s-%s.%s",
			node->left->column_ref->table,
			node->left->column_ref->column,
			node->right->column_ref->table,
			node->right->column_ref->column
		);
	}

	std::puts("");

	delete []mark;
	delete []path;
	delete []index_cid;
	delete []index_ref;
}

template<typename Callback>
bool dbms::iterate_many_tables_impl(
	const std::vector<table_manager*> &table_list,
	std::vector<record_manager*> &record_list,
	std::vector<int> &rid_list,
	std::vector<std::vector<expr_node_t*>> &index_cond,
	int *iter_order, int *index_cid, index_manager** index,
	expr_node_t *cond, Callback callback, int now)
{
	if(now < 0)
	{
		if(cond)
		{
			bool result = false;
			try {
				result = typecast::expr_to_bool(expression::eval(cond));
			} catch(const char *msg) {
				std::puts(msg);
				return false; // stop
			}

			if(!result)
				return true; // continue
		}

		if(!callback(table_list, record_list, rid_list))
			return false;  // stop
		return true;  // continue
	} else {
		if(!index[now])
		{
			auto it = table_list[iter_order[now]]->get_record_iterator_lower_bound(0);
			for(; !it.is_end(); it.next())
			{
				record_manager rm(it.get_pager());
				rm.open(it.get(), false);
				rm.read(&rid_list[iter_order[now]], 4);
				// std::printf("%d\n", rid_list[iter_order[now]]);
				table_list[iter_order[now]]->cache_record(&rm);
				record_list[iter_order[now]] = &rm;
				bool ret = iterate_many_tables_impl(
					table_list, record_list, rid_list,
					index_cond, iter_order, index_cid, index,
					cond, callback, now - 1
				);

				if(!ret) return false;
			}
		} else {
			const char *tb_col = table_list[iter_order[now + 1]]->get_cached_column(index_cid[now]);
			table_manager *tb2 = table_list[iter_order[now]];
			auto tb2_it = index[now]->get_iterator_lower_bound(tb_col);
			for(; !tb2_it.is_end(); tb2_it.next())
			{
				int tb2_rid;
				record_manager tb2_rm = tb2->open_record_from_index_lower_bound(tb2_it.get(), &tb2_rid);
				tb2->cache_record(&tb2_rm);

				bool join_ret = false;
				try {
					expr_node_t *join_cond = index_cond[iter_order[now]][iter_order[now + 1]];
					join_ret = typecast::expr_to_bool(expression::eval(join_cond));
				} catch(const char *msg) {
					std::puts(msg);
					return false;
				}

				if(!join_ret) break;

				rid_list[iter_order[now]] = tb2_rid;
				record_list[iter_order[now]] = &tb2_rm;
				bool ret = iterate_many_tables_impl(
					table_list, record_list, rid_list,
					index_cond, iter_order, index_cid, index,
					cond, callback, now - 1
				);

				if(!ret) return false;
			}
		}
	}

	return true;
}

void dbms::close_database()
{
	if(cur_db)
	{
		cur_db->close();
		delete cur_db;
		cur_db = nullptr;
	}
	std::printf("call closed db ~");
}

void dbms::switch_database(const char *db_name, Client* cli, const char *pkt)
{
	if(cur_db)
	{
		cur_db->close();
		delete cur_db;
		cur_db = nullptr;
	}
	cur_db = new database();
	cur_db->open(db_name);
	std::vector<uint8_t> OkPacket = {7, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0};
	OkPacket[3] = pkt[3] + 1;
	UnboundedBuffer reply_;
	reply_.PushData(std::string(OkPacket.begin(), OkPacket.end()).c_str(),
					OkPacket.size());
	cli->SendPacket(reply_);
	printf("OK!\n");
}

void dbms::create_database(const char *db_name, Client* cli, const char *pkt)
{
	database db;
	db.create(db_name);
	db.close();
	switch_select_output(db_name);
	std::vector<uint8_t> OkPacket = {7, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0};
	OkPacket[3] = pkt[3] + 1;
	UnboundedBuffer reply_;
	reply_.PushData(std::string(OkPacket.begin(), OkPacket.end()).c_str(),
					OkPacket.size());
	cli->SendPacket(reply_);
}

void dbms::drop_database(const char *db_name, Client* cli, const char *pkt)
{
	if(cur_db && std::strcmp(cur_db->get_name(), db_name) == 0)
	{
		cur_db->close();
		delete cur_db;
		cur_db = nullptr;
	}
	database db;
	db.open(db_name);
	db.drop();
	std::vector<uint8_t> OkPacket = {7, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0};
	OkPacket[3] = pkt[3] + 1;
	UnboundedBuffer reply_;
	reply_.PushData(std::string(OkPacket.begin(), OkPacket.end()).c_str(),
					OkPacket.size());
	cli->SendPacket(reply_);
}

void dbms::show_database(const char *db_name, Client* cli, const char *pkt)
{
	database db;
	db.open(db_name);
	db.show_info();
	auto dbInfo = db.get_db_info();
	// pack seq
	uint8_t seq = 1;
	// 1.field count pack
	std::vector<uint8_t> out_pack;
	out_pack.push_back(1);
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.push_back(static_cast<uint8_t>(2));
	UnboundedBuffer reply_;
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();
	out_pack.clear();
	// 2.table header
	Protocol::FieldPacket new_field_pack(std::string("Item"), static_cast< uint32_t >(6165), std::string(dbInfo.db_name),
              std::string(dbInfo.db_name), std::string(dbInfo.db_name), std::string(dbInfo.db_name),
              80, 33, 0, 0);
	seq ++;
	std::vector< uint8_t > field_pack = new_field_pack.Pack();
	out_pack.push_back(field_pack.size());
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.insert(
		out_pack.end(),
		field_pack.begin(),
		field_pack.end()
	);
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();
	out_pack.clear();

	Protocol::FieldPacket new_field_pack_1(std::string("Value"), static_cast< uint32_t >(6165), std::string(dbInfo.db_name),
              std::string(dbInfo.db_name), std::string(dbInfo.db_name), std::string(dbInfo.db_name),
              80, 33, 0, 0);
	seq ++;
	std::vector< uint8_t > field_pack_1 = new_field_pack_1.Pack();
	out_pack.push_back(field_pack_1.size());
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.insert(
		out_pack.end(),
		field_pack_1.begin(),
		field_pack_1.end()
	);
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();
	out_pack.clear();
	// 3.eof 
	Protocol::EofPacket eof(0, 2);
    std::vector< uint8_t > eof_packet = eof.Pack();
	seq ++; // 254, 0, 0, 2, 0
	out_pack.push_back(eof_packet.size());
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.insert(
		out_pack.end(),
		eof_packet.begin(),
		eof_packet.end()
	);
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();
	out_pack.clear();
	// 4.rows
	// db name
	seq ++;
	auto row_val = std::vector<std::string>{std::string("DATABASE NAME"), std::string(dbInfo.db_name)};
	Protocol::RowPacket row_pack1(row_val);
	std::vector< uint8_t > row_packet1 = row_pack1.Pack();
	out_pack.push_back(row_packet1.size());
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.insert(
		out_pack.end(),
		row_packet1.begin(),
		row_packet1.end()
	);
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();
	out_pack.clear();
	// table count
	seq ++;
	row_val = std::vector<std::string>{std::string("TABLE COUNT"), std::to_string(db.get_tab_num())};
	Protocol::RowPacket row_pack2(row_val);
	std::vector< uint8_t > row_packet2 = row_pack2.Pack();
	out_pack.push_back(row_packet2.size());
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.insert(
		out_pack.end(),
		row_packet2.begin(),
		row_packet2.end()
	);
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();
	out_pack.clear();

	// 5.eof
	seq ++; // 254, 0, 0, 2, 0
	out_pack.push_back(eof_packet.size());
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.insert(
		out_pack.end(),
		eof_packet.begin(),
		eof_packet.end()
	);
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();
}

void dbms::drop_table(const char *table_name)
{
	if(assert_db_open())
		cur_db->drop_table(table_name);
	printf("OK!\n");
}

void dbms::show_table(const char* table_name)
{
	if(assert_db_open())
	{
		table_manager *tm = cur_db->get_table(table_name);
		if(tm == nullptr)
		{
			std::fprintf(stderr, "[Error] Table `%s` not found.\n", table_name);
		} else {
			tm->dump_table_info();
		}
	}
}

void dbms::create_table(const table_header_t *header, Client* cli, const char *pkt)
{
	if(assert_db_open())
		cur_db->create_table(header);
	std::vector<uint8_t> OkPacket = {7, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0};
	OkPacket[3] = pkt[3] + 1;
	UnboundedBuffer reply_;
	reply_.PushData(std::string(OkPacket.begin(), OkPacket.end()).c_str(),
					OkPacket.size());
	cli->SendPacket(reply_);
}

void dbms::update_rows(const update_info_t *info, Client* cli, const char *pkt)
{
	if(!assert_db_open())
		return;

	__cache_clear_guard __guard;
	table_manager *tm = cur_db->get_table(info->table);
	if(tm == nullptr)
	{
		std::fprintf(stderr, "[Error] table `%s` doesn't exists.\n", info->table);
		return;
	}

	int col_id = tm->lookup_column(info->column_ref->column);
	if(col_id < 0)
	{
		std::fprintf(stderr, "[Error] column `%s' not exists.\n", info->column_ref->column);
		return;
	}

	int succ_count = 0, fail_count = 0;
	try {
		iterate_one_table(tm, info->where, [&](table_manager *tm, record_manager *, int rid) -> bool {
			expression val = expression::eval(info->value);
			int col_type = tm->get_column_type(col_id);
			if(!typecast::type_compatible(col_type, val))
				throw "[Error] Incompatible data type.";
			auto term_type = typecast::column_to_term(col_type);
			bool ret = tm->modify_record(rid, col_id, typecast::expr_to_db(val, term_type));
			succ_count += ret;
			fail_count += 1 - ret;
			return true;
		} );
	} catch(const char *msg) {
		std::puts(msg);
		return;
	} catch(...) {
	}
	Protocol::OkPacket ok_pack;
    std::vector<uint8_t> ok_packed = ok_pack.Pack(succ_count, 0, 2, 0);
	std::vector< uint8_t > res;
	res.push_back(ok_packed.size());
	res.push_back(0);
	res.push_back(0);
	res.push_back(1);
	res.insert(
		res.end(),
		ok_packed.begin(),
		ok_packed.end()
	);
	UnboundedBuffer reply_;
	reply_.PushData(std::string(res.begin(), res.end()).c_str(),
					res.size());
	cli->SendPacket(reply_);
	std::printf("[Info] %d row(s) updated, %d row(s) failed.\n",
			succ_count, fail_count);
}

void dbms::select_rows(const select_info_t *info, Client* cli, const char *pkt)
{
	if(!assert_db_open())
		return;

	__cache_clear_guard __guard;

	// get required tables
	std::vector<std::shared_ptr<table_manager>> alias_tables;
	std::vector<table_manager*> required_tables;
	for(linked_list_t *table_l = info->tables; table_l; table_l = table_l->next)
	{
		table_join_info_t *table_info = (table_join_info_t*)table_l->data;
		table_manager *tm = cur_db->get_table(table_info->table);
		if(tm == nullptr)
		{
			std::fprintf(stderr, "[Error] table `%s` doesn't exists.\n", table_info->table);
			return;
		} else {
			if(table_info->alias == nullptr)
			{
				required_tables.push_back(tm);
			} else {
				auto alias = tm->mirror(table_info->alias);
				alias_tables.push_back(alias);
				required_tables.push_back(alias.get());
			}
		}
	}

	// get select expression name
	std::vector<expr_node_t*> exprs;
	std::vector<std::string> expr_names;
	bool is_aggregate = false;
	for(linked_list_t *link_p = info->exprs; link_p; link_p = link_p->next)
	{
		expr_node_t *expr = (expr_node_t*)link_p->data;
		is_aggregate |= expression::is_aggregate(expr);
		exprs.push_back(expr);
		expr_names.push_back(expression::to_string(expr));
	}

	std::vector<std::string> headers;
	std::vector< std::vector<std::string> > rows;
	// output header info
	for(size_t i = 0; i < exprs.size(); ++i)
	{
		if(i != 0) {
			std::fprintf(output_file, ",");
			printf(",");
		}
		std::fprintf(output_file, "%s", expr_names[i].c_str());
		printf("%s", expr_names[i].c_str());
		// headers.push_back(expr_names[i]);
		headers.insert(headers.begin(), expr_names[i]);
	}

	if(exprs.size() == 0)
	{
		for(size_t i = 0; i < required_tables.size(); ++i)
		{
			if(i != 0) {
				std::fprintf(output_file, ",");
				printf(",");
			}
			required_tables[i]->dump_header(output_file, headers);
		}
	}

	std::fprintf(output_file, "\n");
	printf("\n");

	// pack seq
	uint8_t seq = 1;
	// 1.field count pack
	std::vector<uint8_t> out_pack;
	out_pack.push_back(1);
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.push_back(static_cast<uint8_t>(headers.size()));
	UnboundedBuffer reply_;
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();
	out_pack.clear();

	// 2.table header
	for(auto h : headers) {
		seq ++;
		linked_list_t *table_l = info->tables;
		table_join_info_t *table_info = (table_join_info_t*)table_l->data;
		std::cout << h << ",h ";

		Protocol::FieldPacket new_field_pack(h, static_cast< uint32_t >(6165), std::string(table_info->table), 
        std::string(table_info->table), std::string(cur_db->get_name()), h, 80,
        33, 0, 0);
		std::vector< uint8_t > field_pack = new_field_pack.Pack();
		out_pack.push_back(field_pack.size());
		out_pack.push_back(0);
		out_pack.push_back(0);
		out_pack.push_back(seq);
		out_pack.insert(
			out_pack.end(),
			field_pack.begin(),
			field_pack.end()
		);
		reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
						out_pack.size());
		cli->SendPacket(reply_);
		reply_.Clear();
		out_pack.clear();
	}

	// 3.eof 
	Protocol::EofPacket eof(0, 2);
    std::vector< uint8_t > eof_packet = eof.Pack();
	seq ++; // 254, 0, 0, 2, 0
	out_pack.push_back(eof_packet.size());
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.insert(
		out_pack.end(),
		eof_packet.begin(),
		eof_packet.end()
	);
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();
	out_pack.clear();

	if(is_aggregate)
	{
		select_rows_aggregate(
			info,
			required_tables,
			exprs,
			expr_names,
			cli,
			seq
		);

		return;
	}

	// iterate records
	int counter = 0;

	iterate(required_tables, info->where,
		[&](const std::vector<table_manager*> &tables,
			const std::vector<record_manager*> &records,
			const std::vector<int>& )
		{
			std::vector<std::string> row;
			for(size_t i = 0; i < exprs.size(); ++i)
			{
				expression ret;
				try {
					ret = expression::eval(exprs[i]);
				} catch (const char *e) {
					std::fprintf(stderr, "%s\n", e);
					printf("%s\n", e);
					return false;
				}
				// std::printf("%s\t = ", expr_names[i].c_str());
				if(i != 0) {
					std::fprintf(output_file, ",");
					printf(",");
				}
        switch(ret.type)
				{
					case TERM_INT:
						std::fprintf(output_file, "%d", ret.val_i);
						printf("%d", ret.val_i);
						row.insert(row.begin(), std::to_string(ret.val_i));
            break;
					case TERM_FLOAT:
						std::fprintf(output_file, "%f", ret.val_f);
						printf("%f", ret.val_f);
						row.insert(row.begin(), std::to_string(ret.val_f));
            break;
					case TERM_STRING:
						std::fprintf(output_file, "%s", ret.val_s);
					    row.insert(row.begin(), ret.val_s);
					    printf("%s", ret.val_s);
						break;
					case TERM_BOOL:
						std::fprintf(output_file, "%s", ret.val_b ? "TRUE" : "FALSE");
						printf("%s", (ret.val_b ? "TRUE" : "FALSE"));
						row.insert(row.begin(), ret.val_b ? "TRUE" : "FALSE");
            break;
					case TERM_DATE: {
						char date_buf[32];
						time_t time = ret.val_i;
						auto tm = std::localtime(&time);
						std::strftime(date_buf, 32, DATE_TEMPLATE, tm);
						printf("%s", date_buf);
						std::fprintf(output_file, "%s", date_buf);
						row.insert(row.begin(), date_buf);
						break; 
					}
					case TERM_NULL:
						std::fprintf(output_file, "NULL");
						row.insert(row.begin(), "NULL");
						printf("NULL");
            break;
					default:
						debug_puts("[Error] Data type not supported!");
				}
			}
		
			if(exprs.size() == 0)
			{
				for(size_t i = 0; i < tables.size(); ++i)
				{
					if(i != 0) {
						std::fprintf(output_file, ",");
						printf(",");
					}
					tables[i]->dump_record(output_file, records[i], row);
				}
				rows.push_back(row);
			} else {
				rows.push_back(row);
			}
			std::fprintf(output_file, "\n");
			printf("\n");
			++counter;
			
			return true;
		}
	);

	std::printf("[Info] %d row(s) selected.\n", counter);

	for(auto r : rows) {
		seq ++;
		std::cout << std::endl;
		for(auto item : r) {
			std::cout  << item << "  | ";
		}
		std::cout << std::endl;
		Protocol::RowPacket row_pack(r);
		std::vector< uint8_t > row_packed = row_pack.Pack();
		std::vector< uint8_t > out_pack;
		out_pack.push_back(row_packed.size());
		out_pack.push_back(0);
		out_pack.push_back(0);
		out_pack.push_back(seq);
		out_pack.insert(
			out_pack.end(),
			row_packed.begin(),
			row_packed.end()
		);
		reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
						out_pack.size());
		cli->SendPacket(reply_);
		reply_.Clear();
		out_pack.clear();
	}
	// 5.eof
	seq ++;
	out_pack.push_back(eof_packet.size());
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.insert(
		out_pack.end(),
		eof_packet.begin(),
		eof_packet.end()
	);
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();
    std::fprintf(output_file, "\n");
	std::fflush(output_file);
}

void dbms::select_rows_aggregate(
	const select_info_t *info,
	const std::vector<table_manager*> &required_tables,
	const std::vector<expr_node_t*> &exprs,
	const std::vector<std::string> &, 
	Client* cli,
	uint8_t seq)
{
	if(exprs.size() != 1)
	{
		std::fprintf(stderr, "[Error] Support only for one select expression for aggregate select.");
		printf("[Error] Support only for one select expression for aggregate select.");
		return;
	}

	// check aggregate type
	expr_node_t *expr = exprs[0];
	int val_i = 0;
	float val_f = 0;
	if(expr->op == OPERATOR_MIN)
	{
		val_i = std::numeric_limits<int>::max();
		val_f = std::numeric_limits<float>::max();
	} else if(expr->op == OPERATOR_MAX) {
		val_i = std::numeric_limits<int>::min();
		val_f = std::numeric_limits<float>::min();
	}

	term_type_t agg_type = TERM_NONE;

	int counter = 0;
	iterate(required_tables, info->where,
		[&](const std::vector<table_manager*> &,
			const std::vector<record_manager*> &,
			const std::vector<int>& )
		{
			if(expr->op != OPERATOR_COUNT)
			{
				expression ret;
				try {
					ret = expression::eval(expr->left);
				} catch (const char *e) {
					std::fprintf(stderr, "%s\n", e);
					return false;
				}

				agg_type = ret.type;
				if(ret.type == TERM_FLOAT)
				{
					switch(expr->op)
					{
						case OPERATOR_SUM:
						case OPERATOR_AVG:
							val_f += ret.val_f;
							break;
						case OPERATOR_MIN:
							if(ret.val_f < val_f)
								val_f = ret.val_f;
							break;
						case OPERATOR_MAX:
							if(ret.val_f > val_f)
								val_f = ret.val_f;
							break;
						default: break;
					}
				} else {
					switch(expr->op)
					{
						case OPERATOR_SUM:
						case OPERATOR_AVG:
							val_i += ret.val_i;
							break;
						case OPERATOR_MIN:
							if(ret.val_i < val_i)
								val_i = ret.val_i;
							break;
						case OPERATOR_MAX:
							if(ret.val_i > val_i)
								val_i = ret.val_i;
							break;
						default: break;
					}
				}
			}

			++counter;
			return true;
		}
	);

	std::string result;

	if(expr->op == OPERATOR_COUNT)
	{
		std::fprintf(output_file, "%d\n", counter);
		result = std::to_string(counter);
	} else {
		if(agg_type != TERM_FLOAT && agg_type != TERM_INT)
		{
			std::fprintf(stderr, "[Error] Aggregate only support for int and float type.\n");
			return;
		}

		if(expr->op == OPERATOR_AVG)
		{
			if(agg_type == TERM_INT)
				val_f = double(val_i) / counter;
			else val_f /= counter;
			std::fprintf(output_file, "%f\n", val_f);
			printf("%f\n", val_f);
			result = std::to_string(val_f);
		} else if(agg_type == TERM_FLOAT) {
			std::fprintf(output_file, "%f\n", val_f);
			printf("%f\n", val_f);
			result = std::to_string(val_f);
		} else if(agg_type == TERM_INT) {
			std::fprintf(output_file, "%d\n", val_i);
			printf("%d\n", val_i);
			result = std::to_string(val_i);
		}
	}

	seq++;
	std::vector<std::string> row;
	row.push_back(result);
	Protocol::RowPacket row_pack(row);
	std::vector< uint8_t > row_packed = row_pack.Pack();
	std::vector< uint8_t > out_pack;
	out_pack.push_back(row_packed.size());
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.insert(
		out_pack.end(),
		row_packed.begin(),
		row_packed.end()
	);
	UnboundedBuffer reply_;
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();
	out_pack.clear();

	// eof
	seq ++;
	Protocol::EofPacket eof(0, 2);
    std::vector< uint8_t > eof_packet = eof.Pack();
	out_pack.push_back(eof_packet.size());
	out_pack.push_back(0);
	out_pack.push_back(0);
	out_pack.push_back(seq);
	out_pack.insert(
		out_pack.end(),
		eof_packet.begin(),
		eof_packet.end()
	);
	reply_.PushData(std::string(out_pack.begin(), out_pack.end()).c_str(),
					out_pack.size());
	cli->SendPacket(reply_);
	reply_.Clear();

	std::printf("[Info] %d row(s) selected.\n", counter);
	std::fprintf(output_file, "\n");
	printf("\n");
	std::fflush(output_file);
}

void dbms::delete_rows(const delete_info_t *info, Client* cli, const char *pkt)
{
	if(!assert_db_open())
		return;
	__cache_clear_guard __guard;

	std::vector<int> delete_list;
	table_manager *tm = cur_db->get_table(info->table);
	if(tm == nullptr)
	{
		std::fprintf(stderr, "[Error] table `%s` doesn't exists.\n", info->table);
		return;
	}

	iterate_one_table_with_index(tm, info->where,
		[&delete_list](table_manager*, record_manager*, int rid) -> bool {
			delete_list.push_back(rid);
			return true;
		} );

	int counter = 0;
	for(int rid : delete_list)
		counter += tm->remove_record(rid);

	Protocol::OkPacket ok_pack;
    std::vector<uint8_t> ok_packed = ok_pack.Pack(counter, 0, 2, 0);
	std::vector< uint8_t > res;
	res.push_back(ok_packed.size());
	res.push_back(0);
	res.push_back(0);
	res.push_back(1);
	res.insert(
		res.end(),
		ok_packed.begin(),
		ok_packed.end()
	);
	UnboundedBuffer reply_;
	reply_.PushData(std::string(res.begin(), res.end()).c_str(),
					res.size());
	cli->SendPacket(reply_);
	std::printf("[Info] %d row(s) deleted.\n", counter);
}

void dbms::insert_rows(const insert_info_t *info, Client* cli, const char *pkt)
{
	if(!assert_db_open())
		return;
	__cache_clear_guard __guard;

	table_manager *tb = cur_db->get_table(info->table);
	if(tb == nullptr)
	{
		std::fprintf(stderr, "[Error] table `%s` not found.\n", info->table);
		return;
	}

	std::vector<int> cols_id;
	if(info->columns == nullptr)
	{
		// exclude __rowid__, which has the largest index
		for(int i = 0; i < tb->get_column_num() - 1; ++i)
			cols_id.push_back(i);
	} else {
		for(linked_list_t *link_ptr = info->columns; link_ptr; link_ptr = link_ptr->next)
		{
			column_ref_t *column = (column_ref_t*)link_ptr->data;
			int cid = tb->lookup_column(column->column);
			if(cid < 0)
			{
				std::fprintf(stderr, "[Error] No column `%s` in table `%s`.\n",
					column->column, tb->get_table_name());
				return;
			}
			cols_id.push_back(cid);
		}
	}

	int count_succ = 0, count_fail = 0;
	for(linked_list_t *list = info->values; list; list = list->next)
	{
		tb->init_temp_record();
		linked_list_t *expr_list = (linked_list_t*)list->data;
		unsigned val_num = 0;
		for(linked_list_t *i = expr_list; i; i = i->next, ++val_num);
		if(val_num != cols_id.size())
		{
			std::fprintf(stderr, "[Error] column size not equal.");
			continue;
		}

		bool succ = true;
		for(auto it = cols_id.begin(); expr_list; expr_list = expr_list->next, ++it)
		{
			expression v;
			try {
				v = expression::eval((expr_node_t*)expr_list->data);
			} catch (const char *e) {
				std::fprintf(stderr, "%s\n", e);
				return;
			}

			auto col_type = tb->get_column_type(*it);
			if(!typecast::type_compatible(col_type, v))
			{
				std::fprintf(stderr, "[Error] incompatible type.\n");
				return;
			}

			term_type_t desired_type = typecast::column_to_term(col_type);
			char *db_val = typecast::expr_to_db(v, desired_type);
			if(!tb->set_temp_record(*it, db_val))
			{
				succ = false;
				break;
			}
		}

		if(succ) succ = (tb->insert_record() > 0);
		count_succ += succ;
		count_fail += 1 - succ;
	}

	Protocol::OkPacket ok_pack;
    std::vector<uint8_t> ok_packed = ok_pack.Pack(count_succ, 0, 2, 0);
	std::vector< uint8_t > res;
	res.push_back(ok_packed.size());
	res.push_back(0);
	res.push_back(0);
	res.push_back(1);
	res.insert(
		res.end(),
		ok_packed.begin(),
		ok_packed.end()
	);
	UnboundedBuffer reply_;
	reply_.PushData(std::string(res.begin(), res.end()).c_str(),
					res.size());
	cli->SendPacket(reply_);
	std::printf("[Info] %d row(s) inserted, %d row(s) failed.\n", count_succ, count_fail);
}

void dbms::drop_index(const char *tb_name, const char *col_name)
{
}

void dbms::create_index(const char *tb_name, const char *col_name)
{
	if(!assert_db_open())
		return;
	table_manager *tb = cur_db->get_table(tb_name);
	if(tb == nullptr)
	{
		std::fprintf(stderr, "[Error] table `%s` not exists.\n", tb_name);
	} else {
		tb->create_index(col_name);
	}
}

bool dbms::assert_db_open()
{
	if(cur_db && cur_db->is_opened())
		return true;
	std::fprintf(stderr, "[Error] database is not opened.\n");
	return false;
}

expr_node_t *dbms::get_join_cond(expr_node_t *cond)
{
	if(!cond) return nullptr;
	if(cond->left->term_type == TERM_COLUMN_REF && cond->right->term_type == TERM_COLUMN_REF)
	{
		return cond;
	} else {
		return nullptr;
	}
}

bool dbms::find_longest_path(int now, int depth, int *mark, int *path, std::vector<std::vector<int>> &E, int excepted_len, int &max_depth)
{
	mark[now] = 1;
	path[depth] = now;
	if(depth > max_depth)
		max_depth = depth;
	if(depth == excepted_len)
		return true;
	for(int i = 0; i != (int)E.size(); ++i)
	{
		if(!E[now][i] || mark[i]) continue;
		if(find_longest_path(i, depth + 1, mark, path, E, excepted_len, max_depth))
			return true;
	}

	mark[now] = 0;
	return false;
}

bool dbms::value_exists(const char *table, const char *column, const char *data)
{
	if(!assert_db_open())
		return false;
	table_manager *tm = cur_db->get_table(table);
	if(tm == nullptr)
	{
		std::printf("[Error] No table named `%s`\n", table);
		return false;
	}

	return tm->value_exists(column, data);
}
