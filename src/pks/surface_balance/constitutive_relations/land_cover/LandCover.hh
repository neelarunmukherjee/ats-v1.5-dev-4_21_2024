/*
  ATS is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors: Ethan Coon (ecoon@lanl.gov)
*/
//! Basic land cover/plant function type

/*!

This is a simple base class for PFTs, but currently called LandCover_t to
differentiate from PFT, which is currently used by the dynamic BGC PK.

This class gets used for static compositional land cover, such as NLCD data.
To use this, a list of these specs is included in the "initial conditions" list
of State, and can be used by multiple evaluators somehow.  These are much
cleaner to work with in "new state."

This incorporates a few basic models:
- The rooting profile function from CLM/ELM (see CLM Tech Note)
- Manning's roughness model
- soil resistance on evaporative fluxes (see Jan paper)

Make sure to set the names of the sublists as that of the corresponding domain
partition.

.. _land-cover-spec:
.. admonition:: land-cover-spec

    * `"max rooting depth [m]`" ``[double]`` Max rooting depth [m]
    * `"rooting profile alpha [-]`" ``[double]`` alpha in the rooting profile function [-]
    * `"rooting profile beta [-]`" ``[double]`` beta in the rooting profile function [-]

    * `"mannings n`" ``[double]`` Manning's n [??]

    * `"leaf on time [doy]`" ``[double]`` Day of year, relative to time 0, when leaves
       begin transpiring.  Note that -1 implies evergreen. [doy]
    * `"leaf off time [doy]`" ``[double]`` Day of year, relative to time 0, when leaves
       stop transpiring.  Note that -1 implies evergreen. [doy]

    * `"interception coefficient [-]`" ``[double]`` Fraction, per unit LAI, of
       water intercepted by the canopy.

*/

#pragma once

#include <map>
#include "Teuchos_ParameterList.hpp"

namespace Amanzi {
namespace SurfaceBalance {

//
// LandCover, or PFT struct
//
struct LandCover {
  LandCover(Teuchos::ParameterList& plist);

  // rooting profiles
  double rooting_depth_max;  // [m]
  double rooting_profile_alpha; // alpha in the rooting profile function [-]
  double rooting_profile_beta; // beta in the rooting profile function [-]

  // stomatal limiters
  double stomata_closed_mafic_potential; // [Pa] mafic potential at which stomates are fully closed
  double stomata_open_mafic_potential; // [Pa] mafic potential at which stomates are fully open

  // manning's coef
  double mannings_n; // [?]

  // transpiration controls
  double leaf_on_doy; // [doy], -1 implies evergreen
  double leaf_off_doy; // [doy], -1 implies evergreen

  // radiation parameters
  double albedo_ground;
  double albedo_canopy;
  double emissivity_ground;
  double emissivity_canopy;

  double beers_k_sw;
  double beers_k_lw;

  // transition thickness between snow and bare ground
  // likely this is a property of the understory veg
  double snow_transition_depth; // [m]

  // soil properties controlling evaporation
  double dessicated_zone_thickness; // [m] Thickness over which vapor must diffuse
      //  when the soil is dry.
  double clapp_horn_b; // [-] exponent of the WRM, Clapp & Hornberger eqn 1

};

using LandCoverMap = std::map<std::string, LandCover>;
LandCoverMap getLandCover(Teuchos::ParameterList& plist);


} // namespace SurfaceBalance
} // namespace ATS

