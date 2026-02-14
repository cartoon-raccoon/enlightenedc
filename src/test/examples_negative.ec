// test individually, should produce syntax errors

// Preprocessor syntax errors

// Missing identifier
#define

// Missing string after include
#include



// Declaration syntax errors

// Missing semicolon
I64i x = 5

// Missing identifier in declarator
I64i *;

// Malformed declarator (unclosed paren)
I64i (x;

// Missing initializer expression
I64i x = ;



// Struct syntax errors

// Missing semicolon in struct member
struct S { I64i x }

// Missing closing brace
struct S { I64i x;

// Invalid bitfield (missing width)
struct B { I64i x : ; };



// Enum syntax errors

// Missing enumerator value
enum E { A = };

// Double comma
enum E { A, , B };

// Missing closing brace
enum E { A, B;



// Array syntax errors

// Missing closing bracket
I64i arr[10;

// Empty subscript expression (grammar requires expression)
I64i arr[ ];



// Function definition syntax errors

// Missing function body
I64i f(I64i x)

// Malformed parameter list
I64i f(, I64i x) { return x; }

// Missing closing brace in function
I64i f(I64i x) { return x;



// Compound statement syntax errors

// Missing closing brace
{ I64i x = 1;

// Invalid mixture (bad order)
{ I64i x = 1; I64i y }



// If statement syntax errors

// Missing parentheses
if x > 0 { }

// Missing statement after condition
if (x > 0)



// Switch syntax errors

// Missing parentheses
switch x { }

// Missing braces
switch (x) case 1: break;

// Malformed case label
switch (x) {
    case : break;
}



// Case range syntax errors

// Missing second expression
switch (x) {
    case 1 ... : break;
}



// Loop syntax errors

// While missing parentheses
while x < 5 { }

// Do-while missing parentheses
do { } while x < 5;

// For missing semicolons
for (i = 0 i < 5; i++) { }

// For missing closing paren
for (i = 0; i < 5; i++ { }



// Jump statement syntax errors

// Missing label
goto ;

// Return missing semicolon
return



// Expression syntax errors

// Missing RHS
x += ;

// Incomplete ternary
x ? y;

// Missing false branch
x ? y : ;

// Invalid operator sequence
x + * y;

// Missing operand
x && ;

// Empty sizeof
sizeof();

// Invalid sizeof form
sizeof(,);



// Postfix expression syntax errors

// Missing index expression
x[ ];

// Missing member name
x->;



// Primary expression syntax errors

// Empty parentheses
();

// Malformed grouped expression
(1 + );



// Initializer syntax errors

// Missing expression inside braces
I64i x = { };

// Missing closing brace
I64i x = { 1, 2;



// Parameter syntax errors

// Default without expression
I64i f(I64i x =) { }

// Ellipsis not last
I64i f(..., I64i x) { }