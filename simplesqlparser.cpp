#include "simplesqlparser.h"

//rollback helpers
#define RB_SET_POINT(rb_name) QString rb_name(str)
#define RB_TO_POINT(rb_name) str = rb_name

#define RB_BLOCK_START(rb_name) { RB_SET_POINT(rb_name); do
#define RB_BLOCK_END() while(false); }

#define RB_LOOP_START(rb_name) for(RB_SET_POINT(rb_name);;rb_name = str)

#define RB_TEST_COND(rb_name,cond) {if(!(cond)) { RB_TO_POINT(rb_name); break;}}

class SimpleSqlParser::QueryContext{
public:
    bool inAggregatedFunc;
    bool inSelectArgs;
    bool inFrom;
    bool inWhere;
    bool inGroup;
    bool inHaving;

    bool isAggregatedQuery;

    bool error_unaggregated_args;
    bool error_select_nested_in_select;
    QSet<QString> unAggregatedColumns;
    QueryContext* parent;

    QueryContext() {
        isAggregatedQuery = false;
        inAggregatedFunc = false;
        inSelectArgs = false;
        inFrom =false;
        inWhere = false;
        inGroup = false;
        inHaving = false;
        parent = NULL;

        error_unaggregated_args = false;
        error_select_nested_in_select = false;
    }

    QueryContext* newSubContext() {
        QueryContext* ret = new QueryContext();
        ret->parent = this;
        return ret;
    }
    QueryContext* exitSubContext() {
        if(this->isAggregatedQuery && !this->unAggregatedColumns.empty())
            this->error_unaggregated_args = true;

        this->parent->error_select_nested_in_select =
                (this->parent->error_select_nested_in_select || this->error_select_nested_in_select);
        this->parent->error_unaggregated_args =
                (this->parent->error_unaggregated_args || this->error_unaggregated_args);
        QueryContext* ret = this->parent;
        delete this;
        return ret;
    }
};


SimpleSqlParser::SimpleSqlParser()
{
    qContext = new QueryContext();
    lastToken="";
}

//static bool td_string_literal_quote_char(QString &str, QChar qc);
//static bool td_expr(QString &str);
//static bool td_select_stmt(QString &str);
//static bool td_join_source(QString &str);

static const char* keywords[] = {
"ABORT","ACTION", "ADD", "AFTER", "ALL",    "ALTER",    "ANALYZE",    "AND",    "AS",    "ASC",
    "ATTACH", "AUTOINCREMENT",    "BEFORE",    "BEGIN",    "BETWEEN",    "BY",    "CASCADE",
    "CASE", "CAST", "CHECK", "COLLATE",    "COLUMN",    "COMMIT",    "CONFLICT",    "CONSTRAINT",
    "CREATE", "CROSS", "CURRENT_DATE",    "CURRENT_TIME",    "CURRENT_TIMESTAMP",    "DATABASE",
    "DEFAULT", "DEFERRABLE",    "DEFERRED",    "DELETE",    "DESC",    "DETACH",    "DISTINCT",
    "DROP", "EACH",    "ELSE",    "END",    "ESCAPE",    "EXCEPT",    "EXCLUSIVE",    "EXISTS",
    "EXPLAIN", "FAIL",    "FOR",    "FOREIGN",    "FROM",    "FULL",    "GLOB",    "GROUP",
    "HAVING", "IF", "IGNORE",    "IMMEDIATE",    "IN",    "INDEX",    "INDEXED",    "INITIALLY",
    "INNER", "INSERT",    "INSTEAD", "INTERSECT",    "INTO",    "IS",    "ISNULL",    "JOIN",
    "KEY", "LEFT", "LIKE", "LIMIT", "MATCH",    "NATURAL",    "NO",    "NOT",    "NOTNULL",
    "NULL", "OF", "OFFSET", "ON", "OR", "ORDER", "OUTER",    "PLAN",    "PRAGMA",    "PRIMARY",
    "QUERY", "RAISE", "REFERENCES", "REGEXP", "REINDEX",    "RELEASE",    "RENAME",    "REPLACE",
    "RESTRICT", "RIGHT", "ROLLBACK", "ROW", "SAVEPOINT", "SELECT", "SET", "TABLE", "TEMP",
    "TEMPORARY", "THEN", "TO", "TRANSACTION", "TRIGGER",    "UNION",    "UNIQUE",    "UPDATE",
    "USING", "VACUUM", "VALUES",    "VIEW",    "VIRTUAL",    "WHEN",    "WHERE" , NULL};


