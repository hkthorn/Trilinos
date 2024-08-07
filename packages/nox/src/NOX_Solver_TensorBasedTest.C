// @HEADER
// *****************************************************************************
//            NOX: An Object-Oriented Nonlinear Solver Package
//
// Copyright 2002 NTESS and the NOX contributors.
// SPDX-License-Identifier: BSD-3-Clause
// *****************************************************************************
// @HEADER

/*  Notes:
**
**  (.) The TensorBasedTest solver is virtually identical to the
**  LineSearchBased solver.  Thus, maybe at some point I should remove
**  this TensorBasedTest solver and convert LineSearchBased solver and the
**  LineSearch classes to work with the tensor linesearches.  Right
**  now I see 2 options for this conversion: Make optional argument in
**  LineSearch::compute to allow for a direction argument or add
**  getDirection method to solver so that linesearch object can
**  compute the curvilinear linesearch.  The latter option might have
**  trouble using the const direction.  Need to investigate...
**     //NOX::Abstract::Vector dir2 = dir.clone(ShapeCopy);
**     //const NOX::Direction::Tensor& direction = s.getDirection();
**
**  (.)  Should change to *sufficient* decrease condition instead of
**  just "fprime<0"
**
**  (.)  Maybe move the test of full step into compute instead of in
**  performLinesearch.  However, this might cause trouble with
**  counters and other things.
**
**  (.)  In the dual linesearch, it is checking both full steps and
**  taking the best of either.  This is different from TENSOLVE.
**
**  (.)  Old comment says:
**  "// Note that for Newton direction, fprime = -2.0*oldf"
**  Is this really true?
*/

#include "NOX_Common.H"

#ifdef WITH_PRERELEASE

#include "NOX_Solver_TensorBasedTest.H"    // class definition
#include "NOX_Abstract_Vector.H"
#include "NOX_Abstract_Group.H"
#include "Teuchos_ParameterList.hpp"
#include "NOX_Utils.H"
#include "NOX_GlobalData.H"
#include "NOX_Solver_SolverUtils.H"
#include "NOX_Observer.hpp"

#include "stdio.h"  // for printf()


NOX::Solver::TensorBasedTest::
TensorBasedTest(const Teuchos::RCP<NOX::Abstract::Group>& xgrp,
        const Teuchos::RCP<NOX::StatusTest::Generic>& t,
        const Teuchos::RCP<Teuchos::ParameterList>& p) :
  globalDataPtr(Teuchos::rcp(new NOX::GlobalData(p))),
  utilsPtr(globalDataPtr->getUtils()),
  solnptr(xgrp),
  oldsolnptr(xgrp->clone(DeepCopy)),
  dirptr(xgrp->getX().clone(ShapeCopy)),
  testptr(t),
  paramsPtr(p),
  lineSearch(globalDataPtr, paramsPtr->sublist("Line Search")),
  direction(globalDataPtr, paramsPtr->sublist("Direction"))
{
  NOX::Solver::validateSolverOptionsSublist(p->sublist("Solver Options"));
  observer = NOX::Solver::parseObserver(p->sublist("Solver Options"));
  init();
}

// Protected
void NOX::Solver::TensorBasedTest::init()
{
  // Initialize
  stepSize = 0;
  niter = 0;
  status = NOX::StatusTest::Unconverged;

  checkType = parseStatusTestCheckType(paramsPtr->sublist("Solver Options"));

  // Print out initialization information
  if (utilsPtr->isPrintType(NOX::Utils::Parameters)) {
    utilsPtr->out() << "\n" << NOX::Utils::fill(72) << "\n";
    utilsPtr->out() << "\n-- Parameters Passed to Nonlinear Solver --\n\n";
    paramsPtr->print(utilsPtr->out(),5);
    utilsPtr->out() << "\n" << NOX::Utils::fill(72) << "\n";
  }

}

void NOX::Solver::TensorBasedTest::
reset(const NOX::Abstract::Vector& initialGuess,
      const Teuchos::RCP<NOX::StatusTest::Generic>& t)
{
  solnptr->setX(initialGuess);;
  testptr = t;
  stepSize = 0;
  niter = 0;
  status = NOX::StatusTest::Unconverged;
}

