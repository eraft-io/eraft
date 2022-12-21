// Copyright 2022 The uhp-sql Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "client.h"

#include <stdint.h>
#include <sys/time.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "auth.h"
#include "common.h"
#include "greeting.h"
#include "unbounded_buffer.h"
#include "err.h"
#include "../src/parser/defs.h"
#include "../src/parser/parser.h"
#include "../src/database/dbms.h"
#include "../src/table/table_header.h"

extern "C" char run_parser(const char *input);

template<typename T, typename DataDeleter>
void free_linked_list(linked_list_t *linked_list, DataDeleter data_deleter)
{
	for(linked_list_t *l_ptr = linked_list; l_ptr; )
	{
		T* data = (T*)l_ptr->data;
		data_deleter(data);
		linked_list_t *tmp = l_ptr;
		l_ptr = l_ptr->next;
		free(tmp);
	}
}

void expression::free_exprnode(expr_node_t *expr)
{
	if(!expr) return;
	if(expr->op == OPERATOR_NONE)
	{
		switch(expr->term_type)
		{
			case TERM_STRING:
				free(expr->val_s);
				break;
			case TERM_COLUMN_REF:
				free(expr->column_ref->table);
				free(expr->column_ref->column);
				free(expr->column_ref);
				break;
			case TERM_LITERAL_LIST:
				free_linked_list<expr_node_t>(
					expr->literal_list,
					expression::free_exprnode
				);
				break;
			default:
				break;
		}
	} else {
		free_exprnode(expr->left);
		free_exprnode(expr->right);
	}

	free(expr);
}

void free_column_ref(column_ref_t *cref)
{
	if(!cref) return;
	free(cref->column);
	free(cref->table);
	free(cref);
}

