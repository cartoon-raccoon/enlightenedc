#include "semantics/types.hpp"

#include <cfloat>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>

#include "codegen/llvm.hpp"
#include "semantics/symbols.hpp"
#include "semantics/typeerr.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace ecc::sema::types;
using namespace ecc::tokens;
using namespace ecc::codegen;

/*
 * TYPE METHODS
 */

size_t Type::alloc_size() {
    // If no type, call finalize
    if (!llvm_type) {
        finalize();
    }

    // Check that type has been initialized properly
    if (!llvm_type && finalized) {
        throw std::runtime_error("LLVM Type not initialized on type, cannot get size");
    }

    const llvm::DataLayout& dl = ctxt().llvm().mod().getDataLayout();
    return dl.getTypeAllocSize(llvm_type);
}

bool Type::is_void() const {
    return kind == Kind::VOID;
}

bool Type::is_primitive() const {
    return kind == Kind::PRIMITIVE;
}

bool Type::is_class() const {
    return kind == Kind::CLASS;
}

bool Type::is_union() const {
    return kind == Kind::UNION;
}

bool Type::is_enum() const {
    return kind == Kind::ENUM;
}

bool Type::is_pointer() const {
    return kind == Kind::POINTER;
}

bool Type::is_array() const {
    return kind == Kind::ARRAY;
}

bool Type::is_function() const {
    return kind == Kind::FUNCTION;
}

LLVMType *Type::get_llvmtype() {
    if (llvm_type)
        return llvm_type;

    finalize();

    assert(llvm_type);
    return llvm_type;
}

void UserType::validate_member_type(Type *type, Optional<std::string> name, Location loc) {
    if (type == this) {
        // check for recursion without indirection

        // member is guaranteed to have name, as anonymous types cannot be re-referenced
        throw RecursiveTypeError(*name, loc);
    }

    if (type->is_usertype()) {
        // if member is usertype, check for completeness
        UserType *uty = type->as_usertype();
        if (!uty->is_complete()) {
            throw IncompleteTypeUseError(*uty->name, uty->decl_loc);
        }
    } else if (type->is_void()) {
        // check if type is void
        throw InvalidVoidError(loc);
    } else if (type->is_array()) {
        // if type is array:
        ArrayType *arrayty = type->as_array();
        if (arrayty->base == this) {
            // check for recursive errors with arrays as well
            throw RecursiveTypeError(*name, loc);
        }
        if (!arrayty->arr_size) {
            // check that array is sized
            throw UnsizedArrInUserTypeError(loc);
        }
    }
}

/*
 * VOID TYPE METHODS
 */

void VoidType::finalize() {
    if (finalized) {
        assert(llvm_type && "VoidType marked finalized but llvm_type is null");
        dbprint("VoidType: already finalized, skipping");
        return;
    }

    dbprint("VoidType: finalizing");
    llvm_type = llvm::Type::getVoidTy(ctxt().llvm().ctx());

    finalized = true;
}

/*
 * PRIMITIVE TYPE METHODS
 */
bool PrimitiveType::is_integer() const {
    return pr_is_integer(primkind);
}

bool PrimitiveType::is_signed() const {
    return pr_is_signed(primkind);
}

bool PrimitiveType::is_float() const {
    return pr_is_float(primkind);
}

bool PrimitiveType::is_bool() const {
    return pr_is_bool(primkind);
}

bool PrimitiveType::is_compatible_with(Type *from) {
    // Only primitive types allowed
    if (from->kind != Type::Kind::PRIMITIVE) {
        return false;
    }

    PrimitiveType *new_from = from->as_primitive();
    if (is_bool()) {
        // bool can be promoted to anything numeric
        return new_from->is_integer() || new_from->is_float();
    }

    // If either is not an integer, return false
    if (!this->is_integer() && !new_from->is_integer()) {
        return false;
    }

    size_t my_size = this->alloc_size();

    return 0 < my_size && my_size <= new_from->alloc_size();
}

