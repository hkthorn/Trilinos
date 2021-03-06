// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef MUELU_LOCALORDINALTRANSFER_FACTORY_DEF_HPP
#define MUELU_LOCALORDINALTRANSFER_FACTORY_DEF_HPP

#include "Xpetra_ImportFactory.hpp"
#include "Xpetra_VectorFactory.hpp"
#include "Xpetra_MapFactory.hpp"
#include "Xpetra_IO.hpp"

#include "MueLu_CoarseMapFactory.hpp"
#include "MueLu_Aggregates.hpp"
#include "MueLu_LocalOrdinalTransferFactory_decl.hpp"

#include "MueLu_Level.hpp"
#include "MueLu_Monitor.hpp"

namespace MueLu {

  template <class LocalOrdinal, class GlobalOrdinal, class Node>
  RCP<const ParameterList> LocalOrdinalTransferFactory<LocalOrdinal, GlobalOrdinal, Node>::GetValidParameterList() const {
    RCP<ParameterList> validParamList = rcp(new ParameterList());

    validParamList->set<RCP<const FactoryBase> >(TransferVecName_,              Teuchos::null, "Factory for TransferVec generation");
    validParamList->set<RCP<const FactoryBase> >("Aggregates",                   Teuchos::null, "Factory for aggregates generation");
    validParamList->set<RCP<const FactoryBase> >("CoarseMap",                    Teuchos::null, "Generating factory of the coarse map");

    return validParamList;
  }

  template <class LocalOrdinal, class GlobalOrdinal, class Node>
  void LocalOrdinalTransferFactory<LocalOrdinal, GlobalOrdinal, Node>::DeclareInput(Level& fineLevel, Level& coarseLevel) const {
    static bool isAvailableXfer = false;
    if (coarseLevel.GetRequestMode() == Level::REQUEST) {
      isAvailableXfer = coarseLevel.IsAvailable(TransferVecName_, this);
      if (isAvailableXfer == false) {
        Input(fineLevel, TransferVecName_);
        Input(fineLevel, "Aggregates");
        Input(fineLevel, "CoarseMap");
      }
    }

  }

  template <class LocalOrdinal, class GlobalOrdinal, class Node>
  void LocalOrdinalTransferFactory<LocalOrdinal, GlobalOrdinal, Node>::Build(Level & fineLevel, Level &coarseLevel) const {
    FactoryMonitor m(*this, "Build", coarseLevel);

    GetOStream(Runtime0) << "Transferring " <<TransferVecName_ << std::endl;
    RCP<LocalOrdinalVector> coarseTV;
    RCP<LocalOrdinalVector> fineTV;
    LO LO_INVALID = Teuchos::OrdinalTraits<LO>::invalid();

    if (coarseLevel.IsAvailable(TransferVecName_, this)) {
      GetOStream(Runtime0) << "Reusing "<<TransferVecName_ << std::endl;
      return;
    }
    
    RCP<Aggregates>     aggregates = Get< RCP<Aggregates> > (fineLevel, "Aggregates");
    fineTV                         = Get< RCP<LocalOrdinalVector> >(fineLevel, TransferVecName_);
    RCP<const Map>      coarseMap  = Get< RCP<const Map> >  (fineLevel, "CoarseMap");
    RCP<const Map>      uniqueMap  = fineTV->getMap();

    ArrayView<const GO> elementAList = coarseMap->getNodeElementList();
    
    coarseTV   = LocalOrdinalVectorFactory::Build(coarseMap,1);
    
    // Create overlapped fine TV to reduce global communication
    RCP<LocalOrdinalVector> ghostedTV = fineTV;
    if (aggregates->AggregatesCrossProcessors()) {

      RCP<const Map>    nonUniqueMap = aggregates->GetMap();
      RCP<const Import> importer     = ImportFactory::Build(uniqueMap, nonUniqueMap);
      
      ghostedTV = LocalOrdinalVectorFactory::Build(nonUniqueMap, 1);
      ghostedTV->doImport(*fineTV, *importer, Xpetra::INSERT);
    }
    
    // Get some info about aggregates
    int                         myPID        = uniqueMap->getComm()->getRank();
    ArrayRCP<LO>                aggSizes     = aggregates->ComputeAggregateSizes();
    const ArrayRCP<const LO>    vertex2AggID = aggregates->GetVertex2AggId()->getData(0);
    const ArrayRCP<const LO>    procWinner   = aggregates->GetProcWinner()->getData(0);
    

    ArrayRCP<const LO> fineData = ghostedTV->getData(0);
    ArrayRCP<LO>     coarseData = coarseTV->getDataNonConst(0);
    
    // Invalidate everything first, to check for errors
    for(LO i=0; i<coarseData.size(); i++)
      coarseData[i] = LO_INVALID;
   
    // Fill in coarse TV
    size_t error_count = 0;
    for (LO lnode = 0; lnode < vertex2AggID.size(); lnode++) {      
      if (procWinner[lnode] == myPID &&
          //lnode < vertex2AggID.size() &&
          lnode < fineData.size() && // TAW do not access off-processor data
          vertex2AggID[lnode] < coarseData.size()) {
        if(coarseData[vertex2AggID[lnode]] == LO_INVALID)
          coarseData[vertex2AggID[lnode]] = fineData[lnode];
        if(coarseData[vertex2AggID[lnode]] != fineData[lnode])
          error_count++;        
      }
    }

    // Error checking:  All nodes in an aggregate must share a local ordinal
    if(error_count > 0) {
      std::ostringstream ofs;
      ofs << "LocalOrdinalTransferFactory: ERROR:  Each aggregate must have a unique LO value.  We had "<<std::to_string(error_count)<<" unknowns that did not match.";
      throw std::runtime_error(ofs.str());
    }
      
    Set<RCP<LocalOrdinalVector> >(coarseLevel, TransferVecName_, coarseTV);

  }

} // namespace MueLu

#endif // MUELU_LOCALORDINALTRANSFER_FACTORY_DEF_HPP
