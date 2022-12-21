/* Modified from https://raw.githubusercontent.com/thinkpad20/sql/master/src/yacc/sql.y
             and https://github.com/Harry-Chen/SimpleDB
 * Grammar: http://h2database.com/html/grammar.html#select */

%define parse.error verbose

%{
#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "parser.h"

void yyerror(const char *s);

#include "sql.yy.c"

%}

%union {
	char *val_s;
	int   val_i;
	float val_f;
	struct field_item_t       *field_items;
	struct table_def_t        *table_def;
	struct column_ref_t       *column_ref;
	struct linked_list_t      *list;
	struct table_constraint_t *constraint;
	struct insert_info_t      *insert_info;
	struct update_info_t      *update_info;
	struct delete_info_t      *delete_info;
	struct select_info_t      *select_info;
	struct table_join_info_t  *join_info;
	struct expr_node_t        *expr;
}

%token TRUE FALSE NULL_TOKEN MIN MAX SUM AVG COUNT
%token LIKE IS OR AND NOT NEQ GEQ LEQ
%token INTEGER DOUBLE FLOAT CHAR VARCHAR DATE
%token INTO FROM WHERE VALUES JOIN INNER OUTER
%token LEFT RIGHT FULL ASC DESC ORDER BY IN ON AS
%token DISTINCT GROUP USING INDEX TABLE DATABASE
%token DEFAULT UNIQUE PRIMARY FOREIGN REFERENCES CHECK KEY OUTPUT
%token USE CREATE DROP SELECT INSERT UPDATE DELETE SHOW SET EXIT

%token IDENTIFIER
%token DATE_LITERAL
%token STRING_LITERAL
%token FLOAT_LITERAL
%token INT_LITERAL

%type <val_s> IDENTIFIER STRING_LITERAL DATE_LITERAL
%type <val_f> FLOAT_LITERAL
%type <val_i> INT_LITERAL

%type <val_i> field_type field_width field_flag field_flags
%type <val_s> table_name database_name
%type <val_s> create_database_stmt use_database_stmt drop_database_stmt show_database_stmt 
%type <val_s> drop_table_stmt show_table_stmt

%type <field_items> table_field table_fields
%type <table_def> create_table_stmt
%type <column_ref> column_ref
%type <constraint> table_extra_option
%type <list> column_list expr_list insert_values literal_list
%type <list> table_extra_options table_extra_option_list
%type <insert_info> insert_stmt insert_columns
%type <update_info> update_stmt
%type <delete_info> delete_stmt
%type <select_info> select_stmt
%type <expr> expr factor term condition cond_term where_clause literal literal_list_expr
%type <expr> aggregate_expr aggregate_term select_expr default_expr
%type <val_i> logical_op compare_op aggregate_op
%type <list> select_expr_list select_expr_list_s table_refs
%type <join_info> table_item

%start sql_stmts

%%

sql_stmts  :  sql_stmt
		   |  sql_stmts sql_stmt
		   ;

sql_stmt   :  create_table_stmt ';'    { parser_create_table($1); }
		   |  create_database_stmt ';' { parser_create_database($1); }
		   |  use_database_stmt ';'    { parser_use_database($1); }
		   |  show_database_stmt ';'   { parser_show_database($1); }
		   |  drop_database_stmt ';'   { parser_drop_database($1); }
		   |  show_table_stmt ';'      { parser_show_table($1); }
		   |  drop_table_stmt ';'      { parser_drop_table($1); }
		   |  insert_stmt ';'          { parser_insert($1); }
		   |  update_stmt ';'          { parser_update($1); }
		   |  delete_stmt ';'          { parser_delete($1); }
		   |  select_stmt ';'          { parser_select($1); }
		   |  EXIT ';'                 { parser_quit(); exit(0); }
		   |  SET OUTPUT '=' STRING_LITERAL ';'  { parser_switch_output($4); }
		   |  CREATE INDEX table_name '(' IDENTIFIER ')' ';' { parser_create_index($3, $5); }
		   |  DROP   INDEX table_name '(' IDENTIFIER ')' ';' { parser_drop_index($3, $5); }
		   ;

create_table_stmt : CREATE TABLE table_name '(' table_fields table_extra_options ')' {
				  	$$ = (table_def_t*)malloc(sizeof(table_def_t));
					$$->name = $3;
					$$->fields = $5;
					$$->constraints = $6;
				  }
				  ;

