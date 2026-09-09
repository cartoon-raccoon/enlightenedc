// test individually, should produce syntax errors

// Preprocessor syntax errors

// Missing identifier
#define

// Missing string after include
#include



// Declaration syntax errors

// Missing semicolon
I64 x = 5

// Missing identifier in declarator
I64 *;

// Malformed declarator (unclosed paren)
I64 (x;

// Missing initializer expression
I64 x = ;



// Struct syntax errors

// Missing semicolon in struct member
struct S { I64 x }

// Missing closing brace
struct S { I64 x;

// Invalid bitfield (missing width)
struct B { I64 x : ; };



// Enum syntax errors

// Missing enumerator value
enum E { A = };

// Double comma
enum E { A, , B };

// Missing closing brace
enum E { A, B;



// Array syntax errors

// Missing closing bracket
I64 arr[10;

// Empty subscript expression (grammar requires expression)
I64 arr[ ];



// Function definition syntax errors

// Missing function body
I64 f(I64 x)

// Malformed parameter list
I64 f(, I64 x) { return x; }

// Missing closing brace in function
I64 f(I64 x) { return x;



// Compound statement syntax errors

// Missing closing brace
{ I64 x = 1;

// Invalid mixture (bad order)
{ I64 x = 1; I64 y }



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
I64 x = { };

// Missing closing brace
I64 x = { 1, 2;



// Parameter syntax errors

// Default without expression
I64 f(I64 x =) { }

// Ellipsis not last, with a separating comma
I64 f(..., I64 x) { }

// Ellipsis not last, without a separating comma
I64 f(... I64 x) { }

// Named parameters both before and after the ellipsis
I64 f(I32 a, ..., I64 x) { }

// Two ellipses in one parameter list
I64 f(..., ...) { }



// Semantic errors

// Default argument used in a call through a function pointer.
// A short call is only allowed when it names the function directly, so the
// missing argument for `b` cannot be filled from its default here.
I64 with_default(I64 a, I64 b = 5) { return a + b; }
I64 (*with_default_fp)(I64, I64) = with_default;
with_default_fp(1);