//accept str from src
bool SimpleSqlParser::td_accept(QString &str, QString acc_str) {
    str = str.trimmed();
    if(str.startsWith(acc_str, Qt::CaseInsensitive)){
        str.remove(0, acc_str.length());
        qDebug() << "accept: " << acc_str;
        lastToken = acc_str;
        return true;
    }
    return false;
}

//accept count characters from src
bool SimpleSqlParser::td_accept(QString &str, int count) {
    if(str.length() < count)
        return false;
    qDebug() << "accept: " << str.left(count);
    lastToken = str.left(count);
    str.remove(0,count);
    return true;
}

//accept any word (not including keywords)
bool SimpleSqlParser::td_accept_any_word(QString &str) {
    str = str.trimmed();
    int i = 0;
    while(i<str.length() && str.at(i).isLetterOrNumber())
        i++;
    if(i==0)
        return false;
    QString word = str.left(i);
    for(int i=0;keywords[i] != NULL;i++)
        if(QString::compare(word,keywords[i], Qt::CaseInsensitive) == 0)
            return false;
    td_accept(str, i);
    return true;
}

//accept specific word
bool SimpleSqlParser::td_accept_word(QString &str, QString word) {
    str = str.trimmed();
    int i = 0;
    while(i<str.length() && str.at(i).isLetterOrNumber())
        i++;
    if(i==0)
        return false;
    if(str.left(i).toUpper() == word.toUpper())
    {
        td_accept(str, i);
        return true;
    }
    return false;
}

bool SimpleSqlParser::td_numeric_literal(QString &str) {
    str = str.trimmed();
    int i = 0;
    if(str.length() == 0)
        return false;
    if(!str.at(0).isDigit() && str.at(0) != '.')
        return false;
    while(i < str.length() && str.at(i).isDigit())
        i++;
    if(i < str.length() && str.at(i) == '.')
        i++;
    while(i < str.length() && str.at(i).isDigit())
        i++;
    if(i+2 < str.length() && str.at(i).toUpper() == 'E'
            && (str.at(i+1) == '+' || str.at(i+1) == '-')
            && str.at(i+2).isDigit()) {
        i+=2;
        while(i < str.length() && str.at(i).isDigit())
                i++;
    }
    td_accept(str, i);
    return true;
}

QString SimpleSqlParser::format_escape_entity(QString s){
    if(s == "\\t")
        return "\t";
    if(s == "\\n")
        return "\n";
    if(s == "\\r")
        return "\r";
    if(s == "\\0")
        return "\0";
    if(s == "\\'")
        return "'";
    if(s == "\\\"")
        return "\"";
    if(s == "\\`")
        return "`";
    if(s == "\\v'")
        return "\v";

    if(s.at(0) == '\\') {
        s.remove(0,1);
        return s;
    }
    return s;
}


bool SimpleSqlParser::td_string_literal_quote_char(QString &str, QChar qc) {
    RB_SET_POINT(q);
    str = str.trimmed();
    if(str.length() == 0)
        return false;
    QString quoted_text="";
    if(str.at(0) == qc)
    {
        str.remove(0,1);
        while(true) {
            if(str.length() == 0)
                break;
            if(str.at(0) == '\\' && str.length() >= 2) {
                quoted_text+=format_escape_entity(str.left(2));
                str.remove(0,2);
            }
            else if(str.startsWith(QString("")+qc+qc))
            { quoted_text+=qc; str.remove(0,2);}
            else if(str.at(0) == qc) {
                str.remove(0,1);
                lastToken = quoted_text;
                return true;
            } else
            { quoted_text+= str.at(0); str.remove(0,1);}
        }
    }
    RB_TO_POINT(q);
    return false;
}