create_database_stmt : CREATE DATABASE database_name   { $$ = $3; };
use_database_stmt    : USE database_name               { $$ = $2; };
drop_database_stmt   : DROP DATABASE database_name     { $$ = $3; };
show_database_stmt   : SHOW DATABASE database_name     { $$ = $3; };
drop_table_stmt      : DROP TABLE table_name           { $$ = $3; };
show_table_stmt      : SHOW TABLE table_name           { $$ = $3; };
insert_stmt          : INSERT INTO insert_columns VALUES insert_values {
					 	$$ = $3;
						$$->values = $5;
					 }
					 ;

insert_values        : '(' expr_list ')' {
					 	$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
						$$->data = $2;
						$$->next = NULL;
					 }
					 | insert_values ',' '(' expr_list ')' {
					 	$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
						$$->data = $4;
						$$->next = $1;
					 }
					 ;

insert_columns       : table_name {
					 	$$ = (insert_info_t*)malloc(sizeof(insert_info_t));
						$$->table   = $1;
						$$->columns = NULL;
						$$->values  = NULL;
					 }
					 | table_name '(' column_list ')' {
					 	$$ = (insert_info_t*)malloc(sizeof(insert_info_t));
						$$->table   = $1;
						$$->columns = $3;
						$$->values  = NULL;
					 }
					 ;

delete_stmt         : DELETE FROM table_name where_clause {
					 	$$ = (delete_info_t*)malloc(sizeof(delete_info_t));
						$$->table = $3;
						$$->where = $4;
					}
					;

update_stmt         : UPDATE table_name SET column_ref '=' expr where_clause {
					 	$$ = (update_info_t*)malloc(sizeof(update_info_t));
						$$->table = $2;
						$$->value = $6;
						$$->where = $7;
						$$->column_ref = $4;
					}
					;

select_stmt         : SELECT select_expr_list_s FROM table_refs where_clause {
					 	$$ = (select_info_t*)malloc(sizeof(select_info_t));
						$$->tables = $4;
						$$->exprs  = $2;
						$$->where  = $5;
					}
					;

table_refs          : table_refs ',' table_item {
						$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
						$$->data = $3;
						$$->next = $1;
					}
					| table_item {
						$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
						$$->data = $1;
						$$->next = NULL;
					}
					;

table_item          : table_name {
					 	$$ = (table_join_info_t*)calloc(1, sizeof(table_join_info_t));
						$$->join_type = TABLE_JOIN_NONE;
						$$->table = $1;
					}
				    | table_name AS IDENTIFIER {
					 	$$ = (table_join_info_t*)calloc(1, sizeof(table_join_info_t));
						$$->join_type = TABLE_JOIN_NONE;
						$$->table = $1;
						$$->alias = $3;
					}
					;

select_expr_list_s  : select_expr_list { $$ = $1; }
					| '*'              { $$ = NULL; }

select_expr_list    : select_expr_list ',' select_expr {
						$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
						$$->data = $3;
						$$->next = $1;
					}
					| select_expr {
						$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
						$$->data = $1;
						$$->next = NULL;
					}
					;

select_expr         : expr            { $$ = $1; }
					| aggregate_expr  { $$ = $1; }

aggregate_expr      : aggregate_op '(' aggregate_term ')' {
						$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
						$$->left  = $3;
						$$->op    = $1;
					}
					| COUNT '(' aggregate_term ')' {
						$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
						$$->left  = $3;
						$$->op    = OPERATOR_COUNT;
					}
					| COUNT '(' '*' ')' {
						$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
						$$->left  = NULL;
						$$->op    = OPERATOR_COUNT;
					}
					;

aggregate_term      : column_ref {
						$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
						$$->column_ref = $1;
						$$->term_type  = TERM_COLUMN_REF;
					}
					;

aggregate_op        : SUM   { $$ = OPERATOR_SUM; }
					| AVG   { $$ = OPERATOR_AVG; }
					| MIN   { $$ = OPERATOR_MIN; }
					| MAX   { $$ = OPERATOR_MAX; }
					;

where_clause        : WHERE condition { $$ = $2; }
					| /* empty */     { $$ = NULL; }
					;

table_extra_options : ',' table_extra_option_list  { $$ = $2; }
					| /* empty */                  { $$ = NULL; }
					;