Optional<uint64_t> PrimitiveType::int_max() const {
    switch (primkind) {
    case PrimType::U8:
        return UINT8_MAX;
    case PrimType::U16:
        return UINT16_MAX;
    case PrimType::U32:
        return UINT32_MAX;
    case PrimType::U64:
        return UINT64_MAX;
    case PrimType::BOOL:
        return 1;
    case PrimType::I8:
        return INT8_MAX;
    case PrimType::I16:
        return INT16_MAX;
    case PrimType::I32:
        return INT32_MAX;
    case PrimType::I64:
        return INT64_MAX;
    case PrimType::F32:
    case PrimType::F64:
        return {};
    }
}

Optional<double> PrimitiveType::flt_max() const {
    return primkind == PrimType::F64 ? DBL_MAX : Optional<double>{};
}

void PrimitiveType::finalize() {
    if (finalized) {
        assert(llvm_type && "PrimitiveType marked finalize but llvm_type is null");
        dbprint("PrimitiveType: ", to_string(), " already finalized, skipping");
        return;
    }
    dbprint("PrimitiveType: finalizing ", to_string());

    switch (primkind) {
    case PrimType::U8:
    case PrimType::I8:
    case PrimType::BOOL: //? should bool be 1 bit?
        llvm_type = llvm::Type::getInt8Ty(ctxt().llvm().ctx());
        break;

    case PrimType::U16:
    case PrimType::I16:
        llvm_type = llvm::Type::getInt16Ty(ctxt().llvm().ctx());
        break;

    case PrimType::U32:
    case PrimType::I32:
        llvm_type = llvm::Type::getInt32Ty(ctxt().llvm().ctx());
        break;

    case PrimType::U64:
    case PrimType::I64:
        llvm_type = llvm::Type::getInt64Ty(ctxt().llvm().ctx());
        break;

    case PrimType::F32:
        llvm_type = llvm::Type::getFloatTy(ctxt().llvm().ctx());
        break;

    case PrimType::F64:
        llvm_type = llvm::Type::getDoubleTy(ctxt().llvm().ctx());
        break;
    }

    finalized = true;
}

std::string PrimitiveType::formal() {
    switch (primkind) {
    case PrimType::U8:
        return "U8";
    case PrimType::U16:
        return "U16";
    case PrimType::U32:
        return "U32";
    case PrimType::U64:
        return "U64";
    case PrimType::I8:
        return "I8";
    case PrimType::I16:
        return "I16";
    case PrimType::I32:
        return "I32";
    case PrimType::I64:
        return "I64";
    case PrimType::BOOL:
        return "Bool";
    case PrimType::F32:
        return "F32";
    case PrimType::F64:
        return "F64";
    }
}

/*
 * CLASS TYPE METHODS
 */

bool ClassType::is_fully_defined() {
    if (!is_complete())
        return false;

    bool ret = true;
    for (auto& member : members) {
        switch (member->ty->kind) {
        case Kind::CLASS: {
            ClassType *cls = member->ty->as_class();
            assert(cls);
            ret = ret && cls->is_fully_defined();
        } break;

        case Kind::UNION: {
            UnionType *unn = member->ty->as_union();
            assert(unn);
            ret = ret && unn->is_fully_defined();
        } break;

        case Kind::ENUM: {
            EnumType *enm = member->ty->as_enum();
            assert(enm);
            ret = ret && enm->is_fully_defined();
        } break;

        // This branch shouldn't be reached because array sizedness is checked
        // when adding a member anyway.
        case Kind::ARRAY: {
            ArrayType *arr = member->ty->as_array();
            assert(arr);
            ret = ret && arr->arr_size.has_value();
        } break;

        default: {
            ret = ret && true; // NOLINT
        } break;
        }
    }

    return ret;
}