bool SimpleSqlParser::td_string_literal(QString &str) {
    return (td_string_literal_quote_char(str, '"')
            || td_string_literal_quote_char(str, '\''));
}

bool SimpleSqlParser::td_blob_literal(QString &str) {
    return td_string_literal(str);
    //TODO: what's a blob literal?
}

bool SimpleSqlParser::td_literal_value(QString &str) {
    if(td_numeric_literal(str) ||
            td_string_literal(str) ||
            td_blob_literal(str) ||
            td_accept_word(str, "NULL") ||
            td_accept_word(str, "CURRENT_TIME") ||
            td_accept_word(str, "CURRENT_DATE") ||
            td_accept_word(str, "CURRENT_TIMESTAMP"))
        return true;
    return false;
}

bool SimpleSqlParser::td_binary_operator(QString &str) {
    if(td_accept(str,"||"))
        return true;
    if(td_accept(str,"*") || td_accept(str,"/") || td_accept(str,"%"))
        return true;
    if(td_accept(str,"+") || td_accept(str,"-"))
        return true;
    if(td_accept(str,"<<") || td_accept(str,">>") || td_accept(str,"&") || td_accept(str,"|"))
        return true;
    if(td_accept(str,"==") || td_accept(str,"=") || td_accept(str,"!=") || td_accept(str,"<>"))
        return true;
    if(td_accept(str,"<=") || td_accept(str,">="))
        return true;
    if(td_accept(str,"<") || td_accept(str,">"))
        return true;

    if(td_accept_word(str,"IS")) {
        td_accept_word(str,"NOT");
        return true;
    }

    if(td_accept_word(str,"AND") || td_accept_word(str,"OR"))
        return true;

    return false;
}

// names **************


bool SimpleSqlParser::td_table_name(QString &str) {
    return td_string_literal_quote_char(str,'`') || td_accept_any_word(str);
}

bool SimpleSqlParser::td_database_name(QString &str) {
    return td_string_literal_quote_char(str,'`') || td_accept_any_word(str);
}

bool SimpleSqlParser::td_collection_name(QString &str) {
    return td_string_literal_quote_char(str,'`') || td_accept_any_word(str);
}

bool SimpleSqlParser::td_index_name(QString &str) {
    return td_string_literal_quote_char(str,'`') || td_accept_any_word(str);
}

bool SimpleSqlParser::td_function_name(QString &str) {
    return td_accept_any_word(str);
}

bool SimpleSqlParser::td_type_name(QString &str) {
    return td_accept_any_word(str);
}

bool SimpleSqlParser::td_table_alias(QString &str) {
    return td_string_literal_quote_char(str,'`') || td_accept_any_word(str);
}

bool SimpleSqlParser::td_column_alias(QString &str) {
    return td_string_literal_quote_char(str,'`') || td_accept_any_word(str);
}

bool SimpleSqlParser::td_column_name(QString &str) {
    if(td_string_literal_quote_char(str,'`') || td_accept_any_word(str)) {
        if(qContext->inSelectArgs && !qContext->inAggregatedFunc)
            qContext->unAggregatedColumns.insert(lastToken);
        else if(qContext->inGroup && qContext->unAggregatedColumns.contains(lastToken))
            qContext->unAggregatedColumns.remove(lastToken);
        return true;
    }
    return false;
}


// end of names **************