void NOX::Solver::TensorBasedTest::
reset(const NOX::Abstract::Vector& initialGuess)
{
  solnptr->setX(initialGuess);
  stepSize = 0;
  niter = 0;
  status = NOX::StatusTest::Unconverged;
}

void NOX::Solver::TensorBasedTest::
reset()
{
  stepSize = 0;
  niter = 0;
  status = NOX::StatusTest::Unconverged;
}

NOX::Solver::TensorBasedTest::~TensorBasedTest()
{

}


NOX::StatusTest::StatusType
NOX::Solver::TensorBasedTest::getStatus() const
{
  return status;
}

NOX::StatusTest::StatusType  NOX::Solver::TensorBasedTest::step()
{
  observer->runPreIterate(*this);

  // First time thru, perform some initilizations
  if (niter == 0) {

    // Compute F of initial guess
    NOX::Abstract::Group::ReturnType rtype = solnptr->computeF();
    if (rtype != NOX::Abstract::Group::Ok)    {
      utilsPtr->out() << "NOX::Solver::TensorBasedTest::init - "
              << "Unable to compute F" << std::endl;
      throw std::runtime_error("NOX Error");
    }

    // Test the initial guess
    status = testptr->checkStatus(*this, checkType);
    if ((status == NOX::StatusTest::Converged) &&
    (utilsPtr->isPrintType(NOX::Utils::Warning)))  {
      utilsPtr->out() << "Warning: NOX::Solver::TensorBasedTest::init() - "
              << "The solution passed into the solver (either "
              << "through constructor or reset method) "
              << "is already converged!  The solver will not "
              << "attempt to solve this system since status is "
              << "flagged as converged." << std::endl;
    }

    printUpdate();
  }

  // First check status
  if (status != NOX::StatusTest::Unconverged) {
    observer->runPostIterate(*this);
    return status;
  }

  // Copy pointers into temporary references
  NOX::Abstract::Group& soln = *solnptr;
  NOX::Abstract::Group& oldsoln = *oldsolnptr;
  NOX::Abstract::Vector& dir = *dirptr;
  NOX::StatusTest::Generic& test = *testptr;

  // Compute the direction for the update vector at the current solution.
  bool ok;
  ok = direction.compute(dir, soln, *this);
  if (!ok) {
    utilsPtr->out() << "NOX::Solver::TensorBasedTest::iterate - "
     << "unable to calculate direction" << std::endl;
    status = NOX::StatusTest::Failed;
    observer->runPostIterate(*this);
    return status;
  }

  // Update iteration count.
  niter ++;

  // Copy current soln to the old soln.
  oldsoln = soln;

  // Do line search and compute new soln.
  //ok = lineSearch.compute2(soln, step, dir, *this, direction);
  ok = lineSearch.compute(soln, stepSize, dir, *this);
  if (!ok) {
    if (stepSize == 0) {
      utilsPtr->out() << "NOX::Solver::TensorBasedTest::iterate - line search failed" << std::endl;
      status = NOX::StatusTest::Failed;
      observer->runPostIterate(*this);
      return status;
    }
    else if (utilsPtr->isPrintType(NOX::Utils::Warning))
       utilsPtr->out() << "NOX::Solver::TensorBasedTest::iterate - "
        << "using recovery step for line search" << std::endl;
  }


  // Compute F for new current solution.
  NOX::Abstract::Group::ReturnType rtype = soln.computeF();
  if (rtype != NOX::Abstract::Group::Ok)  {
    utilsPtr->out() << "NOX::Solver::LineSearchBased::iterate - "
     << "unable to compute F" << std::endl;
    status = NOX::StatusTest::Failed;
    observer->runPostIterate(*this);
    return status;
  }


  // Evaluate the current status.
  status = test.checkStatus(*this, checkType);

  observer->runPostIterate(*this);

  // Return status.
  return status;
}


