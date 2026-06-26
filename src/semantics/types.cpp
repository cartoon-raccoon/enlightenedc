#include "semantics/types.hpp"

#include <algorithm>
#include <cfloat>
#include <llvm/IR/DerivedTypes.h>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>

#include "codegen/llvm.hpp"
#include "semantics/primitives.hpp"
#include "semantics/symbols.hpp"
#include "semantics/typeerr.hpp"
#include "tokens.hpp"
#include "util.hpp"

using namespace ecc::sema::types;
using namespace ecc::sema::prim;
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

size_t ConstType::alloc_size() {
    if (!finalized) {
        finalize();
    }

    return base->alloc_size();
}

bool ConstType::coercible_to(Type *dst) {
    if (!dst->is_const()) {
        return false;
    } else {
        return base->coercible_to(dst->unqual());
    }
}

void ConstType::finalize() {
    if (finalized) {
        assert(llvm_type && "ConstType marked finalized but llvm_type is null");
        dbprint("ConstType: already finalized, skipping");
        return;
    }

    dbprint("VoidType: finalizing");

    base->finalize();
    llvm_type = base->get_llvmtype();

    finalized = true;
}

Type *ConstType::effective_type() {
    return ctxt().get_const(base->effective_type());
}

TypeID ConstType::generate_id() const {
    VarHash<TypeID, TypeID> h;
    return h(base->id(), CONST_SALT);
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

bool PrimitiveType::coercible_to(Type *dst) {
    // Only primitive types or enums allowed
    PrimitiveType *new_dst;
    if (!dst->is_primitive()) {
        // check for enum
        if (dst->is_enum()) {
            new_dst = dst->as_enum()->underlying;
        } else {
            return false;
        }
    } else {
        new_dst = dst->as_primitive();
    }

    if (new_dst->is_bool()) {
        // we can coerce to bool implicitly.
        return is_integer() || is_float();
    }

    // If we are float and they are integer, return false
    if (this->is_float() && new_dst->is_integer()) {
        return false;
    }

    return true;
}

bool PrimitiveType::castable_to(Type *dst) {
    if (!dst->is_primitive() && !dst->is_pointer())
        return false;

    if (dst->is_pointer()) {
        // fixme: switch this over to system size
        return primkind == PrimType::U64;
    } else {
        // dst must be primitive here, all non-primitives were filtered out earlier
        assert(dst->is_primitive());
        return true;
    }
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
    switch (primkind) {
    case PrimType::F32:
        return FLT_MAX;
    case PrimType::F64:
        return DBL_MAX;
    default:
        return {};
    }
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

TypeID PrimitiveType::generate_id() const {
    VarHash<PrimTypeRank, bool> h;

    return h(pr_rank(primkind), pr_is_signed(primkind));
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
 * ACCESSOR METHODS
 */

void Accessor::apply_index_offset(size_t offset) {
    if (auto *idx = std::get_if<size_t>(&accessor)) {
        *idx += offset;
    }
}

std::string UserType::name() const {
    return std::visit(
        match{
            [&](const std::string& name) { return name; },
            [&](const uint64_t& anon_id) {
                std::stringstream ss;

                ss << ANON_USERTYPE_PREFIX << anon_id;

                return ss.str();
            }},
        identifier);
}

std::string UserType::mangled_name() const {
    std::stringstream ss;

    ss << base() << "_" << name() << "_" << scope->id;

    return ss.str();
}

TypeID UserType::generate_id() const {
    VarHash<std::string> h;
    return h(mangled_name());
}

/*
 * RECORD TYPE METHODS
 */

void RecordType::validate_new_member(Type *type, Optional<std::string> name, Location loc) {
    if (type == this) {
        // check for recursion without indirection.

        // member is guaranteed to have name, as anonymous types cannot be re-referenced
        throw RecursiveTypeError(*name, loc);
    }

    if (name) {
        // search recursively, not just immediate
        if (TypeMember *member = find(*name)) {
            throw MemberNameCollision(loc, *name, member->loc);
        }
    }

    if (type->is_usertype()) {
        // if member is usertype, check for completeness
        UserType *uty = type->as_usertype();
        if (!uty->is_complete()) {
            throw IncompleteTypeUseError(uty->name(), uty->decl_loc);
        }
        // if the new member is recordtype and directly contains us, that's mutual recursion,
        // also an error
        RecordType *rectype = type->as_recordtype();
        if (rectype && rectype->directly_contains(this)) {
            throw RecursiveTypeError(*name, loc);
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

RecordType::TypeMember *RecordType::add_member(std::string name, Type *type, Location loc) {
    dbprint("ClassType: adding member with name ", name, " type ", type->id());

    validate_new_member(type, name, loc);

    Box<TypeMember> member = std::make_unique<TypeMember>(name, type, loc, members.size());
    auto *ret              = member.get();
    members.push_back(std::move(member));

    return ret;
}

RecordType::TypeMember *RecordType::add_member(Type *type, Location loc) {
    dbprint("ClassType: adding anonymous member with type ", type->id());

    validate_new_member(type, {}, loc);

    Box<TypeMember> member = std::make_unique<TypeMember>(type, loc, members.size());
    auto *ret              = member.get();
    members.push_back(std::move(member));

    return ret;
}

AccessorPath RecordType::index(std::string& name) {
    AccessorPath path;

    if (!find(name))
        return path;

    // Try to find the name non-recursively
    TypeMember *maybe = find_imm(name);
    if (maybe) {
        // If found, push the index
        path.push_back(maybe->idx);
    } else {
        // If not found, check the anonymous members

        // For each anonymous member
        for (auto& mem : anon_members()) {
            // Cast the member as a record type, skipping if unable
            RecordType *mem_rec = mem->ty->as_recordtype();
            if (!mem_rec)
                continue;
            // If the type doesn't contain the member at all
            if (!mem_rec->find(name))
                continue;

            auto rec_path = mem_rec->index(name);
            path.extend(rec_path);
            break;
        }
    }

    return path;
}

RecordType::TypeMember *RecordType::find(std::string& name) {
    dbprint("RecordType: Looking for member with name ", name);

    for (auto& mem : members) {
        if (mem->name) {
            // if the member has a name, check if match
            // if match, return; else short circuit and continue
            if (*mem->name == name) {
                return mem.get();
            } else {
                continue;
            }
        } else if (mem->ty->is_recordtype()) {
            // if anonymous member is a class, recursively search in it
            // if we find something, return it
            auto *clsty = mem->ty->as_recordtype();
            assert(clsty);
            auto *maybe_mem = clsty->find(name);
            if (maybe_mem) {
                return maybe_mem;
            }
        }
    }

    return nullptr;
}

RecordType::TypeMember *RecordType::find_imm(std::string& name) {
    for (auto& mem : named_members()) {
        if (*mem->name == name) {
            return mem.get();
        }
    }

    return nullptr;
}

RecordType::TypeMember *RecordType::find(size_t idx) {
    dbprint("RecordType: Looking for member with index ", idx);
    if (idx >= num_members()) {
        return nullptr;
    }
    return members[idx].get();
}

RecordType::TypeMember *RecordType::find(Accessor& acc) {
    return std::visit(
        match{[&](MemberAcc& mem) { return find(mem); }, [&](IndexAcc idx) { return find(idx); }},
        acc.accessor);
}

RecordType::TypeMember *RecordType::find_by_path(AccessorPath& path) {
    TypeMember *curr    = nullptr;
    RecordType *curr_ty = this;

    for (auto& acc : path) {
        curr = curr_ty->find(acc);
        if (!curr)
            break;

        if (acc.next()) {
            if (!(curr->ty->is_recordtype())) {
                // if the current type member is not a record type, but we still have
                // accessors remaining, return null
                return nullptr;
            } else {
                // otherwise, set curr_ty to
                curr_ty = curr->ty->as_recordtype();
                assert(curr_ty);
            }
        }
    }

    return curr;
}

AccessorPath RecordType::indexify(AccessorPath& path) {
    AccessorPath idx_path;

    TypeMember *curr    = nullptr;
    RecordType *curr_ty = this;

    for (auto& acc : path) {
        curr = curr_ty->find(acc);
        if (!curr) {
            // if we fail to find an accessor in the path, return an empty path
            return {};
        }

        idx_path.push_back(curr->idx);

        if (acc.next()) {
            // if we have more accessors, we need to make sure the current type is a record type
            if (!curr->ty->is_recordtype()) {
                // if it's not a record type, we can't index into it, so return an empty path
                return {};
            } else {
                curr_ty = curr->ty->as_recordtype();
                assert(curr_ty);
            }
        }
    }

    return idx_path;
}

bool RecordType::directly_contains(Type *ty) const {
    for (const auto& mem : members) {
        if (mem->ty == ty)
            return true;
        if (mem->ty->is_recordtype()) {
            RecordType *rec = mem->ty->as_recordtype();
            if (rec->directly_contains(ty))
                return true;
        } else if (mem->ty->is_array()) {
            ArrayType *arr = mem->ty->as_array();
            if (arr->base == ty)
                return true;
        }
    }
    return false;
}

bool RecordType::is_fully_defined() {
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

/*
 * CLASS TYPE METHODS
 */

void ClassType::add_parent(ClassType *cls) {
    if (this->parent) {
        throw cls; // fixme: better error handling
    }

    this->parent = cls;

    if (!members.empty()) {
        for (auto& member : members) {
            member->idx += member_offset();
        }
    }
}

bool ClassType::is_parent_of(ClassType *cls) {
    if (!cls->parent)
        return false;

    return (*cls->parent) == this;
}

bool ClassType::is_ancestor_of(ClassType *cls) {
    if (!cls->parent)
        return false;

    if (is_parent_of(cls)) {
        // base case: if we are a direct parent, return true
        return true;
    } else {
        // recursive case: check if we are an ancestor of cls' parent
        // parent is guaranteed to have a value, as checked above.
        return is_ancestor_of(*cls->parent);
    }
}

size_t ClassType::member_offset() const {
    if (!parent) {
        return 0;
    } else {
        // recurse up the inheritance chain, accumulating member offsets
        return (*parent)->member_offset() + (*parent)->num_local_members();
    }
}

bool ClassType::is_fully_defined() {
    // If we have a parent, check that we and the parent are fully defined
    // Else, delegate to parent class impl
    if (parent) {
        return RecordType::is_fully_defined() && (*parent)->is_fully_defined();
    } else {
        return RecordType::is_fully_defined();
    }
}

RecordType::TypeMember *ClassType::add_member(std::string name, Type *type, Location loc) {
    auto *mem = RecordType::add_member(name, type, loc);

    if (parent) {
        mem->idx += member_offset();
    }

    return mem;
}

RecordType::TypeMember *ClassType::add_member(Type *type, Location loc) {
    auto *mem = RecordType::add_member(type, loc);

    if (parent) {
        mem->idx += member_offset();
    }

    return mem;
}

AccessorPath ClassType::index(std::string& name) {
    // Attempt to index self first
    AccessorPath path = RecordType::index(name);

    // If path is not empty or we have no parent, return it
    if (!path.empty() || !parent) {
        assert(path.is_all_indices());
        return path;
    }

    assert(parent.has_value());

    path = (*parent)->index(name);

    if (!path.empty()) {
        assert(path.is_all_indices());
    }

    return path;
}

RecordType::TypeMember *ClassType::find(std::string& name) {
    dbprint("ClassType: Looking for member with name ", name);

    TypeMember *mem = RecordType::find(name);

    if (mem || !parent) {
        return mem;
    }

    assert(parent.has_value());

    return (*parent)->find(name);
}

RecordType::TypeMember *ClassType::find_imm(std::string& name) {
    TypeMember *mem = RecordType::find_imm(name);

    if (mem || !parent) {
        return mem;
    }

    assert(parent.has_value());

    return (*parent)->find_imm(name);
}

RecordType::TypeMember *ClassType::find(size_t idx) {
    dbprint("ClassType: Looking for member with index ", idx);

    if (parent) {
        size_t offset = member_offset();
        if (idx < offset) {
            return (*parent)->find(idx);
        }
        return RecordType::find(idx - offset);
    }

    return RecordType::find(idx);
}

RecordType::TypeMember *ClassType::find_relative(size_t idx) {
    dbprint("ClassType: Looking for member with index ", idx);

    if (idx >= num_members())
        return nullptr;

    return members[idx].get();
}

RecordType::TypeMember *ClassType::find(Accessor& acc) {
    return std::visit(
        match{[&](MemberAcc& mem) { return find(mem); }, [&](IndexAcc idx) { return find(idx); }},
        acc.accessor);
}

RecordType::TypeMember *ClassType::find_by_path(AccessorPath& path) {
    return RecordType::find_by_path(path);

    // if (mem || !parent) {
    //     return mem;
    // }

    // assert(parent.has_value());

    // return (*parent)->find_by_path(path);
}

size_t ClassType::num_members() const {
    if (parent) {
        return (*parent)->num_members() + RecordType::num_members();
    } else {
        return RecordType::num_members();
    }
}

size_t ClassType::num_local_members() const {
    return RecordType::num_members();
}

bool ClassType::coercible_to(Type *dst) {
    if (!dst->is_class())
        return false;

    ClassType *target = dst->as_class();
    ClassType *curr   = this;

    while (curr->parent) {
        curr = *curr->parent;
        if (curr == target)
            return true;
    }
    return false;
}

Vec<ClassType const *> ClassType::ancestor_chain() const {
    Vec<ClassType const *> chain;
    for (ClassType const *c = this; c; c = c->parent ? *c->parent : nullptr)
        chain.push_back(c);

    return chain;
}

bool ClassType::castable_to(Type *dst) {
    if (!dst->is_class())
        return false;
    if (coercible_to(dst))
        return true; // upcast: dst is ancestor of this

    // downcast: this is ancestor of dst
    ClassType *target = dst->as_class();
    ClassType *curr   = target;

    while (curr->parent) {
        curr = *curr->parent;
        if (curr == this)
            return true;
    }
    return false;
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

    // recursively finalize up the chain first.
    if (parent) {
        (*parent)->finalize();
    }

    Vec<LLVMType *> args;

    if (parent) {
        // if we have a parent, all its parents have been finalized as well, so the parent's
        // llvmtype will contain the members of all its parents, and its own members. so, reading
        // the elements of the parent's llvmtype will read in all the members of parent classes, in
        // order, up the inheritance chain.
        llvm::StructType *parent_llvm = llvm::dyn_cast<llvm::StructType>((*parent)->get_llvmtype());

        assert(parent_llvm && "");

        for (auto *elem : parent_llvm->elements()) {
            args.push_back(elem);
        }
    }

    for (auto& member : members) {
        dbprint("ClassType: finalizing member declared at ", member->loc);
        member->ty->finalize();
        assert(member->ty->get_llvmtype());
        args.push_back(member->ty->get_llvmtype());
    }

    if (!is_anonymous()) {
        llvm_type = llvm::StructType::create(ctxt().llvm().ctx(), args, name());
    } else {
        llvm_type = llvm::StructType::create(ctxt().llvm().ctx(), args);
    }

    finalized = true;
}

std::string ClassType::formal() {
    return !is_anonymous() ? base() + " " + name() : to_string();
}

/*
 * UNION TYPE METHODS
 */

bool UnionType::coercible_to(Type *dst) {
    // If there is an underlying type representative, check that
    if (type_rep) {
        return (*type_rep)->coercible_to(dst);
    }

    // Otherwise, unions are incompatible with anything
    return false;
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

        size_t type_rep_size = (*type_rep)->alloc_size();

        // validate that no members are larger than type rep
        for (auto& member : members) {
            size_t mem_size = member->ty->alloc_size();
            if (mem_size > type_rep_size) {
                throw UnionTypeRepSizeOverflow(
                    def_loc, member->loc, member->ty, type_rep_size, mem_size);
            }
        }
    } else if (!members.empty()) {
        // get largest member for the size
        auto largest = std::max(members.begin(), members.end(), [](auto s1, auto s2) {
            return (*s1)->ty->alloc_size() < (*s2)->ty->alloc_size();
        });

        size_t size = (*largest)->ty->alloc_size();

        // set llvm_type as an array of unsigned bytes with size equal to largest member
        llvm_type = llvm::ArrayType::get(ctxt().u8->get_llvmtype(), size);
    } else {
        // set llvm_type as an empty array
        llvm_type = llvm::ArrayType::get(ctxt().u8->get_llvmtype(), 0);
    }

    finalized = true;
}

std::string UnionType::formal() {
    return !is_anonymous() ? base() + " " + name() : to_string();
}

/*
 * ENUM TYPE METHODS
 */

EnumType::EnumType(
    Location decl_loc, uint64_t anon_id, sema::sym::Scope *scope, TypeContext& tyctxt)
    : UserType(decl_loc, Kind::ENUM, anon_id, tyctxt, scope),
      // default type for an enum with no declared underlying type is I32.
      underlying(ctxt().get_i32()) {
}

EnumType::EnumType(
    Location decl_loc, std::string name, sema::sym::Scope *scope, TypeContext& tyctxt)
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
    if (idx >= enumerators.size())
        return nullptr;

    return enumerators[idx].get();
}

bool EnumType::coercible_to(Type *dst) {
    return underlying->coercible_to(dst);
}

void EnumType::finalize() {
    if (finalized) {
        assert(llvm_type && "EnumType marked finalized but llvm_type is null");
        dbprint("EnumType: already finalized, skipping");
        return;
    }

    if (!is_complete()) {
        throw TypeSemError("enum not fully defined", decl_loc);
    }

    if (!underlying->is_integral()) {
        throw InvalidEnumUnderlyingError(def_loc);
    }

    llvm_type = underlying->get_llvmtype();

    // check # of enumerators <= max value of underlying
    auto max_enum_ct = *underlying->int_max();
    if (max_enum_ct < num_enumerators()) {
        throw EnumeratorCountOverflow(def_loc, max_enum_ct, num_enumerators());
    }

    if (!enumerators.empty()) {
        auto max_val =
            std::max_element(enumerators.begin(), enumerators.end(), [&](auto& mem1, auto& mem2) {
                return mem1->value < mem2->value;
            });

        if ((*max_val)->value < 0 || static_cast<uint64_t>((*max_val)->value) > max_enum_ct) {
            throw EnumeratorValueOverflow(def_loc, max_enum_ct, (*max_val)->value);
        }
    }

    finalized = true;
}

std::string EnumType::formal() {
    return !is_anonymous() ? base() + " " + name() : to_string();
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

bool PointerType::coercible_to(Type *dst) {
    if (Type::coercible_to(dst))
        return true;

    PointerType *ptr = dst->as_pointer();
    if (!ptr)
        return false;

    int my_nesting = nesting_lvl();
    int ds_nesting = ptr->nesting_lvl();
    Type *dst_base = ptr->true_base();

    return my_nesting == 1 && ds_nesting == 1 ? base == dst_base || dst_base->is_void() : false;
}

void PointerType::finalize() {
    if (finalized) {
        assert(llvm_type && "PointerType was marked finalized but llvm_type was null");
        return;
    }

    // do not finalize base here: pointers to forward-declared (incomplete) types
    // are valid, and LLVM opaque pointers require no pointee type anyway.

    llvm_type = llvm::PointerType::get(ctxt().llvm().ctx(), 0);
    finalized = true;
}

Type *PointerType::decay() {
    return ctxt().get_size_type(false);
}

Type *PointerType::effective_type() {
    return ctxt().get_pointer(base->effective_type());
}

TypeID PointerType::generate_id() const {

    VarHash<TypeID, TypeID> h;
    return h(base->id(), POINTER_SALT);
}

/*
 * ARRAY TYPE METHODS
 */

bool ArrayType::is_fully_sized() {
    // fixme: does not work if there is a break in the type hierarchy of arrays
    return base->is_array() ? arr_size.has_value() && base->as_array()->is_fully_sized()
                            : arr_size.has_value();
}

bool ArrayType::coercible_to(Type *dst) {
    switch (dst->kind) {

    // if the other is an array, enforce strict equality
    case Kind::ARRAY: {
        return dst == this;
    }

    // if pointer, make sure bases match
    case Kind::POINTER: {
        PointerType *dst_ptr = dst->as_pointer();
        return base->coercible_to(dst_ptr);
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
        llvm_type = llvm::ArrayType::get(base->get_llvmtype(), *arr_size);
        assert(llvm_type);
    } else {
        //? would this be a problem?
        throw std::runtime_error("attempted to finalize unsized array");
    }

    finalized = true;
}

Type *ArrayType::decay() {
    return ctxt().get_pointer(base);
}

Type *ArrayType::effective_type() {
    return arr_size ? ctxt().get_array(base->effective_type(), *arr_size)
                    : ctxt().get_array(base->effective_type());
}

TypeID ArrayType::generate_id() const {
    if (arr_size) {
        VarHash<TypeID, uint64_t, TypeID> h;
        return h(base->id(), *arr_size, ARRAY_SALT);
    } else {
        VarHash<TypeID, TypeID> h;
        return h(base->id(), UARRAY_SALT);
    }
}

/*
 * FUNCTION TYPE METHODS
 */

size_t FunctionType::alloc_size() {
    throw std::runtime_error("cannot call size() on FunctionType");
}

std::size_t FunctionType::hash_sig() const {
    std::size_t hash = std::hash<TypeID>{}(signature.returntype->id());

    for (auto *param : signature.params) {
        hash ^= std::hash<TypeID>{}(param->id()) + BOOST_GOLDEN_RATIO + (hash << HASH_SHL) +
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

    Vec<LLVMType *> params_llvms;
    for (auto& param : signature.params) {
        params_llvms.push_back(param->get_llvmtype());
    }

    LLVMType *return_llvm = signature.returntype->get_llvmtype();

    llvm_type = llvm::FunctionType::get(return_llvm, params_llvms, signature.variadic);

    finalized = true;
}

Type *FunctionType::decay() {
    return ctxt().get_pointer(this);
}

TypeID FunctionType::generate_id() const {
    size_t sig_hash = hash_sig();

    VarHash<size_t, bool, TypeID> h;
    return h(sig_hash, signature.variadic, FUNCTION_SALT);
}

/*
 * TYPE BUiLDER METHODS
 */

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

Type *TypeBuilder::finalize(Optional<Ref<Vec<FuncParam>>> last_params) {
    if (!base) {
        throw std::runtime_error("TypeBuilder::finalize: cannot construct type from null base");
    }

    dbprint("TypeBuilder: finalizing type");

    Type *curr = base;
    while (!type_stack.empty()) {
        auto next_cstrctr = type_stack.top();
        std::visit(
            match{
                [&](Arr& arr) mutable {
                    // Wrap the base in an array.
                    if (arr.size) {
                        curr = this->ctxt().get_array(curr, *arr.size);
                    } else {
                        curr = this->ctxt().get_array(curr);
                    }
                },
                [&](Ptr& ptr) mutable {
                    // Wrap the base in a pointer.
                    curr = this->ctxt().get_pointer(curr);
                    if (ptr.is_const) {
                        curr = this->ctxt().get_const(curr);
                    }
                },
                [&](FnParams& fn) mutable {
                    Vec<Type *> params;
                    // map out the identifiers.
                    params.reserve(fn.params.size());
                    for (auto& param : fn.params) {
                        params.push_back(param.type->unqual());
                    }

                    if (last_params) {
                        (*last_params).get() = std::move(fn.params);
                    }

                    // Wrap the base as the return type in a function type.
                    curr = this->ctxt().get_function(fn.loc, curr, std::move(params), fn.variadic);
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

constexpr size_t BITS8  = 8;
constexpr size_t BITS16 = 16;
constexpr size_t BITS32 = 32;
constexpr size_t BITS64 = 64;

PrimitiveType *TypeContext::get_size_type(bool is_signed) {
    const llvm::DataLayout& dl = llvmref.get().mod().getDataLayout();
    auto size                  = dl.getPointerSizeInBits();

    switch (size) {
    case BITS8:
        return is_signed ? i8.get() : u8.get();
    case BITS16:
        return is_signed ? i16.get() : u16.get();
    case BITS32:
        return is_signed ? i32.get() : u32.get();
    case BITS64:
        return is_signed ? i64.get() : u64.get();
    default:
        return nullptr;
    }
}

Pair<PrimitiveType *, PrimitiveType *> TypeContext::promote(PrimType p1, PrimType p2) {
    PrimType promoted = pr_promote(p1, p2);

    return {get_primitive(promoted), get_primitive(promoted)};
}

Pair<PrimitiveType *, PrimitiveType *> TypeContext::promote(Pair<PrimType, PrimType> prims) {
    return promote(prims.first, prims.second);
}

PrimitiveType *TypeContext::single_promote(PrimType pr) {
    return get_primitive(pr_single_promote(pr));
}

ClassType *TypeContext::get_class(Location decl_loc, std::string& name, sym::Scope *scope) {
    dbprint("TypeContext: class type '", name, "' on scope ", scope->id);
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
    dbprint("TypeContext: anonymous class type on scope ", scope->id);
    auto name    = ANON_USERTYPE_PREFIX + std::to_string(*anonymous_ctr);
    auto mangled = mangle<ClassType>(name, scope->id);

    Box<ClassType> clsty = std::make_unique<ClassType>(decl_loc, *anonymous_ctr, scope, *this);
    anonymous_ctr++;

    return insert_named_type<ClassType>(mangled, scope, std::move(clsty));
}

UnionType *TypeContext::get_union(Location decl_loc, std::string& name, sym::Scope *scope) {
    dbprint("TypeContext: union type '", name, "' on scope ", scope->id);
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
    dbprint("TypeContext: anonymous union type on scope ", scope->id);
    auto name    = ANON_USERTYPE_PREFIX + std::to_string(*anonymous_ctr);
    auto mangled = mangle<UnionType>(name, scope->id);

    Box<UnionType> unnty = std::make_unique<UnionType>(decl_loc, *anonymous_ctr, scope, *this);
    anonymous_ctr++;

    return insert_named_type<UnionType>(mangled, scope, std::move(unnty));
}

EnumType *TypeContext::get_enum(Location decl_loc, std::string& name, sym::Scope *scope) {
    dbprint("TypeContext: enum type '", name, "' on scope ", scope->id);
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
    dbprint("TypeContext: anonymous enum type on scope ", scope->id);
    auto name    = ANON_USERTYPE_PREFIX + std::to_string(*anonymous_ctr);
    auto mangled = mangle<EnumType>(name, scope->id);

    Box<EnumType> enmty = std::make_unique<EnumType>(decl_loc, *anonymous_ctr, scope, *this);
    anonymous_ctr++;

    return insert_named_type<EnumType>(mangled, scope, std::move(enmty));
}

PointerType *TypeContext::get_pointer(Type *base) {
    dbprint("TypeContext: pointer with base ", base->id());
    // Check for base in our pointer store, if exists, return immediately
    auto it = pointers.find(base);
    if (it != pointers.end()) {
        return it->second.get();
    }

    // We don't need to check if base exists in our base type store,
    // since base is a Type *, we can assume it has already been created.

    // If not found, create a new pointer.
    Box<PointerType> ptr = std::make_unique<PointerType>(base, *this);
    auto *ret            = ptr.get();

    pointers[base] = std::move(ptr);

    return ret;
}

PointerType *TypeContext::decay_array(ArrayType *arr) {
    auto *ret = decay_array_ref(arr);

    // If the array we want to decay is unsized, deallocate it
    if (!arr->arr_size) {
        deallocate_unsized_array(arr->base);
    }

    return ret;
}

PointerType *TypeContext::decay_array_ref(ArrayType *arr) {
    ArrayKey key = {arr->base, arr->arr_size};

    auto it = arrays.find(key);
    assert(it != arrays.end());

    // Create and return the pointer type
    return get_pointer(arr->base);
}

ArrayType *TypeContext::get_array(Type *base, uint64_t size) {
    dbprint("TypeContext: array type with base ", base->id(), ", size ", size);
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
    dbprint("TypeContext: array type with base ", base->id(), ", no size");
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

FunctionType *
TypeContext::get_function(Location loc, Type *returntype, Vec<Type *> params, bool variadic) {
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

ConstType *TypeContext::get_const(Type *base) {
    // Guard to prevent double-wrapping of const
    if (base->is_const()) {
        ConstType *cnst = base->as_const();
        assert(cnst && "type base returned is_const() = true but cannot be cast");
        assert(const_types.contains(cnst->base));
        return cnst;
    }

    if (const_types.contains(base)) {
        return const_types.find(base)->second.get();
    }

    auto to_insert = std::make_unique<ConstType>(base, *this);

    ConstType *ret = to_insert.get();

    const_types.insert_or_assign(base, std::move(to_insert));

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