void ClassType::finalize() {
    if (finalized) {
        assert(llvm_type && "ClassType marked finalize but llvm_type is null");
        dbprint("ClassType: already finalized, skipping");
        return;
    }
    dbprint("ClassType: finalizing class defined at ", def_loc);

    if (!is_complete()) {
        throw TypeSemError("class not fully defined", decl_loc);
    }

    Vec<LLVMType *> args;
    args.reserve(num_members());
    for (auto& member : members) {
        dbprint("UnionType: finalizing member declared at ", member->loc);
        member->ty->finalize();
        assert(member->ty->get_llvmtype());
        args.push_back(member->ty->get_llvmtype());
    }

    if (name) {
        llvm_type = llvm::StructType::create(ctxt().llvm().ctx(), args, *name);
    } else {
        llvm_type = llvm::StructType::create(ctxt().llvm().ctx(), args);
    }

    finalized = true;
}

void ClassType::add_member(std::string name, Type *type, Location loc) {
    dbprint("ClassType: adding member with name ", name, " type ", type);

    validate_member_type(type, name, loc);

    Box<ClassTypeMember> member = std::make_unique<ClassTypeMember>(name, type, loc);
    members.push_back(std::move(member));
}

void ClassType::add_member(Type *type, Location loc) {
    dbprint("ClassType: adding anonymous member with type ", type);

    validate_member_type(type, {}, loc);

    Box<ClassTypeMember> member = std::make_unique<ClassTypeMember>(type, loc);
    members.push_back(std::move(member));
}

ClassType::ClassTypeMember *ClassType::find(std::string& name) {
    for (auto& mem : members) {
        if (mem->name) {
            if (mem->name == name) {
                return mem.get();
            }
        }
    }

    return nullptr;
}

ClassType::ClassTypeMember *ClassType::index(int idx) {
    if (idx >= num_members()) {
        return nullptr;
    }
    return members[idx].get();
}

std::string ClassType::formal() {
    return name ? base() + *name : "class_anon";
}

/*
 * UNION TYPE METHODS
 */

bool UnionType::is_fully_defined() {
    if (!is_complete())
        return false;

    bool ret = true;
    for (auto& member : members) {
        switch (member->ty->kind) {
        case Kind::CLASS: {
            ClassType *cls = member->ty->as_class();
            assert(cls);
            ret = ret && cls->is_fully_defined();
        } break;

        case Kind::UNION: {
            UnionType *unn = member->ty->as_union();
            assert(unn);
            ret = ret && unn->is_fully_defined();
        } break;

        case Kind::ENUM: {
            EnumType *enm = member->ty->as_enum();
            assert(enm);
            ret = ret && enm->is_fully_defined();
        } break;

        default: {
            // explicitly express that any other class is implicitly fully defined.
            ret = ret && true; // NOLINT
        } break;
        }
    }

    return ret;
}

bool UnionType::is_compatible_with(Type *dst) {
    // If there is an underlying type representative, check that
    if (type_rep) {
        return (*type_rep)->is_compatible_with(dst);
    }

    // Otherwise, unions are incompatible with anything
    return false;
}

void UnionType::add_member(std::string name, Type *type, Location loc) {
    dbprint("UnionType: adding member with name ", name, " type ", type);

    validate_member_type(type, name, loc);

    Box<UnionTypeMember> member = std::make_unique<UnionTypeMember>(name, type, loc);
    members.push_back(std::move(member));
}

void UnionType::add_member(Type *type, Location loc) {
    dbprint("UnionType: adding anonymous member with type ", type);

    validate_member_type(type, {}, loc);

    Box<UnionTypeMember> member = std::make_unique<UnionTypeMember>(type, loc);
    members.push_back(std::move(member));
}

UnionType::UnionTypeMember *UnionType::find(std::string& name) {
    for (auto& mem : members) {
        if (mem->name) {
            if (mem->name == name) {
                return mem.get();
            }
        }
    }

    return nullptr;
}

UnionType::UnionTypeMember *UnionType::index(int idx) {
    return members[idx].get();
}

