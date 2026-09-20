// Tests for the scoped lexer hack (LexerHack): a type name introduced by a
// class/union/enum specifier should only be recognized as a TYPE_IDENTIFIER
// within the scope it was declared in (and scopes nested inside that one),
// not for the rest of the file.
//
// Under the old, unscoped hack, every check below that reuses a name as a
// plain identifier after its declaring scope has closed would have failed to
// parse at all, because the name would have been stuck as a TYPE_IDENTIFIER
// forever after its first type declaration.

// --- File scope: a type declared at the top level is visible from here on ---

class Point {
    U32 x, y;
};

Point globalPoint = { .x = 1, .y = 2 };
"%d %d\n", globalPoint.x, globalPoint.y; // 1 2

// --- A type declared inside a function is visible for the rest of that
//     function's body, including in nested blocks ---

U0 UsesLocalType() {
    class Local {
        U32 val;
    };

    Local l = { .val = 5 };
    "%d\n", l.val; // 5

    {
        // Nested block: Local should still resolve as a type here, since
        // contains_type() walks outward through enclosing scopes.
        Local inner = { .val = 6 };
        "%d\n", inner.val; // 6
    }
}

// --- Once UsesLocalType's scope has closed, "Local" is no longer a type
//     name: it can be reused as an ordinary identifier here, at file scope,
//     with no relation at all to the class declared above ---

U32 Local = 42;
"%d\n", Local; // 42

// --- Same story, but the reuse happens inside a different function instead
//     of at file scope ---

U0 ReusesNameAsVariable() {
    // If this were still classified as a TYPE_IDENTIFIER, "Local = 7;" would
    // be a parse error (a bare type name is not a valid expression statement).
    U32 Local = 7;
    Local = Local + 1;
    "%d\n", Local; // 8
}

// --- A type declared in one function is not visible in a sibling function,
//     even though both are at the same nesting depth ---

U0 DeclaresAnotherLocalType() {
    enum Status { OK, FAIL };
    Status s = OK;
    "%d\n", s; // 0
}

U0 CannotSeeStatus() {
    // "Status" was scoped to DeclaresAnotherLocalType's body, so it is just
    // an ordinary identifier here.
    U32 Status = 9;
    "%d\n", Status; // 9
}

Point globalPoint2 = { .x = 3, .y = 4 };
"%d %d\n", globalPoint2.x, globalPoint2.y; // 3 4

UsesLocalType();
ReusesNameAsVariable();
DeclaresAnotherLocalType();
CannotSeeStatus();
