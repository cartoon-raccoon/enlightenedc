# Linkage

Identifiers may have three types of linkage: Internal, External, and None.

In the set of translation units and libraries that constitutes an entire program, each declaration of a particular identifier with external linkage denotes the same object or function.
Within one translation unit, each declaration of an identifier with internal linkage denotes the same object or function. Each declaration of an identifier with no linkage denotes a unique entity.

For a block-scope identifier declared with the storage-class specifier `global` where a prior declaration of that identifier is visible, the later declaration shall inherit the linkage of the prior declaration.

## Language Linkage

Separate from linkage is language linkage, marked using a string literal such as "C". There are two types of language linkage, None, and C linkage.

## Constraints

Any identifier with a language linkage other than None must have an external linkage.

C language linkage may only be applied to identifiers corresponding to a function.
