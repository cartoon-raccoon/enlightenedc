%%
logical_or_expression:
      logical_and_expression
    | logical_or_expression OROR logical_and_expression
    ;

logical_and_expression:
      inclusive_or_expression
    | logical_and_expression ANDAND inclusive_or_expression
    ;

inclusive_or_expression:
      exclusive_or_expression
    | inclusive_or_expression OR exclusive_or_expression
    ;

exclusive_or_expression:
      and_expression
    | exclusive_or_expression XOR and_expression
    ;

and_expression:
      equality_expression
    | and_expression AND equality_expression
    ;

equality_expression:
      relational_expression
    | equality_expression EQ relational_expression
    | equality_expression NE relational_expression
    ;

relational_expression:
      shift_expression
    | relational_expression LT shift_expression
    | relational_expression GT shift_expression
    | relational_expression LE shift_expression
    | relational_expression GE shift_expression
    ;

shift_expression:
      additive_expression
    | shift_expression LSHIFT additive_expression
    | shift_expression RSHIFT additive_expression
    ;

additive_expression:
      multiplicative_expression
    | additive_expression PLUS multiplicative_expression
    | additive_expression MINUS multiplicative_expression
    ;

multiplicative_expression:
      unary_expression
    | multiplicative_expression MUL unary_expression
    | multiplicative_expression DIV unary_expression
    | multiplicative_expression MOD unary_expression
    ;

%%