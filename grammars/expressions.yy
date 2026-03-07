// Operator precedence.
%left OROR
%left ANDAND
%left OR
%left XOR
%left AND
%left EQ NE
%left LT GT LE GE
%left LSHIFT RSHIFT
%left PLUS MINUS
%left MUL DIV MOD

//for ternary
%nonassoc QUESTION

%%
binary_expression:
    binary_expression OROR binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::OROR);
    }
    | binary_expression ANDAND binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::ANDAND);
    }
    | binary_expression OR binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::OR);
    }
    | binary_expression XOR binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::XOR);
    }
    | binary_expression AND binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::AND);
    }
    | binary_expression EQ binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::EQ);
    }
    | binary_expression NE binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::NE);
    }
    | binary_expression LT binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::LT);
    }
    | binary_expression GT binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::GT);
    }
    | binary_expression LE binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::LE);
    }
    | binary_expression GE binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::GE);
    }
    | binary_expression LSHIFT binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::LSHIFT);
    }
    | binary_expression RSHIFT binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::RSHIFT);
    }
    | binary_expression PLUS binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::PLUS);
    }
    | binary_expression MINUS binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::MINUS);
    }
    | binary_expression MUL binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::MUL);
    }
    | binary_expression DIV binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::DIV);
    }
    | binary_expression MOD binary_expression {
        $$ = std::make_unique<BinaryExpression>(@$, std::move($1), std::move($3), ecc::tokens::MOD);
    }
    | cast_expression {
        $$ = std::move($1);
    }
;

%%