bool SimpleSqlParser::td_extended_column_name(QString &str){
    // [[db-name.]table-name.]column-name
    RB_BLOCK_START(e) {
        RB_BLOCK_START(ee) {
            RB_TEST_COND(ee, td_database_name(str));
            RB_TEST_COND(ee, td_accept(str,"."));
        }RB_BLOCK_END();
        RB_BLOCK_START(ee) {
            RB_TEST_COND(ee, td_table_name(str));
            RB_TEST_COND(ee, td_accept(str,"."));
        }RB_BLOCK_END();
        RB_TEST_COND(e, td_column_name(str));
        return true;
    }RB_BLOCK_END();
    return false;
}

bool SimpleSqlParser::td_simple_expr(QString &str) {
    if(td_literal_value(str))
        return true;

    //nested select: [[NOT] EXISTS] (select_stmt)
    RB_BLOCK_START(e) {
        if(td_accept_word(str,"NOT")) {
            RB_TEST_COND(e, td_accept_word(str,"EXISTS"));
        } else td_accept_word(str,"EXISTS");
        RB_TEST_COND(e, td_accept(str,"("));
        RB_TEST_COND(e, td_select_stmt(str));
        RB_TEST_COND(e, td_accept(str,")"));
        return true;
    }RB_BLOCK_END();

    // ( expr )
    RB_BLOCK_START(e) {
        RB_TEST_COND(e, td_accept(str,"("));
        RB_TEST_COND(e, td_expr(str));
        RB_TEST_COND(e, td_accept(str,")"));
        return true;
    }RB_BLOCK_END();

    // unary operator
    RB_BLOCK_START(e) {
        RB_TEST_COND(e, td_accept(str,"-") || td_accept(str,"+")
                     || td_accept(str,"~") || td_accept_word(str,"NOT"));
        RB_TEST_COND(e, td_expr(str));
        return true;
    }RB_BLOCK_END();

    //functions Min/Max/Avg
    bool prevInAggFuncVal = qContext->inAggregatedFunc;
    RB_BLOCK_START(e) {
        if(td_accept_word(str,"AVG") || td_accept_word(str,"COUNT") || td_accept_word(str,"MAX")
                || td_accept_word(str,"MIN") || td_accept_word(str,"SUM") || td_accept_word(str,"TOTAL")
                || td_accept_word(str,"AVG") || td_accept_word(str,"GROUP_CONCAT") )
            qContext->inAggregatedFunc = true;
        else {
            RB_TEST_COND(e, td_function_name(str));
        }
        RB_TEST_COND(e, td_accept(str,"("));
        if(!td_accept(str,"*")){
            RB_BLOCK_START(ee) {
                td_accept_word(str, "DISTINCT");
                RB_TEST_COND(ee, td_expr(str));
                RB_LOOP_START(eee) {
                    RB_TEST_COND(eee, td_accept(str,","));
                    RB_TEST_COND(eee, td_expr(str));
                }
            }RB_BLOCK_END();
        }
        RB_TEST_COND(e, td_accept(str,")"));
        qContext->inAggregatedFunc = prevInAggFuncVal;
        return true;
    }RB_BLOCK_END();
    qContext->inAggregatedFunc = prevInAggFuncVal;

    // CAST ( expr AS type-name )
    RB_BLOCK_START(e) {
        RB_TEST_COND(e, td_accept_word(str,"CAST"));
        RB_TEST_COND(e, td_accept(str,"("));
        RB_TEST_COND(e, td_expr(str));
        RB_TEST_COND(e, td_accept_word(str,"AS"));
        RB_TEST_COND(e, td_type_name(str));
        RB_TEST_COND(e, td_accept(str,")"));
        return true;
    }RB_BLOCK_END();

    RB_BLOCK_START(e) {
        td_extended_column_name(str);
        return true;
    }RB_BLOCK_END();

    return false;
}