void UnionType::finalize() {
    /*
    If the union has a type representative, that becomes the final type of the union.
    Otherwise, the LLVM type of the union becomes that of the largest member.
    */
    if (finalized) {
        assert(llvm_type && "UnionType marked finalize but llvm_type is null");
        dbprint("UnionType: already finalized, skipping");
        return;
    }
    dbprint("UnionType: finalizing union defined at ", def_loc);

    if (!is_complete()) {
        throw TypeSemError("union not fully defined", decl_loc);
    }

    if (type_rep) {
        (*type_rep)->finalize();
    }
    // Finalize all members first
    for (auto& member : members) {
        dbprint("UnionType: finalizing member declared at ", member->loc);
        member->ty->finalize();
        assert(member->ty->get_llvmtype());
    }

    if (type_rep) {
        // since type_rep is primitive, it is guaranteed to be finalized
        llvm_type = (*type_rep)->get_llvmtype();

        // todo: validate that no members are larger than type rep
        todo();
    } else {
        // todo: get largest member and set that as llvm_type
        todo();
    }

    finalized = true;
}

std::string UnionType::formal() {
    return name ? base() + *name : "union_anon";
}

/*
 * ENUM TYPE METHODS
 */

EnumType::EnumType(Location decl_loc, sema::sym::Scope *scope, TypeContext& tyctxt)
    : UserType(decl_loc, Kind::ENUM, tyctxt, scope),
      // default type for an enum with no declared underlying type is I32.
      underlying(ctxt().get_i32()) {
}

EnumType::EnumType(Location decl_loc, std::string name, sema::sym::Scope *scope,
                   TypeContext& tyctxt)
    : UserType(decl_loc, Kind::ENUM, std::move(name), tyctxt, scope),
      // default type for an enum with no declared underlying type is I32.
      underlying(ctxt().get_i32()) {
}

int64_t EnumType::add_enumerator(std::string enumerator, Location loc) {
    EnumTypeMember *prev_mem = find(enumerator);
    if (prev_mem) {
        throw EnumeratorAlrDecldError(enumerator, loc, prev_mem->loc);
    }

    /*
    Value to use is the value of the last enumerator + 1.
    Ignore any value collisions this might cause.
    */
    int64_t value = 0;
    if (!enumerators.empty()) {
        value = enumerators.back()->value + 1;
    }

    return add_enumerator(enumerator, value, loc);
}

int64_t EnumType::add_enumerator(std::string enumerator, int64_t value, Location loc) {
    EnumTypeMember *prev_mem = find(enumerator);
    if (prev_mem) {
        throw EnumeratorAlrDecldError(enumerator, loc, prev_mem->loc);
    }

    Box<EnumTypeMember> member = std::make_unique<EnumTypeMember>(enumerator, value, loc);

    enumerators.push_back(std::move(member));

    return value;
}

EnumType::EnumTypeMember *EnumType::find(std::string& name) {
    for (auto& member : enumerators) {
        if (name == member->name)
            return member.get();
    }

    return nullptr;
}

EnumType::EnumTypeMember *EnumType::find(size_t idx) {
    for (auto& member : enumerators) {
        if (name == member->name)
            return member.get();
    }

    return nullptr;
}

bool EnumType::is_compatible_with(Type *dst) {
    if (Type::is_compatible_with(dst))
        return true;

    if (!dst->is_primitive()) {
        return false;
    }

    PrimitiveType *prim = dst->as_primitive();
    return prim->is_integer();
}

void EnumType::finalize() {
    if (finalized) {
        assert(llvm_type && "EnumType marked finalized but llvm_type is null");
        dbprint("EnumType: already finalized, skipping");
        return;
    }
    underlying->finalize();
    llvm_type = underlying->get_llvmtype();

    // todo: check # of enumerators <= max value of underlying

    finalized = true;
}

std::string EnumType::formal() {
    return name ? base() + *name : "enum_anon";
}

/*
 * POINTER TYPE METHODS
 */

int PointerType::nesting_lvl() {
    int lvl = 1;

    Type *current = base;
    while (current->is_pointer()) {
        current = current->as_pointer()->base;
        lvl++;
    }

    return lvl;
}

