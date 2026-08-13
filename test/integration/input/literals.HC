U32 a = 123;
U32 b = -45;
U32 c = 077;
U32 d = 0xFF;

F64 f1 = 1.23;
F64 f2 = -3.14e10;

U8 ch = 'A';
U8 nl = '\n';
U8 qt = '\"';

"%d %d %d %d\n", a, b, c, d;
"%f %f\n", f1, f2;

// Lowercase hex digits (only uppercase is exercised above).
U32 e = 0xff;
"%d\n", e;

// Float forms not exercised above: trailing dot, and a bare exponent with no
// fractional part.
F64 f3 = 5.;
F64 f4 = 6e2;
"%f %f\n", f3, f4;

// Escape sequences beyond \n and \".
U8 tb = '\t';
U8 bs = '\\';
U8 sq = '\'';
U8 oct = '\7';   // octal escape

"%d %d %d %d\n", tb, bs, sq, oct;

// Multi-digit hex/octal escapes are only valid in string literals, not char
// literals (a char literal admits exactly one escaped character).
"hex: \x41, octal: \101\n";

// Adjacent string literal concatenation.
"concatenated " "string\n";