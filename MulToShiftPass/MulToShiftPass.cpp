#include "llvm/IR/PassManager.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

bool isPowerOfTwo(const APInt &Val) {
    return Val.getBoolValue() && !(Val & (Val - 1));
}

struct MulToShiftPass : PassInfoMixin<MulToShiftPass> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
        bool Changed = false;

        for (auto &BB : F) {
            for (auto &I : make_early_inc_range(BB)) {
                auto *BinOp = dyn_cast<BinaryOperator>(&I);
                if (!BinOp || BinOp->getOpcode() != Instruction::Mul) continue;
                if (!BinOp->getType()->isIntegerTy()) continue;

                Value *LHS = BinOp->getOperand(0);
                Value *RHS = BinOp->getOperand(1);

                ConstantInt *ConstOp = nullptr;
                Value *OtherOp = nullptr;

                if (auto *CLHS = dyn_cast<ConstantInt>(LHS);
                    CLHS && isPowerOfTwo(CLHS->getValue())) {
                    ConstOp = CLHS;
                    OtherOp = RHS;
                } else if (auto *CRHS = dyn_cast<ConstantInt>(RHS);
                           CRHS && isPowerOfTwo(CRHS->getValue())) {
                    ConstOp = CRHS;
                    OtherOp = LHS;
                }

                if (!ConstOp) continue;

                unsigned ShiftAmt = ConstOp->getValue().exactLogBase2();
                IRBuilder<> Builder(BinOp);
                Value *Shl = Builder.CreateShl(OtherOp, ConstantInt::get(BinOp->getType(), ShiftAmt));

                BinOp->replaceAllUsesWith(Shl);
                BinOp->eraseFromParent();
                Changed = true;

                errs() << "[MulToShift] Replaced mul with shl in function '" 
                       << F.getName() << "'
";
            }
        }

        return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "MulToShiftPass",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "mul-to-shift") {
                        FPM.addPass(MulToShiftPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}
