#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"


using namespace llvm;

namespace {
    struct Popcorn : PassInfoMixin<Popcorn> {
        // Main entry point, takes IR unit to run the pass on (&F) and the
        // corresponding pass manager (to be queried if need be)
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
            for (auto &Func : M) {
                if (Func.getName() != "main")
                    continue;

                for (auto &BB : Func) {
                    for (auto &Ins : BB) {
                        auto ICS = ImmutableCallSite(&Ins);
                        if (nullptr == ICS.getInstruction())
                            continue;

                        auto DirectInv = dyn_cast<Function>(ICS.getCalledValue()->stripPointerCasts());
                        if (nullptr == DirectInv)
                            continue;

                        errs() << DirectInv->getName() << "\n";
                        errs() << DirectInv->getFunction() << "\n";

                    }
                }
            }
            return PreservedAnalyses::all();
        }
    };

} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getHelloWorldPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "Popcorn", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, ModulePassManager &MPM,
                           ArrayRef<PassBuilder::PipelineElement>) {
                            if (Name == "popcorn") {
                                MPM.addPass(Popcorn());
                                return true;
                            }
                            return false;
                        });
            }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize Popcorn when added to the pass pipeline on the
// command line, i.e. via '-passes=hello-world'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getHelloWorldPluginInfo();
}