NOX::StatusTest::StatusType  NOX::Solver::TensorBasedTest::solve()
{
  observer->runPreSolve(*this);

  this->reset();

  // Iterate until converged or failed
  while (status == NOX::StatusTest::Unconverged) {
    status = step();
    printUpdate();
  }

  Teuchos::ParameterList& outputParams = paramsPtr->sublist("Output");
  outputParams.set("Nonlinear Iterations", niter);
  outputParams.set("2-Norm of Residual", solnptr->getNormF());

  observer->runPostSolve(*this);

  return status;
}

const NOX::Abstract::Group&
NOX::Solver::TensorBasedTest::getSolutionGroup() const
{
  return *solnptr;
}

const NOX::Abstract::Group&
NOX::Solver::TensorBasedTest::getPreviousSolutionGroup() const
{
  return *oldsolnptr;
}

int NOX::Solver::TensorBasedTest::getNumIterations() const
{
  return niter;
}

const Teuchos::ParameterList&
NOX::Solver::TensorBasedTest::getList() const
{
  return *paramsPtr;
}

Teuchos::RCP< const NOX::Abstract::Group >
NOX::Solver::TensorBasedTest::getSolutionGroupPtr() const
{
  return solnptr;
}

Teuchos::RCP< const NOX::Abstract::Group >
NOX::Solver::TensorBasedTest::getPreviousSolutionGroupPtr() const
{
  return oldsolnptr;
}

Teuchos::RCP< const Teuchos::ParameterList >
NOX::Solver::TensorBasedTest::getListPtr() const
{
  return paramsPtr;
}

Teuchos::RCP<const NOX::SolverStats>
NOX::Solver::TensorBasedTest::getSolverStatistics() const
{
  return globalDataPtr->getSolverStatistics();
}

const NOX::Direction::Tensor&
NOX::Solver::TensorBasedTest::getDirection() const
{
  return direction;
}

// protected
void NOX::Solver::TensorBasedTest::printUpdate()
{
  double normSoln = 0;
  double normStep = 0;

  // Print the status test parameters at each iteration if requested
  if ((status == NOX::StatusTest::Unconverged) &&
      (utilsPtr->isPrintType(NOX::Utils::OuterIterationStatusTest))) {
    utilsPtr->out() << NOX::Utils::fill(72) << "\n";
    utilsPtr->out() << "-- Status Test Results --\n";
    testptr->print(utilsPtr->out());
    utilsPtr->out() << NOX::Utils::fill(72) << "\n";
  }

  // All processes participate in the computation of these norms...
  if (utilsPtr->isPrintType(NOX::Utils::InnerIteration)) {
    normSoln = solnptr->getNormF();
    normStep = (niter > 0) ? dirptr->norm() : 0;
  }

  // ...But only the print process actually prints the result.
  if (utilsPtr->isPrintType(NOX::Utils::OuterIteration)) {
    utilsPtr->out() << "\n" << NOX::Utils::fill(72) << "\n";
    utilsPtr->out() << "-- Nonlinear Solver Step " << niter << " -- \n";
    utilsPtr->out() << "f = " << utilsPtr->sciformat(normSoln);
    utilsPtr->out() << "  step = " << utilsPtr->sciformat(stepSize);
    utilsPtr->out() << "  dx = " << utilsPtr->sciformat(normStep);
    if (status == NOX::StatusTest::Converged)
      utilsPtr->out() << " (Converged!)";
    if (status == NOX::StatusTest::Failed)
      utilsPtr->out() << " (Failed!)";
    utilsPtr->out() << "\n" << Utils::fill(72) << "\n" << std::endl;
  }

  // Print the final parameter values of the status test
  if ((status != NOX::StatusTest::Unconverged) &&
      (utilsPtr->isPrintType(NOX::Utils::OuterIteration))) {
    utilsPtr->out() << NOX::Utils::fill(72) << "\n";
    utilsPtr->out() << "-- Final Status Test Results --\n";
    testptr->print(utilsPtr->out());
    utilsPtr->out() << NOX::Utils::fill(72) << "\n";
  }
}

#endif
