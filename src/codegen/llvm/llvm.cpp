#include "codegen/llvm/llvm.hpp"

#include "error.hpp"
#include "lowering/cfg/cfg.hpp"
#include "semantics/typeerr.hpp"
#include "util.hpp"

using namespace ecc::codegen;
using namespace ecc::sema::types;
using namespace ecc::sema::prim;
using namespace ecc::sema;

constexpr size_t BYTE_SIZE = 8;

LLVMCore::LLVMCore() {
    dbprint("Initializing LLVM");
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllDisassemblers();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    auto triple_str = llvm::sys::getDefaultTargetTriple();
    target_triple   = llvm::Triple(triple_str);
    std::string error;
    target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    if (!target) {
        throw EccError(ErrorSource::LLVM, error);
    }

    auto cpu             = llvm::sys::getHostCPUName();
    const auto *features = "";
    llvm::TargetOptions opt;

    target_machine = target->createTargetMachine(
        llvm::Triple(target_triple), cpu, features, opt, llvm::Reloc::PIC_);

    dbprint("LLVM initialized");
}

LLVMCore::~LLVMCore() {
    dbprint("LLVM: Shutting down");
    llvm::llvm_shutdown();
}

Box<CodeGenUnit> LLVMCore::make_unit(const std::string& unit_name) {
    return std::make_unique<LLVMUnit>(unit_name, *this);
}

LLVMUnit::LLVMUnit(const std::string& module_name, LLVMCore& llvmcore) {
    dbprint("LLVM: Creating LLVMUnit with module name '", module_name, "'");
    context   = std::make_unique<llvm::LLVMContext>();
    llvmmod   = std::make_unique<llvm::Module>(module_name, *context);
    irbuilder = std::make_unique<llvm::IRBuilder<>>(*context);

    llvmmod->setTargetTriple(llvmcore.target_triple);

    llvmmod->setDataLayout(llvmcore.target_machine->createDataLayout());

    dbprint("LLVM: LLVMUnit created");
}

bool LLVMUnit::is_finalized(Type *type) {
    return typemap.contains(type);
}

LLVMType *LLVMUnit::get_llvm_type(Type *type) {
    if (!is_finalized(type)) {
        finalize(type);
    }

    return typemap[type];
}

void LLVMUnit::finalize(VoidType *type) {
    if (is_finalized(type)) {
        dbprint("VoidType: already finalized, skipping");
        return;
    }

    dbprint("VoidType: finalizing");
    typemap[type] = llvm::Type::getVoidTy(ctx());
}

void LLVMUnit::finalize(PrimitiveType *type) {
    if (is_finalized(type)) {
        dbprint("PrimitiveType: ", type->to_string(), " already finalized, skipping");
        return;
    }
    dbprint("PrimitiveType: finalizing ", type->to_string());

    switch (type->get_primkind()) {
    case PrimType::U8:
    case PrimType::I8:
    case PrimType::BOOL: //? should bool be 1 bit?
        typemap[type] = llvm::Type::getInt8Ty(ctx());
        break;

    case PrimType::U16:
    case PrimType::I16:
        typemap[type] = llvm::Type::getInt16Ty(ctx());
        break;

    case PrimType::U32:
    case PrimType::I32:
        typemap[type] = llvm::Type::getInt32Ty(ctx());
        break;

    case PrimType::U64:
    case PrimType::I64:
        typemap[type] = llvm::Type::getInt64Ty(ctx());
        break;

    case PrimType::F32:
        typemap[type] = llvm::Type::getFloatTy(ctx());
        break;

    case PrimType::F64:
        typemap[type] = llvm::Type::getDoubleTy(ctx());
        break;
    }
}

void LLVMUnit::finalize(ClassType *type) {
    if (is_finalized(type)) {
        dbprint("ClassType: already finalized, skipping");
        return;
    }
    dbprint("ClassType: finalizing class defined at ", type->def_loc);

    if (!type->is_complete()) {
        throw TypeSemError("class not fully defined", type->decl_loc);
    }

    // recursively finalize up the chain first.
    if (type->get_parent()) {
        finalize(*type->get_parent());
    }

    Vec<LLVMType *> args;

    if (type->get_parent()) {
        // if we have a parent, all its parents have been finalized as well, so the parent's
        // llvmtype will contain the members of all its parents, and its own members. so, reading
        // the elements of the parent's llvmtype will read in all the members of parent classes, in
        // order, up the inheritance chain.
        llvm::StructType *parent_llvm =
            llvm::dyn_cast<llvm::StructType>(get_llvm_type(*type->get_parent()));

        assert(parent_llvm && "");

        for (auto *elem : parent_llvm->elements()) {
            args.push_back(elem);
        }
    }

    for (auto& member : type->get_members()) {
        dbprint("ClassType: finalizing member declared at ", member->loc);
        finalize(member->ty);
        args.push_back(get_llvm_type(member->ty));
    }

    if (!type->is_anonymous()) {
        typemap[type] = llvm::StructType::create(ctx(), args, type->name());
    } else {
        typemap[type] = llvm::StructType::get(ctx(), args);
    }
}