table_extra_option_list : table_extra_option_list ',' table_extra_option {
							$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
							$$->data = $3;
							$$->next = $1;
						}
						| table_extra_option {
							$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
							$$->data = $1;
							$$->next = NULL;
						}
						;

table_extra_option : PRIMARY KEY '(' IDENTIFIER ')' {
				   	$$ = (table_constraint_t*)calloc(1, sizeof(table_constraint_t));
					$$->column_ref = (column_ref_t*)malloc(sizeof(column_ref_t));
					$$->column_ref->table = NULL;
					$$->column_ref->column = $4;
					$$->type = TABLE_CONSTRAINT_PRIMARY_KEY;
				   }
				   | FOREIGN KEY '(' IDENTIFIER ')' REFERENCES IDENTIFIER '(' IDENTIFIER ')' {
				   	$$ = (table_constraint_t*)calloc(1, sizeof(table_constraint_t));
					$$->column_ref = (column_ref_t*)malloc(sizeof(column_ref_t));
					$$->column_ref->table = NULL;
					$$->column_ref->column = $4;
					$$->foreign_column_ref = (column_ref_t*)malloc(sizeof(column_ref_t));
					$$->foreign_column_ref->table = $7;
					$$->foreign_column_ref->column = $9;
					$$->type = TABLE_CONSTRAINT_FOREIGN_KEY;
				   }
				   | UNIQUE '(' column_ref ')' {
				   	$$ = (table_constraint_t*)calloc(1, sizeof(table_constraint_t));
					$$->type = TABLE_CONSTRAINT_UNIQUE;
					$$->column_ref = $3;
				   }
				   | CHECK '(' condition ')' {
				   	$$ = (table_constraint_t*)calloc(1, sizeof(table_constraint_t));
					$$->type = TABLE_CONSTRAINT_CHECK;
					$$->check_cond = $3;
				   }
				   ;

column_ref   : IDENTIFIER {
			 	$$ = (column_ref_t*)malloc(sizeof(column_ref_t));
				$$->table  = NULL;
				$$->column = $1;
			 }
			 | table_name '.' IDENTIFIER {
			 	$$ = (column_ref_t*)malloc(sizeof(column_ref_t));
				$$->table  = $1;
				$$->column = $3;
			 }
			 ;

column_list  : column_list ',' column_ref {
				$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
				$$->data = $3;
				$$->next = $1;
			 }
			 | column_ref {
			 	$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
				$$->data = $1;
				$$->next = NULL;
			 }
			 ;


table_fields : table_field                  { $$ = $1; }
			 | table_fields ',' table_field { $$ = $3; $$->next = $1; }
			 ;

table_field  : IDENTIFIER field_type field_width field_flags default_expr {
			 	$$ = (field_item_t*)malloc(sizeof(field_item_t));
				$$->name = $1;
				$$->type = $2;
				$$->width = $3;
				$$->flags = $4;
				$$->default_value = $5;
				$$->next = NULL;
			 }
			 ;

default_expr : DEFAULT literal { $$ = $2; }
			 | /* empty */     { $$ = NULL; }

field_flags : field_flags field_flag  { $$ = $1 | $2; }
			| /* empty */             { $$ = 0; }
			;

field_flag  : NOT NULL_TOKEN  { $$ = FIELD_FLAG_NOTNULL; }
			| UNIQUE          { $$ = FIELD_FLAG_UNIQUE; }
			| PRIMARY KEY     { $$ = FIELD_FLAG_PRIMARY; }
			;

field_width : '(' INT_LITERAL ')'  { $$ = $2; }
			| /* empty */          { $$ = 0; }
			;

field_type  : INTEGER { $$ = FIELD_TYPE_INT; }
		    | FLOAT   { $$ = FIELD_TYPE_FLOAT; }
		    | DOUBLE  { $$ = FIELD_TYPE_FLOAT; }
		    | CHAR    { $$ = FIELD_TYPE_CHAR; }
		    | DATE    { $$ = FIELD_TYPE_DATE; }
		    | VARCHAR { $$ = FIELD_TYPE_VARCHAR; }
		    ;

logical_op : AND  { $$ = OPERATOR_AND; }
		   | OR   { $$ = OPERATOR_OR; }

