//=============================================================================
// DESCRIPTION:
//    For every basic block in the input function, this liveness creates a list of any
//    "first-class" values reachable from that block. It uses the results of the
//    DominatorTree liveness.
//
// ALGORITHM:
//    -------------------------------------------------------------------------
//    v_N = set of values defined in basic block N (BB_N)
//    RIV_N = set of reachable values for basic block N (BB_N)
//    -------------------------------------------------------------------------
//    STEP 1:
//    For every BB_N in F:
//      compute v_N and store it in DefinedValuesMap
//    -------------------------------------------------------------------------
//    STEP 2:
//    Compute the RIVs for the entry block (BB_0):
//      RIV_0 = {input args, global vars}
//    -------------------------------------------------------------------------
//    STEP 3: Traverse the CFG and for every BB_M that BB_N dominates,
//    calculate RIV_M as follows:
//      RIV_M = {RIV_N, v_N}
//=============================================================================


#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/ValueMap.h"


using namespace llvm;

namespace {
    using Result = llvm::MapVector<llvm::BasicBlock const *, llvm::SmallPtrSet<llvm::Value *, 8>>;

    void printRIVResult(raw_ostream &OutS, const Result &resultMap) {
        OutS << "=================================================\n";
        OutS << "Reachable Value analysis results\n";
        OutS << "=================================================\n";

        const char *Str1 = "BB id";
        const char *Str2 = "Reachable Values";
        const char *EmptyStr = "";

        for (auto const &KV : resultMap) {
            std::string DummyStr;
            raw_string_ostream BBIdStream(DummyStr);
            KV.first->printAsOperand(BBIdStream, false);
            OutS << format("[[BasicBlock %s]]\n", BBIdStream.str().c_str());
            for (auto const &Value : KV.second) {
                std::string DummyStr;
                raw_string_ostream InstrStr(DummyStr);

                Value->print(InstrStr);

                OutS << format("==>%s\n", InstrStr.str().c_str());
            }
            OutS << "-------------------------------------------------\n";
        }
        OutS << "\n\n";
    }


// DominatorTree node types used in RIV. One could use auto instead, but IMO
// being verbose makes it easier to follow.
    using NodeTy = DomTreeNodeBase<llvm::BasicBlock> *;
// A map that a basic block BB holds a set of pointers to values defined in BB.
    using DefValMapTy = Result;

//-----------------------------------------------------------------------------
// RIV Implementation
//-----------------------------------------------------------------------------
    Result buildRIV(Function &F, NodeTy CFGRoot) {
        Result ResultMap;

        // Initialise a double-ended queue that will be used to traverse all BBs in F
        std::deque<NodeTy> BBsToProcess;
        BBsToProcess.push_back(CFGRoot);

        // STEP 1: For every basic block BB compute the set of values defined
        // in BB
        DefValMapTy DefinedValuesMap;
        for (BasicBlock &BB : F) {
            auto &Values = DefinedValuesMap[&BB];
            for (Instruction &Inst : BB)
                if (Inst.getType()->isFirstClassType())
                    Values.insert(&Inst);
        }

        // STEP 2: Compute the RIVs for the entry BB. This will include global
        // variables and input arguments.
        auto &EntryBBValues = ResultMap[&F.getEntryBlock()];

        for (auto &Global : F.getParent()->getGlobalList())
            if (Global.getValueType()->isFirstClassType())
                EntryBBValues.insert(&Global);

        for (Argument &Arg : F.args())
            if (Arg.getType()->isFirstClassType())
                EntryBBValues.insert(&Arg);

        // STEP 3: Traverse the CFG for every BB in F calculate its RIVs
        while (!BBsToProcess.empty()) {
            auto *Parent = BBsToProcess.back();
            BBsToProcess.pop_back();

            // Get the values defined in Parent
            auto &ParentDefs = DefinedValuesMap[Parent->getBlock()];
            // Get the RIV set of for Parent
            // (Since RIVMap is updated on every iteration, its contents are likely to
            // be moved around when resizing. This means that we need a copy of it
            // (i.e. a reference is not sufficient).
            llvm::SmallPtrSet<llvm::Value *, 8> ParentRIVs =
                    ResultMap[Parent->getBlock()];

            // Loop over all BBs that Parent dominates and update their RIV sets
            for (NodeTy Child : *Parent) {
                BBsToProcess.push_back(Child);
                auto ChildBB = Child->getBlock();

                // Add values defined in Parent to the current child's set of RIV
                ResultMap[ChildBB].insert(ParentDefs.begin(), ParentDefs.end());

                // Add Parent's set of RIVs to the current child's RIV
                ResultMap[ChildBB].insert(ParentRIVs.begin(), ParentRIVs.end());
            }
        }

        return ResultMap;
    }


    struct Liveness : PassInfoMixin<Liveness> {
        // Main entry point, takes IR unit to run the liveness on (&F) and the
        // corresponding liveness manager (to be queried if need be)
        PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM) {
            DominatorTree *DT = &FAM.getResult<DominatorTreeAnalysis>(F);
            Result Res = buildRIV(F, DT->getRootNode());

            printRIVResult(errs(), Res);

            return PreservedAnalyses::all();
        }
    };

} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getLivenessPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "Liveness", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                        [](StringRef Name, FunctionPassManager &FPM,
                           ArrayRef<PassBuilder::PipelineElement>) {
                            if (Name == "liveness") {
                                FPM.addPass(Liveness());
                                return true;
                            }
                            return false;
                        });
            }};
}

// This is the core interface for liveness plugins. It guarantees that 'opt' will
// be able to recognize Liveness when added to the liveness pipeline on the
// command line, i.e. via '-passes=liveness'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getLivenessPluginInfo();
}