void LLVMUnit::finalize(UnionType *type) {
    /*
    If the union has a type representative, that becomes the final type of the union.
    Otherwise, the LLVM type of the union becomes that of the largest member.
    */
    if (is_finalized(type)) {
        dbprint("UnionType: already finalized, skipping");
        return;
    }
    dbprint("UnionType: finalizing union defined at ", type->def_loc);

    if (!type->is_complete()) {
        throw TypeSemError("union not fully defined", type->decl_loc);
    }

    if (type->get_type_rep()) {
        finalize(*type->get_type_rep());
    }
    // Finalize all members first
    for (auto& member : type->get_members()) {
        dbprint("UnionType: finalizing member declared at ", member->loc);
        finalize(member->ty);
        assert(typemap[member->ty]);
    }

    if (type->get_type_rep()) {
        // we finalized the type rep earlier, it is guaranteed to be found
        LLVMType *llvm_type = get_llvm_type(*type->get_type_rep());

        typemap[type] = llvm_type;
    } else if (type->num_members() > 0) {
        // get largest member for the size
        auto largest = std::max_element(
            type->get_members().begin(), type->get_members().end(),
            [](auto& s1, auto& s2) { return s1->ty->alloc_size() < s2->ty->alloc_size(); });

        // size of the largest member in bytes
        size_t size = (*largest)->ty->alloc_size();

        const llvm::DataLayout& dl = mod().getDataLayout();
        llvm::Align align(1);

        // find the strictest alignment
        for (auto& member : type->get_members()) {
            llvm::Align mem_align = dl.getABITypeAlign(typemap[member->ty]);
            if (mem_align > align) {
                align = mem_align;
            }
        }

        unsigned elem_bits = align.value() * BYTE_SIZE;
        LLVMType *elem_ty  = llvm::IntegerType::get(ctx(), elem_bits);

        size_t num_elements = (size + align.value() - 1) / align.value();

        // set llvm_type as an array of integers sized to the strictest alignment,
        // array size is smallest number of integers needed to hold the largest member
        typemap[type] = llvm::ArrayType::get(elem_ty, num_elements);
    } else {
        // set llvm_type as an empty array of bytes
        typemap[type] = llvm::ArrayType::get(llvm::IntegerType::getInt8Ty(ctx()), 0);
    }
}

void LLVMUnit::finalize(EnumType *type) {
    if (is_finalized(type)) {
        dbprint("EnumType: already finalized, skipping");
        return;
    }

    if (!type->is_complete()) {
        throw TypeSemError("enum not fully defined", type->decl_loc);
    }

    typemap[type] = get_llvm_type(type->get_underlying());
}

void LLVMUnit::finalize(PointerType *type) {
    if (is_finalized(type)) {
        return;
    }

    // do not finalize base here: pointers to forward-declared (incomplete) types
    // are valid, and LLVM opaque pointers require no pointee type anyway.

    typemap[type] = llvm::PointerType::get(ctx(), 0);
}

void LLVMUnit::finalize(ArrayType *type) {
    if (is_finalized(type)) {
        dbprint("ArrayType: already finalized, skipping");
        return;
    }

    if (type->get_arr_size()) {
        typemap[type] =
            llvm::ArrayType::get(get_llvm_type(type->get_base()), *type->get_arr_size());
    } else {
        //? would this be a problem?
        throw std::runtime_error("attempted to finalize unsized array");
    }
}

void LLVMUnit::finalize(ConstType *type) {
    if (is_finalized(type)) {
        dbprint("ConstType: already finalized, skipping");
        return;
    }

    finalize(type->get_base());
    typemap[type] = typemap[type->get_base()];
}

void LLVMUnit::finalize(FunctionType *type) {
    if (is_finalized(type)) {
        dbprint("FunctionType: already finalized, skipping");
        return;
    }

    Vec<LLVMType *> params_llvms;
    for (const auto& param : type->get_signature().params) {
        params_llvms.push_back(get_llvm_type(param));
    }

    LLVMType *return_llvm = get_llvm_type(type->get_signature().returntype);

    typemap[type] =
        llvm::FunctionType::get(return_llvm, params_llvms, type->get_signature().variadic);
}

size_t LLVMUnit::get_pointer_size() {
    const llvm::DataLayout& dl = mod().getDataLayout();

    return dl.getPointerSize();
}

size_t LLVMUnit::get_pointer_size_bits() {
    const llvm::DataLayout& dl = mod().getDataLayout();

    return dl.getPointerSizeInBits();
}

size_t LLVMUnit::alloc_size(Type *type) {
    if (!is_finalized(type)) {
        finalize(type);
    }

    Type *type_key      = type->is_const() ? type->as_const()->get_base() : type;
    LLVMType *size_type = get_llvm_type(type_key);

    assert(size_type);

    const llvm::DataLayout& dl = mod().getDataLayout();
    return dl.getTypeAllocSize(size_type);
}

void LLVMUnit::compile(lower::cfg::ProgramCFG& prog) {
}