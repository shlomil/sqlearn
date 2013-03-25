#ifndef SIMPLESQLPARSER_H
#define SIMPLESQLPARSER_H

#include <QtCore>

class SimpleSqlParser
{
public:
    SimpleSqlParser();
    void parse(QString str);
    bool detected_error_unaggregated_args();
    bool detected_error_select_nested_in_select();
    bool detected_error_nested_call_to_aggregated_func();


private:
    class QueryContext;
    QueryContext* qContext;
    QString lastToken;

    bool error_unaggregated_args;
    bool error_select_nested_in_select;
    bool error_nested_call_to_aggregated_func;

    bool td_accept(QString &str, QString acc_str);
    bool td_accept(QString &str, int count);
    bool td_accept_any_word(QString &str);
    bool td_accept_word(QString &str, QString word);
    bool td_numeric_literal(QString &str);
    QString format_escape_entity(QString s);
    bool td_string_literal_quote_char(QString &str, QChar qc);
    bool td_string_literal(QString &str);
    bool td_blob_literal(QString &str);
    bool td_literal_value(QString &str);
    bool td_binary_operator(QString &str);
    bool td_table_name(QString &str);
    bool td_database_name(QString &str);
    bool td_collection_name(QString &str);
    bool td_index_name(QString &str);
    bool td_function_name(QString &str) ;
    bool td_type_name(QString &str);
    bool td_table_alias(QString &str);
    bool td_column_alias(QString &str);
    bool td_column_name(QString &str);
    bool td_extended_column_name(QString &str);
    bool td_simple_expr(QString &str);
    bool td_operate_on_expr(QString &str);
    bool td_expr(QString &str);
    bool td_result_column(QString &str);
    bool td_join_op(QString &str);
    bool td_join_constraint(QString &str);
    bool td_single_source(QString &str);
    bool td_join_source(QString &str);
    bool td_select_core(QString &str);
    bool td_compound_operator(QString &str);
    bool td_ordering_term(QString &str);
    bool td_select_stmt(QString &str);
    void td_parse(QString &str);
};

#endif // SIMPLESQLPARSER_H
