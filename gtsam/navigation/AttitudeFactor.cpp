/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 *  @file   AttitudeFactor.cpp
 *  @author Frank Dellaert
 *  @brief  Implementation file for Attitude factor
 *  @date   January 28, 2014
 **/

#include "AttitudeFactor.h"

using namespace std;

namespace gtsam {

//***************************************************************************
void AttitudeFactor::print(const string& s,
    const KeyFormatter& keyFormatter) const {
  cout << s << "AttitudeFactor on " << keyFormatter(this->key()) << "\n";
  nZ_.print("  measured direction in nav frame: ");
  bRef_.print("  reference direction in body frame: ");
  this->noiseModel_->print("  noise model: ");
}

//***************************************************************************
bool AttitudeFactor::equals(const NonlinearFactor& expected, double tol) const {
  const This* e = dynamic_cast<const This*>(&expected);
  return e != NULL && Base::equals(*e, tol) && this->nZ_.equals(e->nZ_, tol)
      && this->bRef_.equals(e->bRef_, tol);
}

//***************************************************************************
Vector AttitudeFactor::evaluateError(const Pose3& p,
    boost::optional<Matrix&> H) const {
  const Rot3& nRb = p.rotation();
  if (H) {
    Matrix D_nRef_R, D_e_nRef;
    Sphere2 nRef = nRb.rotate(bRef_, D_nRef_R);
    Vector e = nZ_.error(nRef, D_e_nRef);
    H->resize(2, 6);
    H->block < 2, 3 > (0, 0) = D_e_nRef * D_nRef_R;
    H->block < 2, 3 > (0, 3) << Matrix::Zero(2, 3);
    return e;
  } else {
    Sphere2 nRef = nRb * bRef_;
    return nZ_.error(nRef);
  }
}

//***************************************************************************

}/// namespace gtsam