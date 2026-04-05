#ifndef ECC_BACKEND_H
#define ECC_BACKEND_H
namespace ecc::driver {

class TranslationUnit;

/**
A simple class that drives the backend of the compilation pipeline.
*/
class Backend {
public:
    Backend() {}

    void run(TranslationUnit& unit);
};

} // namespace ecc::driver

#endif