bool SimpleSqlParser::td_operate_on_expr(QString &str) {

    // binary op : expr bin_op expr [ bin_op expr] ...
    RB_BLOCK_START(ee) {
        RB_TEST_COND(ee, td_binary_operator(str));
        RB_TEST_COND(ee, td_expr(str));
        return true;
    }RB_BLOCK_END();

    RB_BLOCK_START(ee) {
        RB_TEST_COND(ee, td_accept_word(str, "COLLATE"));
        RB_TEST_COND(ee, td_collection_name(str));
        return true;
    }RB_BLOCK_END();

    // expr [NOT] LIKE expr [ESCAPE expr]
    RB_BLOCK_START(ee) {
        td_accept_word(str,"NOT");
        RB_TEST_COND(ee, (td_accept_word(str,"LIKE") || td_accept_word(str,"GLOB")
                          || td_accept_word(str,"REGEXP") || td_accept_word(str,"MATCH") ) );
        RB_TEST_COND(ee, td_expr(str));
        if(td_accept_word(str,"ESCAPE"))
            RB_TEST_COND(ee, td_expr(str));
        return true;
    }RB_BLOCK_END();

    // expr ISNULL
    RB_BLOCK_START(ee) {
        RB_TEST_COND(ee, (td_accept_word(str,"ISNULL") || td_accept_word(str,"NOTNULL")
                          || (td_accept_word(str,"NOT") && td_accept_word(str,"NULL")) ));
        return true;
    }RB_BLOCK_END();

    //expr [NOT] BETWEEN expr AND expr
    RB_BLOCK_START(ee) {
        td_accept_word(str,"NOT");
        RB_TEST_COND(ee, td_accept_word(str,"BETWEEN"));
        RB_TEST_COND(ee, td_expr(str));
        RB_TEST_COND(ee, td_accept_word(str,"AND"));
        RB_TEST_COND(ee, td_expr(str));
        return true;
    }RB_BLOCK_END();

    // expr [NOT] IN ( select_stmt )
    RB_BLOCK_START(ee) {
        td_accept_word(str,"NOT");
        RB_TEST_COND(ee, td_accept_word(str,"IN"));
        RB_BLOCK_START(eee) {
            RB_TEST_COND(eee, td_accept(str,"("));
            RB_TEST_COND(eee, td_select_stmt(str));
            RB_TEST_COND(eee, td_accept(str,")"));
            return true;
        }RB_BLOCK_END();
        RB_BLOCK_START(eee) {
            RB_TEST_COND(eee, td_accept(str,"("));
            RB_TEST_COND(eee, td_expr(str));
            RB_LOOP_START(eeee) {
                RB_TEST_COND(eeee, td_accept(str,","));
                RB_TEST_COND(eeee, td_expr(str));
            }
            RB_TEST_COND(eee, td_accept(str,")"));
            return true;
        }RB_BLOCK_END();
        RB_BLOCK_START(eee) {
            RB_BLOCK_START(eeee) {
                RB_TEST_COND(eeee, td_database_name(str));
                RB_TEST_COND(eeee, td_accept(str,"."));
            }RB_BLOCK_END();
            RB_TEST_COND(eee, td_table_name(str));
            return true;
        }RB_BLOCK_END();
    }RB_BLOCK_END();
    return true;
}

bool SimpleSqlParser::td_expr(QString &str) {
    if(!td_simple_expr(str))
        return false;

    td_operate_on_expr(str);

    return true;
}

bool SimpleSqlParser::td_result_column(QString &str) {
    if(td_accept(str,"*"))
        return true;
    RB_BLOCK_START(t) {
        RB_TEST_COND(t, td_table_name(str));
        RB_TEST_COND(t, td_accept(str,"."));
        RB_TEST_COND(t, td_accept(str,"*"));
        return true;
    }RB_BLOCK_END();

    RB_BLOCK_START(e) {
        td_expr(str);
        if(td_accept_word(str,"AS")) {
            RB_TEST_COND(e, td_column_alias(str));
        } else td_column_alias(str);
        return true;
    }RB_BLOCK_END();
    return false;
}

