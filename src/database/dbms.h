#ifndef __TRIVIALDB_DBMS__
#define __TRIVIALDB_DBMS__
#include "database.h"
#include "../table/table.h"
#include "../parser/defs.h"
#include "../expression/expression.h"
#include <cstdio>
#include "../../network/server.h"
#include "../../network/client.h"
#include "../../network/field.h"

#define VERSION "1.0.0"
#define MOCK_PMEMKV_REDIS_IP "172.17.0.2"
#define MOCK_PMEMKV_REDIS_PORT 6379
#define DEMO_SERVER_ADDR "0.0.0.0:12306"

class Uhpsqld : public Server {
 public:
  Uhpsqld();
  ~Uhpsqld();

  // bool ParseArgs(int ac, char* av[]);

 private:
  std::shared_ptr<StreamSocket> _OnNewConnection(int fd, int tag) override;

  bool _Init() override;
  bool _RunLogic() override;
  bool _Recycle() override;

  unsigned short port_;
};


class dbms
{
	FILE *output_file;
	database *cur_db;
private:
	dbms();

public:
	~dbms();

	void close_database();
	void show_database(const char *db_name, Client* cli, const char *pkt);
	void switch_database(const char *db_name, Client* cli, const char *pkt);
	void drop_database(const char *db_name, Client* cli, const char *pkt);
	void create_database(const char *db_name, Client* cli, const char *pkt);

	void create_table(const table_header_t *header, Client* cli, const char *pkt);
	void show_table(const char *table_name);
	void drop_table(const char *table_name);

	void create_index(const char *tb_name, const char *col_name);
	void drop_index(const char *tb_name, const char *col_name);

	void insert_rows(const insert_info_t *info, Client* cli, const char *pkt);
	void delete_rows(const delete_info_t *info, Client* cli, const char *pkt);
	void select_rows(const select_info_t *info, Client* cli, const char *pkt);
	void update_rows(const update_info_t *info, Client* cli, const char *pkt);

	void switch_select_output(const char *filename);

	void select_rows_aggregate(
		const select_info_t *info,
		const std::vector<table_manager*> &required_tables,
		const std::vector<expr_node_t*> &exprs,
		const std::vector<std::string> &expr_names, 
		Client* cli,
		uint8_t seq);

	bool value_exists(const char *table, const char *column, const char *data);

public:
	bool assert_db_open();
	void cache_record(table_manager *tm, record_manager *rm);

	template<typename Callback>
	void iterate(std::vector<table_manager*> required_tables, expr_node_t *cond, Callback callback);

	template<typename Callback>
	void iterate_one_table(table_manager* table,
			expr_node_t *cond, Callback callback);
	template<typename Callback>
	bool iterate_one_table_with_index(table_manager* table,
			expr_node_t *cond, Callback callback);
	template<typename Callback>
	bool iterate_many_tables_impl(
		const std::vector<table_manager*> &table_list,
		std::vector<record_manager*> &record_list,
		std::vector<int> &rid_list,
		std::vector<std::vector<expr_node_t*>> &index_cond,
		int *iter_order, int *index_cid, index_manager** index,
		expr_node_t *cond, Callback callback, int now);
	template<typename Callback>
	void iterate_many_tables(
		const std::vector<table_manager*> &table_list,
		expr_node_t *cond, Callback callback);

	static expr_node_t *get_join_cond(expr_node_t *cond);
	static void extract_and_cond(expr_node_t *cond, std::vector<expr_node_t*> &and_cond);
	static bool find_longest_path(int now, int depth, int *mark, int *path, std::vector<std::vector<int>> &E, int excepted_len, int &max_depth);

public:
	static dbms* get_instance()
	{
		static dbms ms;
		return &ms;
	}
};

#endif