Type *PointerType::true_base() {
    Type *curr = base;
    while (curr->is_pointer()) {
        curr = curr->as_pointer()->base;
    }

    return curr;
}

bool PointerType::is_callable() {
    /*
    A pointer is callable if it is a singly-nested pointer to a function.
    */

    return nesting_lvl() == 1 && base->is_function();
}

bool PointerType::is_compatible_with(Type *from) {
    if (Type::is_compatible_with(from))
        return true;

    PointerType *ptr = from->as_pointer();
    if (!ptr)
        return false;

    int my_nesting = nesting_lvl();
    int ds_nesting = ptr->nesting_lvl();
    Type *dst_base = ptr->true_base();

    return my_nesting == 1 && ds_nesting == 1 ? base == dst_base || dst_base->is_void()
                                              : from == this;
}

void PointerType::finalize() {
    if (finalized) {
        assert(llvm_type && "PointerType was marked finalized but llvm_type was null");
        return;
    }

    llvm_type = llvm::PointerType::get(ctxt().llvm().ctx(), 0);
    finalized = true;
}

Type *PointerType::effective_type() {
    return ctxt().get_pointer(base->effective_type(), is_const);
}

/*
 * ARRAY TYPE METHODS
 */

bool ArrayType::is_fully_sized() {
    // fixme: does not work if there is a break in the type hierarchy of arrays
    return base->is_array() ? arr_size.has_value() && base->as_array()->is_fully_sized()
                            : arr_size.has_value();
}

bool ArrayType::is_compatible_with(Type *from) {
    switch (from->kind) {

    // if the other is an array, enforce strict equality
    case Kind::ARRAY: {
        return this == from;
    }

    // if pointer, make sure bases match
    case Kind::POINTER: {
        PointerType *ptr = from->as_pointer();
        return base == ptr->base;
    }

    default:
        return false;
    }
}

void ArrayType::finalize() {
    if (finalized) {
        assert(llvm_type && "ArrayType marked finalized but llvm_type is null");
        dbprint("ArrayType: already finalized, skipping");
        return;
    }

    if (arr_size) {
        base->finalize();
        llvm_type = llvm::ArrayType::get(base->get_llvmtype(), *arr_size);
        assert(llvm_type);
    } else {
        //? would this be a problem?
        dbprint("ArrayType: no size, unable to finalize");
    }

    finalized = true;
}

Type *ArrayType::effective_type() {
    return arr_size ? ctxt().get_array(base->effective_type(), *arr_size)
                    : ctxt().get_array(base->effective_type());
}

/*
 * FUNCTION TYPE METHODS
 */

size_t FunctionType::alloc_size() {
    throw std::runtime_error("cannot call size() on FunctionType");
}

std::size_t FunctionType::hash_sig() {
    // fixme: don't use pointers
    std::size_t hash = std::hash<Type *>{}(signature.returntype);

    for (auto *param : signature.params) {
        hash ^= std::hash<Type *>{}(param) + BOOST_GOLDEN_RATIO + (hash << HASH_SHL) +
                (hash >> HASH_SHR);
    }

    return hash;
}

void FunctionType::finalize() {
    if (finalized) {
        assert(llvm_type && "FunctionType marked finalized but llvm_type is null");
        dbprint("FunctionType: already finalized, skipping");
        return;
    }
    todo();
}

Type *FunctionType::effective_type() {
    if (eff_ty) {
        return eff_ty;
    }

    todo();
}

void TypeBuilder::add_array(uint64_t size) {
    type_stack.push(Arr{size});
}

void TypeBuilder::add_array() {
    type_stack.push(Arr{{}});
}

void TypeBuilder::add_pointer(bool is_const) {
    type_stack.push(Ptr{is_const});
}

void TypeBuilder::add_function(Location loc, Vec<FuncParam> params, bool variadic) {
    type_stack.push(FnParams{loc, std::move(params), variadic});
}

void TypeBuilder::set_base(BaseType *base) {
    this->base = base;
}