bool SimpleSqlParser::td_join_op(QString &str) {
    if(td_accept(str,","))
        return true;
    RB_BLOCK_START(j) {
        td_accept_word(str,"NATURAL");
        if(td_accept_word(str,"LEFT"))
            td_accept_word(str,"OUTER");
        else (td_accept_word(str,"INNER") || td_accept_word(str,"CROSS"));
        RB_TEST_COND(j, td_accept_word(str,"JOIN"));
        return true;
    }RB_BLOCK_END();
    return false;
}

bool SimpleSqlParser::td_join_constraint(QString &str) {
    RB_BLOCK_START(o) {
        RB_TEST_COND(o, td_accept_word(str, "ON"));
        RB_TEST_COND(o, td_expr(str));
        return true;
    }RB_BLOCK_END();

    RB_BLOCK_START(u) {
        RB_TEST_COND(u, td_accept_word(str, "USING"));
        RB_TEST_COND(u, td_accept(str,"("));
        RB_TEST_COND(u, td_column_name(str));
        RB_LOOP_START(uu) {
            RB_TEST_COND(uu, td_accept(str,","));
            RB_TEST_COND(uu, td_column_name(str));
        }
        RB_TEST_COND(u, td_accept(str,")"));
        return true;
    }RB_BLOCK_END();

    return true;
}

bool SimpleSqlParser::td_single_source(QString &str) {

    RB_BLOCK_START(s) {
        RB_TEST_COND(s, td_accept(str,"("));
        RB_TEST_COND(s, td_select_stmt(str));
        RB_TEST_COND(s, td_accept(str,")"));
        if(td_accept_word(str,"AS")) {
            RB_TEST_COND(s, td_table_alias(str));
        }else td_table_alias(str);
        return true;
    }RB_BLOCK_END();

    RB_BLOCK_START(j) {
        RB_TEST_COND(j, td_accept(str,"("));
        RB_TEST_COND(j, td_join_source(str));
        RB_TEST_COND(j, td_accept(str,")"));
        return true;
    }RB_BLOCK_END();

    RB_BLOCK_START(n) {
        RB_BLOCK_START(nn) {
            RB_TEST_COND(nn, td_database_name(str));
            RB_TEST_COND(nn, td_accept(str,"."));
        }RB_BLOCK_END();
        RB_TEST_COND(n, td_table_name(str));
        if(td_accept_word(str,"AS")) {
            RB_TEST_COND(n, td_table_alias(str));
        } else td_table_alias(str);

        RB_BLOCK_START(nn) {
            RB_TEST_COND(nn, td_accept_word(str,"NOT"));
            RB_TEST_COND(nn, td_accept_word(str,"INDEXED"));
            return true;
        }RB_BLOCK_END();

        RB_BLOCK_START(nn) {
            RB_TEST_COND(nn, td_accept_word(str,"INDEXED"));
            RB_TEST_COND(nn, td_accept_word(str,"BY"));
            RB_TEST_COND(nn, td_index_name(str));
            return true;
        }RB_BLOCK_END();
        return true;
    }RB_BLOCK_END();
    return false;
}

bool SimpleSqlParser::td_join_source(QString &str) {
    if(!td_single_source(str))
        return false;
    RB_LOOP_START(j){
        RB_TEST_COND(j, td_join_op(str));
        RB_TEST_COND(j, td_single_source(str));
        RB_TEST_COND(j, td_join_constraint(str));
    }
    return TRUE;
}

