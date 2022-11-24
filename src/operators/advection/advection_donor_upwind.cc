/* -*-  mode: c++; indent-tabs-mode: nil -*- */
/* -------------------------------------------------------------------------
   ATS

   License: see $ATS_DIR/COPYRIGHT
   Author: Ethan Coon

   Donor upwind advection.
   ------------------------------------------------------------------------- */

#include "advection_donor_upwind.hh"

namespace Amanzi {
namespace Operators {


AdvectionDonorUpwind::AdvectionDonorUpwind(Teuchos::ParameterList& advect_plist,
                                           const Teuchos::RCP<const AmanziMesh::Mesh> mesh)
  : Advection(advect_plist, mesh)
{
  upwind_cell_ = Teuchos::rcp(new Epetra_IntVector(mesh_->face_map(true)));
  downwind_cell_ = Teuchos::rcp(new Epetra_IntVector(mesh_->face_map(true)));
};


// set flux and determine upwind cells
void
AdvectionDonorUpwind::set_flux(const Teuchos::RCP<const CompositeVector>& flux)
{
  flux_ = flux;
  IdentifyUpwindCells_();
};


void
AdvectionDonorUpwind::Apply(const Teuchos::RCP<Functions::BoundaryFunction>& bc_flux,
                            bool include_bc_fluxes){

  // Part 1: Collect fluxes in faces
  { field_->ScatterMasterToGhosted("cell"); // communicate the cells
const Epetra_MultiVector& field_c = *field_->ViewComponent("cell", true);
Epetra_MultiVector& field_f = *field_->ViewComponent("face", true);

flux_->ScatterMasterToGhosted("face");
const Epetra_MultiVector& flux = *flux_->ViewComponent("face", true);

unsigned int nfaces_ghosted = field_f.MyLength();
for (unsigned int f = 0; f != nfaces_ghosted; ++f) { // loop over master and slave faces
  int c1 = (*upwind_cell_)[f];
  if (c1 >= 0) {
    double u = std::abs(flux[0][f]);
    for (unsigned int i = 0; i != num_dofs_; ++i) { field_f[i][f] = u * field_c[i][c1]; }
  }
}
} // namespace Operators

// Part 2: put fluxes in cell
{
  Epetra_MultiVector& field_c = *field_->ViewComponent("cell", false);
  field_c.PutScalar(0.);
  unsigned int ncells_owned = field_c.MyLength();

  // no scatter required
  const Epetra_MultiVector& field_f = *field_->ViewComponent("face", true);

  unsigned int nfaces_ghosted = field_f.MyLength();
  for (unsigned int f = 0; f != nfaces_ghosted; ++f) { // loop over master and slave faces
    int c1 = (*upwind_cell_)[f];
    int c2 = (*downwind_cell_)[f];

    if (c1 >= 0 && c1 < ncells_owned) {
      for (int i = 0; i != num_dofs_; ++i) { field_c[i][c1] -= field_f[i][f]; }
    }

    if (c2 >= 0 && c2 < ncells_owned) {
      for (int i = 0; i != num_dofs_; ++i) { field_c[i][c2] += field_f[i][f]; }
    }
  }
}
}; // namespace Amanzi


void
AdvectionDonorUpwind::IdentifyUpwindCells_()
{
  upwind_cell_->PutValue(-1);
  downwind_cell_->PutValue(-1);

  AmanziMesh::Entity_ID_List faces;
  std::vector<int> fdirs;

  flux_->ScatterMasterToGhosted("face");
  const Epetra_MultiVector& flux_f = *flux_->ViewComponent("face", true);

  unsigned int ncells_used = mesh_->num_entities(AmanziMesh::CELL, AmanziMesh::Parallel_type::ALL);
  for (unsigned int c = 0; c != ncells_used; ++c) {
    mesh_->cell_get_faces_and_dirs(c, &faces, &fdirs);

    for (unsigned int i = 0; i != faces.size(); ++i) {
      AmanziMesh::Entity_ID f = faces[i];
      if (flux_f[0][f] * fdirs[i] >= 0) {
        (*upwind_cell_)[f] = c;
      } else {
        (*downwind_cell_)[f] = c;
      }
    }
  }
};

} // namespace Operators
} // namespace Amanzi