Type *TypeBuilder::finalize() {
    if (!base) {
        throw std::runtime_error("TypeBuilder::finalize: cannot construct type from null base");
    }

    dbprint("TypeBuilder: finalizing type");

    Type *curr = base;
    while (!type_stack.empty()) {
        auto next_cstrctr = type_stack.top();
        std::visit(match{[this, &curr](Arr& arr) mutable {
                             // Wrap the base in an array.
                             if (arr.size) {
                                 curr = this->ctxt().get_array(curr, *arr.size);
                             } else {
                                 curr = this->ctxt().get_array(curr);
                             }
                         },
                         [this, &curr](Ptr& ptr) mutable {
                             // Wrap the base in a pointer.
                             curr = this->ctxt().get_pointer(curr, ptr.is_const);
                         },
                         [this, &curr](FnParams& fn) mutable {
                             Vec<Type *> params;
                             // map out the identifiers.
                             params.reserve(fn.params.size());
                             for (auto& param : fn.params) {
                                 params.push_back(param.type);
                             }
                             // Wrap the base as the return type in a function type.
                             curr = this->ctxt().get_function(fn.loc, curr, std::move(params),
                                                              fn.variadic);
                         }},
                   next_cstrctr);

        // Pop the stack to the next constructor
        type_stack.pop();
    }

    return curr;
}

/*
 * TYPE CONTEXT METHODS
 */

TypeContext::TypeContext(codegen::LLVMUnit& llvm)
    : llvmref(llvm), voidt(std::make_unique<VoidType>(*this)),
      u8(std::make_unique<PrimitiveType>(PrimType::U8, *this)),
      u16(std::make_unique<PrimitiveType>(PrimType::U16, *this)),
      u32(std::make_unique<PrimitiveType>(PrimType::U32, *this)),
      u64(std::make_unique<PrimitiveType>(PrimType::U64, *this)),
      i8(std::make_unique<PrimitiveType>(PrimType::I8, *this)),
      i16(std::make_unique<PrimitiveType>(PrimType::I16, *this)),
      i32(std::make_unique<PrimitiveType>(PrimType::I32, *this)),
      i64(std::make_unique<PrimitiveType>(PrimType::I64, *this)),
      f32(std::make_unique<PrimitiveType>(PrimType::F32, *this)),
      f64(std::make_unique<PrimitiveType>(PrimType::F64, *this)),
      boolt(std::make_unique<PrimitiveType>(PrimType::BOOL, *this)) {
}

TypeBuilder TypeContext::builder() {
    return TypeBuilder(*this);
}

VoidType *TypeContext::get_void() {
    return voidt.get();
}

PrimitiveType *TypeContext::get_primitive(PrimType pkind) {
    dbprint("TypeContext: getting primitive type");
    switch (pkind) {
    case PrimType::U8:
        return u8.get();
    case PrimType::U16:
        return u16.get();
    case PrimType::U32:
        return u32.get();
    case PrimType::U64:
        return u64.get();
    case PrimType::I8:
        return i8.get();
    case PrimType::I16:
        return i16.get();
    case PrimType::I32:
        return i32.get();
    case PrimType::I64:
        return i64.get();
    case PrimType::F32:
        return f32.get();
    case PrimType::F64:
        return f64.get();
    case PrimType::BOOL:
        return boolt.get();
    }

    std::unreachable();
}

Pair<PrimitiveType *, PrimitiveType *> TypeContext::promote(PrimType p1, PrimType p2) {
    PrimType promoted = tokens::pr_promote(p1, p2);

    return {get_primitive(promoted), get_primitive(promoted)};
}

ClassType *TypeContext::get_class(Location decl_loc, std::string& name, sym::Scope *scope) {
    dbprint("TypeContext: class type '", name, "' on scope ", scope);
    std::string mangled = mangle<ClassType>(name, scope->id);

    if (user_types.contains(mangled)) {
        dbprint("TypeContext: existing class found");
        return user_types.find(mangled)->second->as_class();
    }

    Box<ClassType> clsty = std::make_unique<ClassType>(decl_loc, name, scope, *this);

    // If no struct matching the name, make a new struct
    return insert_named_type<ClassType>(mangled, scope, std::move(clsty));
}