PacketLength Client::_HandlePacket(const char *start, std::size_t bytes) {
  const char *const end = start + bytes;
  const char *ptr = start;
  Protocol::AuthPacket ap;
  std::vector<uint8_t> OkPacket = {7, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0};
  OkPacket[3] = start[3] + 1;
  std::string pack = std::string(start, end);
  uint8_t cmdType = static_cast<uint8_t>(start[4]);
  std::string queryStr = std::string(start + 5, end);

  if (pack[3] == INIT_PACKET_CNT) {
    auto authPkt = std::vector<uint8_t>(pack.begin() + 4, pack.end());
    ap.UnPack(authPkt);
    std::cout << "UserName: " << ap.GetUserName() << std::endl;
    std::cout << "PluginName: " << ap.GetPluginName() << std::endl;
    std::cout << "DataBaseName: " << ap.GetDatabaseName() << std::endl;
    reply_.PushData(std::string(OkPacket.begin(), OkPacket.end()).c_str(),
                    OkPacket.size());
    SendPacket(reply_);
    _Reset();
    return static_cast<PacketLength>(bytes);
  } else {
    if(queryStr != "select @@version_comment limit 1") {
      std::cout << "cmd type -> " << std::to_string(cmdType) << std::endl;
      switch (cmdType)
      {
      case MYSQL_COM_INIT_DB:
        {
          queryStr.push_back(';');
          run_parser(queryStr.c_str());
          std::cout << "execute_use_database -> " << queryStr << std::endl;
          std::string dbName((char*)result.param);
          std::cout << dbName << std::endl;
          dbms::get_instance()->switch_database(dbName.c_str(), this, start);
          result.type = SQL_RESET;
          break;
        }
      case MYSQL_COM_QUERY:
        {
          queryStr.push_back(';');
          run_parser(queryStr.c_str());
          std::cout << "result type -> " << std::to_string(result.type) << std::endl;
          switch (result.type)
          {
          case SQL_CREATE_DATABASE:
            {
              std::string dbName((char*)result.param);
              dbms::get_instance()->create_database(dbName.c_str(), this, start);
              result.type = SQL_RESET;
              free((char*) result.param);
              break;
            }
          case SQL_DROP_DATABASE: 
            {
              std::string dbName((char*)result.param);
              dbms::get_instance()->drop_database(dbName.c_str(), this, start);
              result.type = SQL_RESET;
              free((char*) result.param);
              break;
            }
          case SQL_SHOW_DATABASE: 
            {
              std::string dbName((char*)result.param);
              dbms::get_instance()->show_database(dbName.c_str(), this, start); 
              result.type = SQL_RESET;
              free((char*) result.param);
              break;      
            }
          case SQL_CREATE_TABLE:
            {
              std::cout << "execute_create_table" << std::endl;
              table_def_t *table = (table_def_t*)result.param;
              table_header_t *header = new table_header_t;
              std::cout << "T name:" << table->name << std::endl;
              if(fill_table_header(header, table)) {
                  dbms::get_instance()->create_table(header, this, start);
              } else {
                  printf("[Error] Fail to create table!\t");
              }
              // free resources
              delete header;
              free(table->name);
              free_linked_list<table_constraint_t>(table->constraints, [](table_constraint_t *data) {
                expression::free_exprnode(data->check_cond);
                free_column_ref(data->column_ref);
                free_column_ref(data->foreign_column_ref);
                free(data);
              });
              for(field_item_t *it = table->fields; it; )
              {
                field_item_t *tmp = it;
                free(it->name);
                expression::free_exprnode(it->default_value);
                it = it->next;
                free(tmp);
              }
              free((void*)table);
              result.type = SQL_RESET;
              break;
            }
          case SQL_INSERT: 
            {
              insert_info_t *info = (insert_info_t*)result.param;
              dbms::get_instance()->insert_rows(info, this, start);
              // free resources
              free(info->table);
              free_linked_list<column_ref_t>(info->columns, free_column_ref);
              free_linked_list<linked_list_t>(info->values, [](linked_list_t *expr_list) {
                  free_linked_list<expr_node_t>(expr_list, expression::free_exprnode);
              } );
              free((void*)info);
              result.type = SQL_RESET;
              break;
            }
          case SQL_SELECT:
            {
              select_info_t *select_info = (select_info_t*)result.param;
              dbms::get_instance()->select_rows(select_info, this, start);
              expression::free_exprnode(select_info->where);
              free_linked_list<expr_node_t>(select_info->exprs, expression::free_exprnode);
              free_linked_list<table_join_info_t>(select_info->tables, [](table_join_info_t *data) {
                  free(data->table);
                  if(data->join_table)
                      free(data->join_table);
                  if(data->alias)
                      free(data->alias);
                  expression::free_exprnode(data->cond);
                  free(data);
              });	
              free((void*)select_info);
              result.type = SQL_RESET;
              break;
            }
          case SQL_UPDATE:
            {
              update_info_t *update_info = (update_info_t*)result.param;
              dbms::get_instance()->update_rows(update_info, this, start);
              free(update_info->table);
              free_column_ref(update_info->column_ref);
              expression::free_exprnode(update_info->where);
              expression::free_exprnode(update_info->value);
              free((void*)update_info);
              result.type = SQL_RESET;
              break;
            }
          case SQL_DELETE:
            {
              delete_info_t *delete_info = (delete_info_t*)result.param;
              dbms::get_instance()->delete_rows(delete_info, this, start);
              free(delete_info->table);
              expression::free_exprnode(delete_info->where);
              free((void*)delete_info); 
              result.type = SQL_RESET;
              break;
            }
          default:
            {
              std::vector<uint8_t> OkPacket = {7, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0};
              OkPacket[3] = pack[3] + 1;
              reply_.PushData(std::string(OkPacket.begin(), OkPacket.end()).c_str(),
                              OkPacket.size());
              SendPacket(reply_);
              reply_.Clear();
              break;
            }
          }
          break;
        }
      default:
        break;
      }
    } else {
        std::vector<uint8_t> OkPacket = {7, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0};
        OkPacket[3] = pack[3] + 1;
        reply_.PushData(std::string(OkPacket.begin(), OkPacket.end()).c_str(),
                        OkPacket.size());
        SendPacket(reply_);
        reply_.Clear();
    }

    return static_cast<PacketLength>(bytes);
  }
}

Client::Client() { _Reset(); }

void Client::_Reset() {
  parser_.Reset();
  reply_.Clear();
}

void Client::OnConnect() {
  std::cout << "new client comming!" << std::endl;
  std::vector<uint8_t> greetingPacket;

  Protocol::GreetingPacket gp(1, "Copyright © 2022 W-SQL Group.");
  std::vector<uint8_t> outputPacket = gp.Pack();
  greetingPacket.push_back(outputPacket.size());
  greetingPacket.push_back(0x00);
  greetingPacket.push_back(0x00);
  greetingPacket.push_back(0x00);

  greetingPacket.insert(greetingPacket.end(), outputPacket.begin(),
                        outputPacket.end());
  reply_.PushData(
      std::string(greetingPacket.begin(), greetingPacket.end()).c_str(),
      greetingPacket.size());
  SendPacket(reply_);
  _Reset();
}
