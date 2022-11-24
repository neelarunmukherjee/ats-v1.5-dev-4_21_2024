/*
  ATS is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/
//! Internal energy based on a linear fit.

/*!

Linear internal energy model -- function of Cv and temperature

.. math::

    u = L_f +  C_v * (T - T_{ref})

.. _iem-linear-spec
.. admonition:: iem-linear-spec

    * `"reference temperature [K]`" ``[double]`` **273.15** The phase transition point, T_ref above

    ONE OF

    * `"latent heat [J kg^-1]`" ``[double]`` Latent heat of fusion, L_f above
    * `"heat capacity [J kg^-1 K^-1]`" ``[double]`` C_v above

    OR

    * `"latent heat [J mol^-1]`" ``[double]`` Latent heat of fusion, L_f above.
    * `"heat capacity [J mol^-1 K^-1]`" ``[double]`` C_v above

    END

*/

#ifndef AMANZI_ENERGY_RELATIONS_IEM_LINEAR_
#define AMANZI_ENERGY_RELATIONS_IEM_LINEAR_

#include "Teuchos_ParameterList.hpp"

#include "iem.hh"
#include "Factory.hh"

namespace Amanzi {
namespace Energy {

class IEMLinear : public IEM {
 public:
  explicit IEMLinear(Teuchos::ParameterList& plist);

  bool IsMolarBasis() { return molar_basis_; }

  double InternalEnergy(double temp);
  double DInternalEnergyDT(double temp) { return Cv_; }

 private:
  virtual void InitializeFromPlist_();

  Teuchos::ParameterList plist_;

  double Cv_;    // units: MJ/({mol/kg}-K)
  double T_ref_; // units: K
  bool molar_basis_;
  double L_;

 private:
  // iem factor registration
  static Utils::RegisteredFactory<IEM, IEMLinear> factory_;
};

} // namespace Energy
} // namespace Amanzi

#endif