ClassType *TypeContext::get_class(Location decl_loc, sym::Scope *scope) {
    /*
    In EnlightenedC (and most C-family languages), two structs are considered
    equal iff they have the same name. (If two structs are named the same but
    have different definitions, that is a separate semantic violation.)

    An anonymous struct that has the exact same members as a named struct
    is not the same type as the named struct.
    */
    dbprint("TypeContext: anonymous class type on scope ", scope);
    auto name = "anon_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;
    auto mangled = mangle<ClassType>(name, scope->id);

    Box<ClassType> clsty = std::make_unique<ClassType>(decl_loc, scope, *this);

    return insert_named_type<ClassType>(mangled, scope, std::move(clsty));
}

UnionType *TypeContext::get_union(Location decl_loc, std::string& name, sym::Scope *scope) {
    dbprint("TypeContext: union type '", name, "' on scope ", scope);
    std::string mangled = mangle<UnionType>(name, scope->id);

    if (user_types.contains(mangled)) {
        dbprint("TypeContext: existing union found");
        return user_types.find(mangled)->second->as_union();
    }

    Box<UnionType> unnty = std::make_unique<UnionType>(decl_loc, name, scope, *this);

    // If no struct matching the name, make a new struct
    return insert_named_type<UnionType>(mangled, scope, std::move(unnty));
}

UnionType *TypeContext::get_union(Location decl_loc, sym::Scope *scope) {
    dbprint("TypeContext: anonymous union type on scope ", scope);
    auto name = "anon_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;
    auto mangled = mangle<UnionType>(name, scope->id);

    Box<UnionType> unnty = std::make_unique<UnionType>(decl_loc, scope, *this);

    return insert_named_type<UnionType>(mangled, scope, std::move(unnty));
}

EnumType *TypeContext::get_enum(Location decl_loc, std::string& name, sym::Scope *scope) {
    dbprint("TypeContext: enum type '", name, "' on scope ", scope);
    std::string mangled = mangle<EnumType>(name, scope->id);

    if (user_types.contains(mangled)) {
        dbprint("TypeContext: existing enum found");
        return user_types.find(mangled)->second->as_enum();
    }

    Box<EnumType> enmty = std::make_unique<EnumType>(decl_loc, name, scope, *this);

    // If no struct matching the name, make a new struct
    return insert_named_type<EnumType>(mangled, scope, std::move(enmty));
}

EnumType *TypeContext::get_enum(Location decl_loc, sym::Scope *scope) {
    dbprint("TypeContext: anonymous enum type on scope ", scope);
    auto name = "anon_" + std::to_string(anonymous_ctr);
    anonymous_ctr++;
    auto mangled = mangle<EnumType>(name, scope->id);

    Box<EnumType> enmty = std::make_unique<EnumType>(decl_loc, scope, *this);

    return insert_named_type<EnumType>(mangled, scope, std::move(enmty));
}

PointerType *TypeContext::get_pointer(Type *base, bool is_const) {
    dbprint("TypeContext: pointer with base ", base);
    // Check for base in our pointer store, if exists, return immediately
    auto it = pointers.find({base, is_const});
    if (it != pointers.end()) {
        return it->second.get();
    }

    // We don't need to check if base exists in our base type store,
    // since base is a Type *, we can assume it has already been created.

    // If not found, create a new pointer.
    Box<PointerType> ptr = std::make_unique<PointerType>(base, is_const, *this);

    ptr->is_const = is_const;
    auto *ret     = ptr.get();

    pointers[{base, is_const}] = std::move(ptr);

    return ret;
}