compare_op : '='  { $$ = OPERATOR_EQ; }
		   | '<'  { $$ = OPERATOR_LT; }
		   | '>'  { $$ = OPERATOR_GT; }
		   | LEQ  { $$ = OPERATOR_LEQ; }
		   | GEQ  { $$ = OPERATOR_GEQ; }
		   | NEQ  { $$ = OPERATOR_NEQ; }
		   | LIKE { $$ = OPERATOR_LIKE; }
		   ;

condition  : condition logical_op cond_term {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->left  = $1;
				$$->right = $3;
				$$->op    = $2;
		   }
		   | cond_term { $$ = $1; }
		   ;

cond_term  : expr compare_op expr {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->left  = $1;
				$$->right = $3;
				$$->op    = $2;
		   }
		   | expr IN '(' literal_list_expr ')' {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->left  = $1;
				$$->right = $4;
				$$->op    = OPERATOR_IN;
		   }
		   | expr IS NULL_TOKEN {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->left  = $1;
				$$->op    = OPERATOR_ISNULL;
		   }
		   | expr IS NOT NULL_TOKEN {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->left  = $1;
				$$->op    = OPERATOR_NOTNULL;
		   }
		   | NOT cond_term {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->left  = $2;
				$$->op    = OPERATOR_NOT;
		   }
		   | '(' condition ')' { $$ = $2; }
		   | TRUE {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->val_b     = 1;
				$$->term_type = TERM_BOOL;
		   }
		   | FALSE {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->val_b     = 0;
				$$->term_type = TERM_BOOL;
		   }
		   ;

expr_list  : expr_list ',' expr {
				$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
				$$->data = $3;
				$$->next = $1;
		   }
		   | expr {
				$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
				$$->data = $1;
				$$->next = NULL;
		   }
		   ;

expr       : expr '+' factor {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->left  = $1;
				$$->right = $3;
				$$->op    = OPERATOR_ADD;
		   }
		   | expr '-' factor {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->left  = $1;
				$$->right = $3;
				$$->op    = OPERATOR_MINUS;
		   }
		   | factor { $$ = $1; }
		   ;

factor     : factor '*' term {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->left  = $1;
				$$->right = $3;
				$$->op    = OPERATOR_MUL;
		   }
		   | factor '/' term {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->left  = $1;
				$$->right = $3;
				$$->op    = OPERATOR_DIV;
		   }
		   | term { $$ = $1; }
		   ;

term       : column_ref {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->column_ref = $1;
				$$->term_type  = TERM_COLUMN_REF;
		   }
		   | '-' term {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->left  = $2;
				$$->op    = OPERATOR_NEGATE;
		   }
		   | literal      { $$ = $1; }
		   | NULL_TOKEN {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->term_type  = TERM_NULL;
		   }
		   | '(' expr ')' { $$ = $2; }
		   ;

literal    : INT_LITERAL {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->val_i      = $1;
				$$->term_type  = TERM_INT;
		   }
		   | FLOAT_LITERAL {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->val_f      = $1;
				$$->term_type  = TERM_FLOAT;
		   }
		   | DATE_LITERAL {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->val_s      = $1;
				$$->term_type  = TERM_DATE;
		   }
		   | STRING_LITERAL {
		   		$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
				$$->val_s      = $1;
				$$->term_type  = TERM_STRING;
		   }
		   ;

literal_list : literal_list ',' literal {
				$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
				$$->data = $3;
				$$->next = $1;
			 }
			 | literal {
				$$ = (linked_list_t*)malloc(sizeof(linked_list_t));
				$$->data = $1;
				$$->next = NULL;
			 }
			 ;

literal_list_expr : literal_list {
					$$ = (expr_node_t*)calloc(1, sizeof(expr_node_t));
					$$->literal_list = $1;
					$$->term_type    = TERM_LITERAL_LIST;
				  }

table_name : IDENTIFIER          { $$ = $1; }
		   | '`' IDENTIFIER '`'  { $$ = $2; }
		   ;

database_name : IDENTIFIER       { $$ = $1; }
			  ;

%%

void yyerror(const char *msg)
{
	fprintf(stderr, "[Error] %s\n", msg);
}

int yywrap()
{
	return 1;
}

char run_parser(const char *input)
{
	char ret;
  	if(input) {
    	YY_BUFFER_STATE buf = yy_scan_string(input);
		yy_switch_to_buffer(buf);
		ret = yyparse();
		yy_delete_buffer(buf);
	} else {
		ret = yyparse();
	}

	return ret;
}
