%{

#include <iostream>
#include <string>

#include "parser.hpp"
    
%}

%option c++
%option noyywrap
// %option batch
// %option debug

%option yylineno

%%

// rules go here

%%

namespace ecc::frontend {

class Lexer : public yyFlexLexer {
    
}

}