PointerType *TypeContext::decay_array(ArrayType *arr) {
    ArrayKey key = {arr->base, arr->arr_size};

    auto it = arrays.find(key);
    assert(it != arrays.end());

    // Create the pointer type
    PointerType *ret = get_pointer(arr->base, false);

    // If the array we want to decay is unsized, deallocate it
    if (!arr->arr_size) {
        deallocate_unsized_array(arr->base);
    }

    return ret;
}

ArrayType *TypeContext::get_array(Type *base, uint64_t size) {
    dbprint("TypeContext: array type with base ", base, ", size ", size);
    ArrayKey key(base, size);
    auto it = arrays.find(key);
    if (it != arrays.end()) {
        it->second->ref_count++;
        return it->second.get();
    }

    Box<ArrayType> arr = std::make_unique<ArrayType>(base, size, *this);
    auto *ret          = arr.get();
    arrays[key]        = std::move(arr);

    return ret;
}

ArrayType *TypeContext::get_array(Type *base) {
    dbprint("TypeContext: array type with base ", base, ", no size");
    ArrayKey key(base, {});
    auto it = arrays.find(key);
    if (it != arrays.end()) {
        dbprint("TypeContext: found existing array, returning");
        it->second->ref_count++;
        return it->second.get();
    }

    Box<ArrayType> arr = std::make_unique<ArrayType>(base, *this);
    auto *ret          = arr.get();
    arrays[key]        = std::move(arr);

    return ret;
}

ArrayType *TypeContext::set_array_size(Type *base, uint64_t size) {
    dbprint("TypeContext: set array of base ", base, ", with size ", size);

    // If array of specified size already exists, return that
    ArrayKey key1 = {base, size};

    auto it = arrays.find(key1);
    if (it != arrays.end()) {
        it->second->ref_count++;
        return it->second.get();
    }

    // Otherwise, create a new one.

    // First, deallocate the previous unsized base
    deallocate_unsized_array(base);

    return get_array(base, size);
}

FunctionType *TypeContext::get_function(Location loc, Type *returntype, Vec<Type *> params,
                                        bool variadic) {
    /*
    Function types do not need to be scope-aware, since their names are purely symbolic, and
    have no bearing on type equality. If a pointer to a function that was declared in a non-global
    scope is used where it is not declared, the SymbolTable resolves that automatically. We have to
    account for scope with named types like class, union, and enum because their name is the main
    key to type equality, and named types in a nested scope shadow any type with the same name in an
    outer scope.
    */

    dbprint("TypeContext: getting function type");

    if (returntype->is_array() || returntype->is_function()) {
        // fixme: use proper location
        throw InvalidReturnTypeError(loc);
    }

    Box<FunctionType> func = std::make_unique<FunctionType>(returntype, *this);
    FunctionType *ret      = func.get();

    FunctionType::FunctionSignature sig = {returntype, std::move(params), variadic};
    func->signature                     = std::move(sig);

    /*
    function type naming convention:

    "function_v_<sig hash>" if variadic,
    "function_<sig hash>" if not.
    */
    size_t hash = func->hash_sig();
    std::string name =
        variadic ? "function_v" + std::to_string(hash) : "function_" + std::to_string(hash);

    if (function_types.contains(name)) {
        dbprint("TypeContext: existing function type found");
        return function_types.find(name)->second.get();
    }

    function_types[name] = std::move(func);

    return ret;
}

void TypeContext::deallocate_unsized_array(Type *base) {
    ArrayKey key = {base, {}};

    auto it = arrays.find(key);
    if (it != arrays.end()) {
        // If exists, decrease its ref count
        dbprint("TypeContext: found array of base ", base, " to deallocate");
        it->second->ref_count--;

        // If ref count is zero, proceed to deallocate
        if (it->second->ref_count == 0) {

            // If base is also an array, check if it is unsized; if so, deallocate it too
            // This is basically recursive ref counting deallocation.
            if (base->is_array()) {
                ArrayType *arrbase = base->as_array();
                if (!arrbase->arr_size) {
                    deallocate_unsized_array(arrbase);
                }
            }

            dbprint("TypeContext: deleting array of base ", base);
            arrays.erase(key);
        }
    }
}
