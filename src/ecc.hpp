#include <memory>
#ifndef ECC_H

#include <vector>
#include <string>

namespace ecc {

class EccConfig {
public:
    EccConfig(int argc, char *argv[]);
    
    // The list of input files.
    std::vector<std::string> input_files;

    std::string output_file;

    // Whether to enable verbose messages.
    bool verbose = false;

    /*
    The phase of compilation at which to stop.
    */
    enum StopAt {
        // Only preprocess the input files, dumping them to their own files.
        PREPROCESS,
        // Only parse the input files. (warn if no output method is detected.)
        PARSE,
        // Parse and validate the input files, and then stop, emitting any errors.
        VALIDATE,
        // Compile the files, emitting LLVM IR bytecode by default. Other output formats
        // (e.g. assembly text, LLVM IR text) can be specified.
        COMPILE,
        // Assemble the files into a single object file.
        ASSEMBLE,
        // Fully complete the process, linking into any standard libraries needed.
        COMPLETE,
    } stop_at = COMPLETE;
};

/*
The EnlightenedC Compiler main driver class.
*/
class Ecc {
public:
    Ecc(int argc, char *argv[]) : config(std::make_unique<EccConfig>(argc, argv)) {}

    std::unique_ptr<EccConfig> config;

    /*
    Run the compilation pipeline on a single input file.

    Each file is one translation unit in Ecc. That is, one AST is produced for each file,
    and thus one LLVM Module per file.
    */
    void run_pipeline(std::string *filename);

    void run();
};


}

#endif