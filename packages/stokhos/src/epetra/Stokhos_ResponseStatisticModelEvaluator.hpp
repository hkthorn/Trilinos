// @HEADER
// *****************************************************************************
//                           Stokhos Package
//
// Copyright 2009 NTESS and the Stokhos contributors.
// SPDX-License-Identifier: BSD-3-Clause
// *****************************************************************************
// @HEADER

#ifndef STOKHOS_RESPONSE_STATISTIC_MODEL_EVALUATOR_HPP
#define STOKHOS_RESPONSE_STATISTIC_MODEL_EVALUATOR_HPP

#include "EpetraExt_ModelEvaluator.h"
#include "Teuchos_RCP.hpp"
#include "Teuchos_Array.hpp"
#include "Epetra_Map.h"
#include "Stokhos_OrthogPolyBasis.hpp"
#include "EpetraExt_MultiComm.h"

namespace Stokhos {

  //! ModelEvaluator providing statistic response functions
  /*!
   * ResponseStatisticModelEvaluator is an implementation of 
   * EpetraExt::ModelEvaluator that wraps a response-only model evaluator
   * and provides additional response functions that are statistics of
   * some other stochastic response.  Since it is designed to support 
   * derivatives w.r.t. the PCE coefficients of the parameters, the underlying
   * model evaluator should be a block model evaluator.
   */
  class ResponseStatisticModelEvaluator : public EpetraExt::ModelEvaluator {
  public:

    // Constructor
    ResponseStatisticModelEvaluator(
      const Teuchos::RCP<EpetraExt::ModelEvaluator>& me,
      const Teuchos::Array< Teuchos::RCP<const Epetra_Map> >& base_g_maps,
      const Teuchos::RCP<const Stokhos::OrthogPolyBasis<int,double> >& sg_basis,
      const Teuchos::RCP<const EpetraExt::MultiComm>& sg_comm,
      const Teuchos::RCP<const Epetra_BlockMap>& block_map);

    /** \name Overridden from EpetraExt::ModelEvaluator . */
    //@{

    //! Return solution vector map
    Teuchos::RCP<const Epetra_Map> get_x_map() const;

    //! Return residual vector map
    Teuchos::RCP<const Epetra_Map> get_f_map() const;

    //! Return parameter vector map
    Teuchos::RCP<const Epetra_Map> get_p_map(int l) const;

    //! Return response map
    Teuchos::RCP<const Epetra_Map> get_g_map(int l) const;

    //! Return array of parameter names
    Teuchos::RCP<const Teuchos::Array<std::string> > 
    get_p_names(int l) const;

    //! Return initial parameters
    Teuchos::RCP<const Epetra_Vector> get_p_init(int l) const;

    //! Create InArgs
    InArgs createInArgs() const;

    //! Create OutArgs
    OutArgs createOutArgs() const;

    //! Evaluate model on InArgs
    void evalModel(const InArgs& inArgs, const OutArgs& outArgs) const;

    //@}

  protected:

    //! Underlying model evaluator
    Teuchos::RCP<EpetraExt::ModelEvaluator> me;

    //! Base maps of block g vectors
    Teuchos::Array< Teuchos::RCP<const Epetra_Map> > base_g_maps;

    //! Stochastic Galerkin basis
    Teuchos::RCP<const Stokhos::OrthogPolyBasis<int, double> > sg_basis;

    //! Parallel SG communicator
    Teuchos::RCP<const EpetraExt::MultiComm> sg_comm;

    //! Map for stochastic blocks
    Teuchos::RCP<const Epetra_BlockMap> block_map;

    //! Number of parameters
    int num_p;

    //! Number of responses
    int num_g;

  };

}

#endif //STOKHOS_RESPONSE_STATISTIC_MODEL_EVALUATOR_HPP
