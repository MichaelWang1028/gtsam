/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file testQPSolver.cpp
 * @brief Test simple QP solver for a linear inequality constraint
 * @date Apr 10, 2014
 * @author Duy-Nguyen Ta
 */

#include <gtsam/inference/Symbol.h>
#include <gtsam/base/Testable.h>
#include <CppUnitLite/TestHarness.h>
#include <gtsam/linear/QPSolver.h>

using namespace std;
using namespace gtsam;
using namespace gtsam::symbol_shorthand;

#define TEST_DISABLED(testGroup, testName)\
    void testGroup##testName##Test(TestResult& result_, const std::string& name_)

/* ************************************************************************* */
// Create test graph according to Forst10book_pg171Ex5
GaussianFactorGraph createTestCase() {
  GaussianFactorGraph graph;

  // Objective functions x1^2 - x1*x2 + x2^2 - 3*x1
  // Note the Hessian encodes:
  //        0.5*x1'*G11*x1 + x1'*G12*x2 + 0.5*x2'*G22*x2 - x1'*g1 - x2'*g2 + 0.5*f
  // Hence, we have G11=2, G12 = -1, g1 = +3, G22 = 2, g2 = 0, f = 0
  graph.push_back(
      HessianFactor(X(1), X(2), 2.0*ones(1, 1), -ones(1, 1), 3.0*ones(1),
          2.0*ones(1, 1), zero(1), 10.0));

  // Inequality constraints
  // Jacobian factors represent Ax-b, ehnce
  // x1 + x2 <= 2 --> x1 + x2 -2 <= 0, --> b=2
  Matrix A1 = (Matrix(4, 1)<<1, -1, 0, 1);
  Matrix A2 = (Matrix(4, 1)<<1, 0, -1, 0);
  Vector b =  (Vector(4)<<2, 0, 0, 1.5);
  // Special constrained noise model denoting <= inequalities with negative sigmas
  noiseModel::Constrained::shared_ptr noise =
      noiseModel::Constrained::MixedSigmas((Vector(4)<<-1, -1, -1, -1));
  graph.push_back(JacobianFactor(X(1), A1, X(2), A2, b, noise));

  return graph;
}

TEST(QPSolver, constraintsAux) {
  GaussianFactorGraph graph = createTestCase();
  QPSolver solver(graph);
  FastVector<size_t> constraintIx = solver.constraintIndices();
  LONGS_EQUAL(1, constraintIx.size());
  LONGS_EQUAL(1, constraintIx[0]);

  VectorValues lambdas;
  lambdas.insert(constraintIx[0], (Vector(4)<< -0.5, 0.0, 0.3, 0.1));
  int factorIx, lambdaIx;
  boost::tie(factorIx, lambdaIx) = solver.findWorstViolatedActiveIneq(lambdas);
  LONGS_EQUAL(1, factorIx);
  LONGS_EQUAL(2, lambdaIx);

  VectorValues lambdas2;
  lambdas2.insert(constraintIx[0], (Vector(4)<< -0.5, 0.0, -0.3, -0.1));
  int factorIx2, lambdaIx2;
  boost::tie(factorIx2, lambdaIx2) = solver.findWorstViolatedActiveIneq(lambdas2);
  LONGS_EQUAL(-1, factorIx2);
  LONGS_EQUAL(-1, lambdaIx2);

  GaussianFactorGraph::shared_ptr freeHessian = solver.freeHessiansOfConstrainedVars();
  GaussianFactorGraph expectedFreeHessian;
  expectedFreeHessian.push_back(
      HessianFactor(X(1), X(2), 2.0 * ones(1, 1), -ones(1, 1), 3.0 * ones(1),
          2.0 * ones(1, 1), zero(1), 1.0));
  EXPECT(expectedFreeHessian.equals(*freeHessian));
}

/* ************************************************************************* */
// Create a simple test graph with one equality constraint
GaussianFactorGraph createEqualityConstrainedTest() {
  GaussianFactorGraph graph;

  // Objective functions x1^2 + x2^2
  // Note the Hessian encodes:
  //        0.5*x1'*G11*x1 + x1'*G12*x2 + 0.5*x2'*G22*x2 - x1'*g1 - x2'*g2 + 0.5*f
  // Hence, we have G11=2, G12 = 0, g1 = 0, G22 = 2, g2 = 0, f = 0
  graph.push_back(
      HessianFactor(X(1), X(2), 2.0*ones(1, 1), zeros(1, 1), zero(1),
          2.0*ones(1, 1), zero(1), 0.0));

  // Equality constraints
  // x1 + x2 = 1 --> x1 + x2 -1 = 0, hence we negate the b vector
  Matrix A1 = (Matrix(1, 1)<<1);
  Matrix A2 = (Matrix(1, 1)<<1);
  Vector b = -(Vector(1)<<1);
  // Special constrained noise model denoting <= inequalities with negative sigmas
  noiseModel::Constrained::shared_ptr noise =
      noiseModel::Constrained::MixedSigmas((Vector(1)<<0.0));
  graph.push_back(JacobianFactor(X(1), A1, X(2), A2, b, noise));

  return graph;
}

TEST(QPSolver, dual) {
  GaussianFactorGraph graph = createEqualityConstrainedTest();

  // Initials values
  VectorValues initials;
  initials.insert(X(1), ones(1));
  initials.insert(X(2), ones(1));

  QPSolver solver(graph);

  GaussianFactorGraph dualGraph = solver.buildDualGraph(graph, initials);
  VectorValues dual = dualGraph.optimize();
  VectorValues expectedDual;
  expectedDual.insert(1, (Vector(1)<<2.0));
  CHECK(assert_equal(expectedDual, dual, 1e-100));
}

/* ************************************************************************* */

TEST(QPSolver, iterate) {
  GaussianFactorGraph graph = createTestCase();
  QPSolver solver(graph);

  GaussianFactorGraph workingGraph = graph.clone();

  VectorValues currentSolution;
  currentSolution.insert(X(1), zeros(1,1));
  currentSolution.insert(X(2), zeros(1,1));

  std::vector<VectorValues> expectedSolutions(3);
  expectedSolutions[0].insert(X(1), (Vector(1) << 4.0/3.0));
  expectedSolutions[0].insert(X(2), (Vector(1) << 2.0/3.0));
  expectedSolutions[1].insert(X(1), (Vector(1) << 1.5));
  expectedSolutions[1].insert(X(2), (Vector(1) << 0.5));
  expectedSolutions[2].insert(X(1), (Vector(1) << 1.5));
  expectedSolutions[2].insert(X(2), (Vector(1) << 0.5));

  bool converged = false;
  int it = 0;
  while (!converged) {
    converged = solver.iterateInPlace(workingGraph, currentSolution);
    CHECK(assert_equal(expectedSolutions[it], currentSolution, 1e-100));
    it++;
  }
}

/* ************************************************************************* */

TEST(QPSolver, optimize) {
  GaussianFactorGraph graph = createTestCase();
  QPSolver solver(graph);
  VectorValues initials;
  initials.insert(X(1), zeros(1,1));
  initials.insert(X(2), zeros(1,1));
  VectorValues solution = solver.optimize(initials);
  VectorValues expectedSolution;
  expectedSolution.insert(X(1), (Vector(1)<< 1.5));
  expectedSolution.insert(X(2), (Vector(1)<< 0.5));
  CHECK(assert_equal(expectedSolution, solution, 1e-100));
}

/* ************************************************************************* */
int main() {
  TestResult tr;
  return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
