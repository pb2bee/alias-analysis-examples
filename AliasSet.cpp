#include "llvm/IR/Constants.h"
#include "llvm/ADT/IntEqClasses.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/CommandLine.h"

#include "AliasSet.h"

using namespace llvm;

cl::opt<bool> Pairwise("pairwise",
        cl::desc("Print pairwise alignment information"));
cl::opt<bool> Print("print-as", cl::desc("Print alias sets"));

template<class T>
DisjointSets<T>::DisjointSets(const PointersList& l) :
        eq(l.size())
{
    // Insert pointers into NodeMap
    unsigned i = 0;

    for (typename PointersList::const_iterator lit = l.begin(), lend = l.end();
            lit != lend; ++lit) {
        const T V = *lit;
        valuemap[V] = i++;
    }
}

template<class T>
bool DisjointSets<T>::add(T V)
{
    if (valuemap.count(V))
        return false;

    unsigned n = valuemap.size();

    eq.grow(n + 1);
    valuemap[V] = n + 1;

    return true;
}

template<class T>
void DisjointSets<T>::unionfind(T V1, T V2)
{
    typename NodeMap::const_iterator mit1 = valuemap.find(V1);
    typename NodeMap::const_iterator mit2 = valuemap.find(V2);

    if (mit1 == valuemap.end() or mit2 == valuemap.end())
        return;

    unsigned id1 = mit1->second;
    unsigned id2 = mit2->second;

    eq.join(id1, id2);
}

template<class T>
void DisjointSets<T>::finish()
{
    eq.compress();

    unsigned num_classes = eq.getNumClasses();

    asl.resize(num_classes);

    for (typename NodeMap::const_iterator mit = valuemap.begin(), mend =
            valuemap.end(); mit != mend; ++mit) {

        T V = mit->first;
        unsigned id = mit->second;

        unsigned eq_class = eq[id];

        asl[eq_class].insert(V);
    }
}

bool AliasSet::runOnModule(Module& M)
{
    this->AA = &getAnalysis<AliasAnalysis>();

    // Add all pointers from M to the initial list of pointers
    SetVector<Value*> initial_list_pointers;

    // Insert all pointers into the initial list
    for (Module::iterator Mit = M.begin(), Mend = M.end(); Mit != Mend; ++Mit) {

        Function &F = *Mit;

        for (Function::arg_iterator ait = F.arg_begin(), aend = F.arg_end();
                ait != aend; ++ait) {
            Argument *A = &*ait;
            if (A->getType()->isPointerTy()) {
                initial_list_pointers.insert(A);
            }
        }

        for (Function::iterator Fit = F.begin(), Fend = F.end(); Fit != Fend;
                ++Fit) {
            BasicBlock &BB = *Fit;

            for (BasicBlock::iterator BBit = BB.begin(), BBend = BB.end();
                    BBit != BBend; ++BBit) {
                Instruction* I = &*BBit;

                if (I->getType()->isPointerTy()) {
                    initial_list_pointers.insert(I);
                }
            }
        }
    }

    // Now, create alias sets
    this->sets = new DisjointSets<Value*>(initial_list_pointers);

    for (Module::iterator Mit = M.begin(), Mend = M.end(); Mit != Mend; ++Mit) {

        Function &F = *Mit;

        // Interprocedural matching

        // Store all return values of this function
        SetVector<Value*> return_values;

        for (Function::iterator Fit = F.begin(), Fend = F.end(); Fit != Fend;
                ++Fit) {
            BasicBlock &BB = *Fit;
            ReturnInst *ret = dyn_cast<ReturnInst>(BB.getTerminator());

            if (ret) {
                Value *return_val = ret->getReturnValue();
                if (return_val) {
                    return_values.insert(return_val);
                }
            }
        }

        // Find all call sites of this function to perform matching
        // This follows the logic of IPConstantProp of LLVM
        for (Value::use_iterator UI = F.use_begin(), E = F.use_end(); UI != E;
                ++UI) {
	  User *U = UI->getUser();
            // Ignore blockaddress uses.
            if (isa<BlockAddress>(U))
                continue;

            // Used by a non-instruction, or not the callee of a function, do not
            // transform.
            if (!isa<CallInst>(U) && !isa<InvokeInst>(U))
                continue;

            CallSite CS(cast<Instruction>(U));
            if (!CS.isCallee(&*UI))
                continue;

            // Instruction *call = CS.getInstruction();

            // TODO: insert code here to match actual and formal parameters
            // of the function.

            // TODO: insert code here to match the formal return name with
            // its actual receiving location.
		}

        // Intraprocedural section
        SetVector<Value*> F_pointers;

        for (Function::arg_iterator ait = F.arg_begin(), aend = F.arg_end();
                ait != aend; ++ait) {
            Argument *A = &*ait;
            if (A->getType()->isPointerTy()) {
                F_pointers.insert(A);
            }
        }

        for (inst_iterator Iit = inst_begin(F), Eit = inst_end(F); Iit != Eit;
                ++Iit) {
            Instruction* I = &*Iit;

            if (I->getType()->isPointerTy()) {
                F_pointers.insert(I);
            }
        }

        size_t n = F_pointers.size();

        Value* p[2];
        for (unsigned i = 0; i < n; ++i) {
            p[0] = F_pointers[i];
            for (unsigned j = i + 1; j < n; ++j) {
                p[1] = F_pointers[j];
                switch (this->AA->alias(p[0], p[1])) {
                    case AliasAnalysis::MayAlias:
                    case AliasAnalysis::PartialAlias:
                    case AliasAnalysis::MustAlias:
                        // union-find
                        this->sets->unionfind(p[0], p[1]);
                        if (Pairwise) {
                            errs() << p[0]->getName() << " is alias of "
                                    << p[1]->getName() << "; "
                                    << this->AA->alias(p[0], p[1]) << "\n";
                        }
                        break;
                    case AliasAnalysis::NoAlias:
                        break;
                }
            }
        }
    }

    // Finish the DisjointSets structure
    sets->finish();
    // Print alias sets
    if (Print) {
        errs() << "\nNumber of alias sets: " << sets->size() << "\n";

        for (DisjointSets<Value*>::iterator sit = sets->begin(), send =
                sets->end(); sit != send; ++sit) {
            const SetVector<Value*>& set = *sit;

            SetVector<Value*>::iterator vit, vend;

            for (vit = set.begin(), vend = set.end(); vit != vend; ++vit) {
                const Value* V = *vit;
                const Argument *A = dyn_cast<Argument>(V);
                const Instruction *I = dyn_cast<Instruction>(V);
                if (I) {
                    errs() << I->getParent()->getParent()->getName() << ".";
                }
                else if (A) {
                    errs() << A->getParent()->getName() << ".";
                }
                errs() << V->getName();
                errs() << " ";
            }

            errs() << "\n\n";
        }
    }
    return false;
}

void AliasSet::getAnalysisUsage(AnalysisUsage& AU) const
{
    AU.addRequired<AliasAnalysis>();
    AU.setPreservesAll();
}

char AliasSet::ID = 0;
static RegisterPass<AliasSet> Y("alias-set", "Alias Set pass", false, false);

