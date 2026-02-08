%{

#include <iostream>
#include <string>

#include "lexer.hpp"
    
%}

%option c++
%option noyywrap
%option yylineno
%option yyclass="ecc::frontend::Lexer"

%%

[ \r\n\t] { /* skip whitespace */ }

%%

