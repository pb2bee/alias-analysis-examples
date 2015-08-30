//===------------ AliasSet.h - How to use the LoopInfo analysis -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass finds all the aliasing pairs of pointers in a program.
//
// Author: Victor Hugo Sperle Campos.
// Date: January 12th, 2015.
// Usage: opt -load Obfuscator.dylib -obfZero input.bc -o output.bc
//
//===----------------------------------------------------------------------===//

#ifndef ALIASSET_H_
#define ALIASSET_H_

#define DEBUG_TYPE "AliasSet"

using namespace llvm;

namespace {

  template<class T>
    struct DisjointSets {

      typedef DenseMap<T, unsigned> NodeMap;

      // Alias set
      typedef SetVector<T> PointersList;

      // Alias set list
      typedef SmallVector<PointersList, 4> AliasSetList;

      // Maps values to their identifier in the IntEqClasses structure
      NodeMap valuemap;
      // DisjointSets of unsigneds, provided by LLVM
      IntEqClasses eq;

      AliasSetList asl;

      // Constructs structure based on a list of pointers input
      DisjointSets(const PointersList& l);

      // Add a new value to the disjoint sets structure
      bool add(T V);

      // Gets two values and union-find them
      // If either of them is not known, raise assertion failure
      void unionfind(T V1, T V2);

      // Prepare alias sets to be iterated. Only call this function after ALL
      // union-finding has been done
      void finish();

      size_t size()
      {
        // assert(asl.size() > 0);

        return asl.size();
      }

      typedef typename AliasSetList::iterator iterator;

      iterator begin()
      {
        assert(asl.size() > 0);

        return this->asl.begin();
      }
      iterator end()
      {
        return this->asl.end();
      }

    };

  struct AliasSet: public ModulePass {
    static char ID;

    AliasSet() :
      ModulePass(ID)
    {
      this->AA = 0;
      this->sets = 0;
    }

    ~AliasSet()
    {
      delete this->sets;
    }

    virtual bool runOnModule(Module& M);

    virtual void getAnalysisUsage(AnalysisUsage& AU) const;

    AliasAnalysis* AA;

    DisjointSets<Value*>* sets;

    // For a given element V, return the set ID which contains V
    unsigned getSetIDForPointer(Value *V) const
    {
      assert(this->sets->asl.size() > 0);

      DisjointSets<Value*>::NodeMap::iterator mit = sets->valuemap.find(V);

      assert(mit != sets->valuemap.end());

      return sets->eq[mit->second];
    }

    // Return an alias set given the input set ID.
    const DisjointSets<Value*>::PointersList& getAliasSet(unsigned setid) const
    {
      return this->sets->asl[setid];
    }

    // Alias Sets iterators
    typedef DisjointSets<Value*>::iterator iterator;

    iterator begin()
    {
      return sets->begin();
    }

    iterator end()
    {
      return sets->end();
    }
  };
}

#endif
