/*
  Copyright 2010-202x held jointly by participating institutions.
  ATS is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors: Neil Carlson (version 1)
           Konstantin Lipnikov (version 2) (lipnikov@lanl.gov)
           Ethan Coon (ATS version) (ecoon@lanl.gov)
*/

#include "test_snow_dist.hh"

namespace Amanzi {

RegisteredPKFactory_ATS<TestSnowDist> TestSnowDist::reg_("snow distribution test");

} // namespace Amanzi