bool SimpleSqlParser::td_select_core(QString &str) {
    if(!td_accept_word(str, "SELECT"))
        return false;

    if(qContext->inSelectArgs)
        qContext->error_select_nested_in_select = true;

    qContext = qContext->newSubContext();
    qContext->inSelectArgs = true;
    (td_accept_word(str, "DISTINCT") || td_accept_word(str, "ALL"));
    td_result_column(str);

    RB_LOOP_START(rc) {
        RB_TEST_COND(rc, td_accept(str, ","));
        RB_TEST_COND(rc, td_result_column(str));
    }
    qContext->inSelectArgs = false;

    qContext->inFrom = true;
    RB_BLOCK_START(f) {
        RB_TEST_COND(f, td_accept_word(str,"FROM"));
        RB_TEST_COND(f, td_join_source(str));
    }RB_BLOCK_END();
    qContext->inFrom = false;

    qContext->inWhere = true;
    RB_BLOCK_START(w) {
        RB_TEST_COND(w, td_accept_word(str,"WHERE"));
        RB_TEST_COND(w, td_expr(str));
    } RB_BLOCK_END();
    qContext->inWhere = false;

    qContext->inGroup = true;
    RB_BLOCK_START(g) {
        RB_TEST_COND(g, td_accept_word(str,"GROUP"));
        RB_TEST_COND(g, td_accept_word(str,"BY"));
        RB_TEST_COND(g, td_expr(str));
        qContext->isAggregatedQuery = true;
        RB_LOOP_START(gg) {
            RB_TEST_COND(gg, td_accept(str,","));
            RB_TEST_COND(gg, td_expr(str));
        }
    }RB_BLOCK_END();
    qContext->inGroup = false;

    qContext->inHaving = true;
    RB_BLOCK_START(h) {
        RB_TEST_COND(h, td_accept_word(str, "HAVING"));
        RB_TEST_COND(h, td_expr(str));
    }RB_BLOCK_END();
    qContext->inHaving = false;

    qContext = qContext->exitSubContext();
    return true;
}

bool SimpleSqlParser::td_compound_operator(QString &str) {
    if(td_accept_word(str, "UNION")) {
        td_accept_word(str, "ALL");
        return true;
    }
    return (td_accept_word(str, "INTERSECT") || td_accept_word(str, "EXCEPT"));
}

bool SimpleSqlParser::td_ordering_term(QString &str) {
    if(!td_expr(str))
        return false;
    RB_BLOCK_START(c) {
        RB_TEST_COND(c, td_accept_word(str,"COLLATE"));
        RB_TEST_COND(c, td_collection_name(str));
    }RB_BLOCK_END();

    (td_accept_word(str,"ASC") || td_accept_word(str,"DESC"));

    return true;
}

bool SimpleSqlParser::td_select_stmt(QString &str) {
    if(!td_select_core(str)) {
        qContext = qContext->exitSubContext();
        return false;
    }
    RB_LOOP_START(c) {
        RB_TEST_COND(c, td_compound_operator(str));
        RB_TEST_COND(c, td_select_core(str));
    }
    RB_BLOCK_START(o) {
        RB_TEST_COND(o, td_accept_word(str, "ORDER"));
        RB_TEST_COND(o, td_accept_word(str, "BY"));
        RB_TEST_COND(o, td_ordering_term(str));
        RB_LOOP_START(oo) {
            RB_TEST_COND(oo, td_accept(str ,","));
            RB_TEST_COND(oo, td_ordering_term(str));
        }
    }RB_BLOCK_END();

    RB_BLOCK_START(l) {
        RB_TEST_COND(l, td_accept_word(str, "LIMIT"));
        RB_TEST_COND(l, td_expr(str));
        RB_BLOCK_START(ll) {
            RB_TEST_COND(ll, (td_accept_word(str, "OFFSET") || td_accept(str, ",")));
            RB_TEST_COND(ll, td_expr(str))
        } RB_BLOCK_END();
    } RB_BLOCK_END();

    return true;
}

void SimpleSqlParser::td_parse(QString &str)
{
    td_select_stmt(str);
}


void SimpleSqlParser::parse(QString str)
{
    QString str_copy(str);
    td_parse(str_copy);
    if(qContext->error_unaggregated_args)
        qDebug() << "selection of unaggregated args detected";
    if(qContext->error_select_nested_in_select)
        qDebug() << "nested select in select line detected";

}
