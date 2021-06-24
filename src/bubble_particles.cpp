// Copyright (c) 2017 Shota SUGIHARA
// Distributed under the MIT License.
#include "bubble_particles.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <Eigen/LU>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace my_mps{

BubbleParticles::BubbleParticles(const std::string& path, const tiny_mps::Condition& condition)
    : Particles(path, condition) {
  init_bubble_radius = cbrt((3 * condition.initial_void_fraction) / (4 * M_PI * condition.bubble_density * (1 - condition.initial_void_fraction)));
  average_pressure = Eigen::VectorXd::Zero(getSize());
  normal_vector = Eigen::Matrix3Xd::Zero(3, getSize());
  modified_pnd = Eigen::VectorXd::Zero(getSize());
  bubble_radius = Eigen::VectorXd::Constant(getSize(), init_bubble_radius);
  void_fraction = Eigen::VectorXd::Constant(getSize(), condition.initial_void_fraction);
  free_surface_type = Eigen::VectorXi::Zero(getSize());
  average_count = 0;

}

bool BubbleParticles::nextLoop(const std::string& path, tiny_mps::Timer& timer) {
  std::cout << std::endl;
  timer.limitCurrentDeltaTime(getMaxSpeed(), condition_);
  timer.printCompuationTime();
  timer.printTimeInfo();
  showParticlesInfo();
  std::cout << boost::format("Max velocity: %f") % getMaxSpeed() << std::endl;
  saveInterval(path, timer);
  if (checkNeedlessCalculation()) {
    std::cerr << "Error: All particles have become ghost." << std::endl;
    writeVtkFile(path + "err.vtk", (boost::format("Time: %s") % timer.getCurrentTime()).str());
    throw std::range_error("Error: All particles have become ghost.");
  }
  if (timer.isUnderMinDeltaTime()) {
    std::cerr << "Error: Delta time has become so small." << std::endl;
    writeVtkFile(path + "err.vtk", (boost::format("Time: %s") % timer.getCurrentTime()).str());
    throw std::range_error("Error: Delta time has become so small.");
  }
  if (!timer.hasNextLoop()) {
    std::cout << std::endl << "Total ";
    timer.printCompuationTime();
    std::cout << "Succeed in simulation." << std::endl;
    return false;
  }
  timer.update();
  temporary_velocity = velocity;
  temporary_position = position;
  return true;
}

bool BubbleParticles::saveInterval(const std::string& path, const tiny_mps::Timer& timer) const {
  if (!timer.isOutputTime()) return false;
  std::string output_index = (boost::format("%04d") % timer.getOutputCount()).str();
  writeVtkFile((boost::format(path + "output_%1%.vtk") % output_index).str(), (boost::format("Time: %s") % timer.getCurrentTime()).str());
  writeGridVtkFile((boost::format(path + "grid_%1%.vtk") % output_index).str(), (boost::format("Time: %s") % timer.getCurrentTime()).str());
  return true;
}

void BubbleParticles::writeVtkFile(const std::string& path, const std::string& title) const {
  int size = getSize();
  std::ofstream ofs(path);
  if(ofs.fail()) {
    std::cerr << "Error: in writeVtkFile() in particles.cpp." << std::endl;
    throw std::ios_base::failure("Error: in writeVtkFile() in particles.cpp.");
  }
  ofs << "# vtk DataFile Version 2.0" << std::endl;
  ofs << title << std::endl;
  ofs << "ASCII" << std::endl;
  ofs << "DATASET UNSTRUCTURED_GRID" << std::endl;
  ofs << std::endl;
  ofs << "POINTS " << size << " double" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << position(0, i) << " " << position(1, i) << " " << position(2, i) << std::endl;
  }
  ofs << std::endl;
  ofs << "CELLS " << size << " " << size * 2 << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << 1 << " " << i << std::endl;
  }
  ofs << std::endl;
  ofs << "CELL_TYPES " << size << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << 1 << std::endl;
  }
  ofs << std::endl;
  ofs << "POINT_DATA " << size << std::endl;
  ofs << "SCALARS Pressure double" << std::endl;
  ofs << "LOOKUP_TABLE Pressure" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << pressure(i) << std::endl;
  }
  ofs << std::endl;
  ofs << "VECTORS Velocity double" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << velocity(0, i) << " " << velocity(1, i) << " " << velocity(2, i) << std::endl;
  }
  ofs << std::endl;
  ofs << "SCALARS Type int" << std::endl;
  ofs << "LOOKUP_TABLE Type" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << particle_types(i) << std::endl;
  }
  ofs << std::endl;
  ofs << "SCALARS ParticleNumberDensity double" << std::endl;
  ofs << "LOOKUP_TABLE ParticleNumberDensity" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << particle_number_density(i) << std::endl;
  }
  ofs << std::endl;
  ofs << "SCALARS NeighborParticles int" << std::endl;
  ofs << "LOOKUP_TABLE NeighborParticles" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << neighbor_particles(i) << std::endl;
  }
  ofs << std::endl;
  ofs << "SCALARS BoundaryCondition int" << std::endl;
  ofs << "LOOKUP_TABLE BoundaryCondition" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << boundary_types(i) << std::endl;
  }
  ofs << std::endl;
  ofs << "VECTORS CorrectionVelocity double" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << correction_velocity(0, i) << " " << correction_velocity(1, i) << " " << correction_velocity(2, i) << std::endl;
  }
  ofs << std::endl;
  ofs << "SCALARS SourceTerm double" << std::endl;
  ofs << "LOOKUP_TABLE SourceTerm" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << source_term(i) << std::endl;
  }
  ofs << std::endl;
  ofs << "SCALARS VoxelsRatio double" << std::endl;
  ofs << "LOOKUP_TABLE VoxelsRatio" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << voxel_ratio(i) << std::endl;
  }

  // extended
  ofs << std::endl;
  ofs << "SCALARS AveragePressure double" << std::endl;
  ofs << "LOOKUP_TABLE AveragePressure" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << average_pressure(i) << std::endl;
  }
  ofs << std::endl;
  ofs << "VECTORS NormalVector double" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << normal_vector(0, i) << " " << normal_vector(1, i) << " " << normal_vector(2, i) << std::endl;
  }
  ofs << std::endl;
  ofs << "SCALARS BubbleRadius double" << std::endl;
  ofs << "LOOKUP_TABLE BubbleRadius" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << bubble_radius(i) << std::endl;
  }
  ofs << std::endl;
  ofs << "SCALARS VoidFraction double" << std::endl;
  ofs << "LOOKUP_TABLE VoidFraction" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << void_fraction(i) << std::endl;
  }
  ofs << std::endl;
  ofs << "SCALARS FreeSurfaceType int" << std::endl;
  ofs << "LOOKUP_TABLE FreeSurfaceType" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << free_surface_type(i) << std::endl;
  }
  ofs << std::endl;
  ofs << "SCALARS ModifiedParticleNumberDensity double" << std::endl;
  ofs << "LOOKUP_TABLE ModifiedParticleNumberDensity" << std::endl;
  for(int i = 0; i < size; ++i) {
    ofs << modified_pnd(i) << std::endl;
  }

  std::cout << "Succeed in writing vtk file: " << path << std::endl;
}

void BubbleParticles::writeGridVtkFile(const std::string& path, const std::string& title) const {
  std::ofstream ofs(path);
  if(ofs.fail()) {
    std::cerr << "Error: in writeGridVtkFile() in particles.cpp." << std::endl;
    throw std::ios_base::failure("Error: in writeGridVtkFile() in particles.cpp.");
  }
  ofs << "# vtk DataFile Version 2.0" << std::endl;
  ofs << title << std::endl;
  ofs << "ASCII" << std::endl;
  ofs << "DATASET STRUCTURED_POINTS" << std::endl;
  ofs << "DIMENSIONS " << grid_w << " " << grid_h << " 1" << std::endl;
  ofs << "ORIGIN " << grid_min_pos(0) << " " << grid_min_pos(1) <<  " 0.0" << std::endl;
  ofs << "SPACING " << condition_.average_distance << " " << condition_.average_distance << " " << condition_.average_distance << std::endl;
  ofs << "POINT_DATA " << grid_w * grid_h << std::endl;
  ofs << "SCALARS Pressure double" << std::endl;
  ofs << "LOOKUP_TABLE Pressure" << std::endl;
  for(int i = 0; i < grid_w * grid_h; ++i) {
    ofs << average_grid[i] << std::endl;
  }
  ofs << std::endl;

  std::cout << "Succeed in writing grid vtk file: " << path << std::endl;
}

void BubbleParticles::extendStorage(int extra_size) {
  int size = getSize();
  Particles::extendStorage(extra_size);
  average_pressure.conservativeResize(size + extra_size);
  average_pressure.segment(size, extra_size).setZero();
  normal_vector.conservativeResize(3, size + extra_size);
  normal_vector.block(0, size, 3, extra_size).setZero();
  modified_pnd.conservativeResize(size + extra_size);
  modified_pnd.segment(size, extra_size).setZero();
  bubble_radius.conservativeResize(size + extra_size);
  bubble_radius.segment(size, extra_size).setZero();
  void_fraction.conservativeResize(size + extra_size);
  void_fraction.segment(size, extra_size).setZero();
  free_surface_type.conservativeResize(size + extra_size);
  free_surface_type.segment(size, extra_size).setZero();
}

void BubbleParticles::setGhostParticle(int index) {
  Particles::setGhostParticle(index);
  average_pressure(index) = 0.0;
  normal_vector.col(index).setZero();
  modified_pnd(index) = 0.0;
  bubble_radius(index) = 0.0;
  void_fraction(index) = condition_.initial_void_fraction;
  free_surface_type(index) = SurfaceLayer::OTHERS;
}

void BubbleParticles::checkSurface(){
  // First step.
  using namespace tiny_mps;
  for(int i_particle = 0; i_particle < getSize(); ++i_particle) {
    if (particle_types(i_particle) == ParticleType::NORMAL || particle_types(i_particle) == ParticleType::WALL
        || particle_types(i_particle) == ParticleType::INFLOW) {
      if (particle_number_density(i_particle) < condition_.surface_threshold_pnd * initial_particle_number_density
              && neighbor_particles(i_particle) < condition_.surface_threshold_number * initial_neighbor_particles) {
              // && temporary_position(1, i) < shift) {
        boundary_types(i_particle) = BoundaryType::SURFACE;
        free_surface_type(i_particle) = SurfaceLayer::OUTER_SURFACE;
      } else {
        boundary_types(i_particle) = BoundaryType::INNER;
        free_surface_type(i_particle) = SurfaceLayer::INNER;
      }
    } else {
      boundary_types(i_particle) = BoundaryType::OTHERS;
      free_surface_type(i_particle) = SurfaceLayer::OTHERS;
    }
  }
  // Second step.
  Grid grid(condition_.pnd_weight_radius, temporary_position, particle_types.array() != ParticleType::GHOST, dimension);
  const double root2 = std::sqrt(2);
  normal_vector.setZero();
  for(int i_particle = 0; i_particle < getSize(); ++i_particle) {
    if(boundary_types(i_particle) == BoundaryType::SURFACE) {
      Grid::Neighbors neighbors;
      grid.getNeighbors(i_particle, neighbors);
      if (neighbors.empty()) continue;
      for (int j_particle : neighbors) {
        Eigen::Vector3d r_ij = temporary_position.col(j_particle) - temporary_position.col(i_particle);
        normal_vector.col(i_particle) += r_ij.normalized() * weightForParticleNumberDensity(r_ij);
      }
      normal_vector.col(i_particle) /= particle_number_density(i_particle);
      for (int j_particle : neighbors) {
        Eigen::Vector3d r_ij = temporary_position.col(j_particle) - temporary_position.col(i_particle);
        if (r_ij.norm() >= root2 * condition_.average_distance
            && (temporary_position.col(i_particle) + condition_.average_distance * normal_vector.col(i_particle).normalized() - temporary_position.col(j_particle)).norm() < condition_.average_distance) {
          boundary_types(i_particle) = BoundaryType::INNER;
          free_surface_type(i_particle) = SurfaceLayer::INNER;
          break;
        }
        if (r_ij.norm() < root2 * condition_.average_distance
            && r_ij.normalized().dot(normal_vector.col(i_particle).normalized()) > 1.0 / root2) {
          boundary_types(i_particle) = BoundaryType::INNER;
          free_surface_type(i_particle) = SurfaceLayer::INNER;
          break;
        }
      }
    }
  }
  // Third step.
  Grid judge_inner_surface(condition_.average_distance * condition_.secondary_surface_eta, temporary_position, particle_types.array() != ParticleType::GHOST, dimension);
  for (int i_particle = 0; i_particle < getSize(); ++i_particle) {
    if (boundary_types(i_particle) == BoundaryType::SURFACE) {
      Grid::Neighbors neighbors;
      judge_inner_surface.getNeighbors(i_particle, neighbors);
      if (neighbors.empty()) continue;
      for (int j_particle : neighbors) {
        if (boundary_types(j_particle) == BoundaryType::INNER && free_surface_type(j_particle) == SurfaceLayer::INNER)
          free_surface_type(j_particle) = SurfaceLayer::INNER_SURFACE;
      }
    }
  }
}

void BubbleParticles::checkSurface2(){
  // First step.
  using namespace tiny_mps;
  for(int i_particle = 0; i_particle < getSize(); ++i_particle) {
    if (particle_types(i_particle) == ParticleType::NORMAL || particle_types(i_particle) == ParticleType::WALL
        || particle_types(i_particle) == ParticleType::INFLOW) {
      if (particle_number_density(i_particle) < condition_.surface_threshold_pnd * initial_particle_number_density
              && neighbor_particles(i_particle) < condition_.surface_threshold_number * initial_neighbor_particles
              && temporary_position(1, i_particle) < condition_.pnd_weight_radius) {
        boundary_types(i_particle) = BoundaryType::SURFACE;
        free_surface_type(i_particle) = SurfaceLayer::OUTER_SURFACE;
      } else {
        boundary_types(i_particle) = BoundaryType::INNER;
        free_surface_type(i_particle) = SurfaceLayer::INNER;
      }
    } else {
      boundary_types(i_particle) = BoundaryType::OTHERS;
      free_surface_type(i_particle) = SurfaceLayer::OTHERS;
    }
  }
  // Second step.
  Grid grid(condition_.pnd_weight_radius, temporary_position, particle_types.array() != ParticleType::GHOST, dimension);
  normal_vector.setZero();
  for(int i_particle = 0; i_particle < getSize(); ++i_particle) {
    if(boundary_types(i_particle) == BoundaryType::SURFACE) {
      Grid::Neighbors neighbors;
      grid.getNeighbors(i_particle, neighbors);
      if (neighbors.empty()) continue;
      for (int j_particle : neighbors) {
        Eigen::Vector3d r_ij = temporary_position.col(j_particle) - temporary_position.col(i_particle);
        normal_vector.col(i_particle) += r_ij.normalized() * weightForParticleNumberDensity(r_ij);
      }
      normal_vector.col(i_particle) /= particle_number_density(i_particle);
    }
  }
  // Third step.
  Grid judge_inner_surface(condition_.average_distance * condition_.secondary_surface_eta, temporary_position, particle_types.array() != ParticleType::GHOST, dimension);
  for (int i_particle = 0; i_particle < getSize(); ++i_particle) {
    if (boundary_types(i_particle) == BoundaryType::SURFACE) {
      Grid::Neighbors neighbors;
      judge_inner_surface.getNeighbors(i_particle, neighbors);
      if (neighbors.empty()) continue;
      for (int j_particle : neighbors) {
        if (boundary_types(j_particle) == BoundaryType::INNER && free_surface_type(j_particle) == SurfaceLayer::INNER)
          free_surface_type(j_particle) = SurfaceLayer::INNER_SURFACE;
      }
    }
  }
}

void BubbleParticles::calculateBubbles() {
  for (int i_particle = 0; i_particle < getSize(); ++i_particle) {
    if (particle_types(i_particle) == tiny_mps::ParticleType::NORMAL) {
      double del_p = (condition_.vapor_pressure - condition_.head_pressure) - pressure(i_particle);
      // double del_p = (condition_.vapor_pressure - condition_.head_pressure) - average_pressure(i_particle);
      if (del_p > 0) bubble_radius(i_particle) += sqrt(2 * abs(del_p) / (3 * condition_.mass_density));
      else bubble_radius(i_particle) -= sqrt(2 * abs(del_p) / (3 * condition_.mass_density));
      if (bubble_radius(i_particle) > condition_.average_distance) bubble_radius(i_particle) = condition_.average_distance;
      if (bubble_radius(i_particle) < 0) bubble_radius(i_particle) = 0;

      double bubble_vol = 4 * M_PI * condition_.bubble_density * bubble_radius(i_particle) * bubble_radius(i_particle) * bubble_radius(i_particle) / 3;
      void_fraction(i_particle) = bubble_vol / (1 + bubble_vol);
      if (void_fraction(i_particle) < condition_.min_void_fraction) void_fraction(i_particle) = condition_.min_void_fraction;
      if (void_fraction(i_particle) > 0.5) void_fraction(i_particle) = 0.5;
    }
  }
}

void BubbleParticles::calculateBubblesFromAveragePressure() {
  for (int i_particle = 0; i_particle < getSize(); ++i_particle) {
    if (particle_types(i_particle) == tiny_mps::ParticleType::NORMAL) {
      int ix = std::floor((temporary_position(0, i_particle) - grid_min_pos(0) + condition_.average_distance / 2.0) / condition_.average_distance);
      int iy = std::floor((temporary_position(1, i_particle) - grid_min_pos(1) + condition_.average_distance / 2.0) / condition_.average_distance);
      if (ix < 0 || ix >= grid_w) continue;
      if (iy < 0 || iy >= grid_h) continue;
      // double del_p = (condition_.vapor_pressure - condition_.head_pressure) - pressure(i_particle);
      double del_p = (condition_.vapor_pressure - condition_.head_pressure) - average_grid[ix + iy * grid_w];
      if (del_p > 0) bubble_radius(i_particle) += sqrt(2 * abs(del_p) / (3 * condition_.mass_density));
      else bubble_radius(i_particle) -= sqrt(2 * abs(del_p) / (3 * condition_.mass_density));
      if (bubble_radius(i_particle) > condition_.average_distance) bubble_radius(i_particle) = condition_.average_distance;
      if (bubble_radius(i_particle) < 0) bubble_radius(i_particle) = 0;

      double bubble_vol = 4 * M_PI * condition_.bubble_density * bubble_radius(i_particle) * bubble_radius(i_particle) * bubble_radius(i_particle) / 3;
      void_fraction(i_particle) = bubble_vol / (1 + bubble_vol);
      if (void_fraction(i_particle) < condition_.min_void_fraction) void_fraction(i_particle) = condition_.min_void_fraction;
      if (void_fraction(i_particle) > 0.5) void_fraction(i_particle) = 0.5;
    }
  }
}

void BubbleParticles::calculateAveragePressure() {
  using namespace tiny_mps;
  Grid grid(condition_.pnd_weight_radius, temporary_position, boundary_types.array() != BoundaryType::OTHERS, condition_.dimension);
  for (int i_particle = 0; i_particle < getSize(); ++i_particle) {
    if (boundary_types(i_particle) == BoundaryType::OTHERS) {
      average_pressure(i_particle) = 0.0;
      continue;
    }
    Grid::Neighbors neighbors;
    grid.getNeighbors(i_particle, neighbors);
    if (neighbors.empty()) {
      average_pressure(i_particle) = pressure(i_particle);
      continue;
    }
    double numerator = pressure(i_particle) * weightPoly6Kernel(0, condition_.pnd_weight_radius);
    double denominator = weightPoly6Kernel(0, condition_.pnd_weight_radius);
    for (int j_particle : neighbors) {
      Eigen::Vector3d r_ij = temporary_position.col(j_particle) - temporary_position.col(i_particle);
      numerator += pressure(j_particle) * weightPoly6Kernel(r_ij.norm(), condition_.pnd_weight_radius);
      denominator += weightPoly6Kernel(r_ij.norm(), condition_.pnd_weight_radius);
    }
    average_pressure(i_particle) = numerator / denominator;
  }
}

inline double BubbleParticles::weightPoly6Kernel(double r, double h) {
  if (r > h) return 0;
  if (condition_.dimension == 2) {
    return 4 * std::pow(h * h - r * r, 3) / (M_PI * std::pow(h, 8));
  } else {
    return 315 * std::pow(h * h - r * r, 3) / (64 * M_PI * std::pow(h, 8));
  }
}

void BubbleParticles::calculateModifiedParticleNumberDensity() {
  using namespace tiny_mps;
  Grid grid(condition_.average_distance * 1.05, temporary_position, particle_types.array() != ParticleType::GHOST, condition_.dimension);
  Eigen::Vector3d l0_vec(condition_.average_distance, 0.0, 0.0);
  for (int i_particle = 0; i_particle < getSize(); ++i_particle) {
    if (particle_types(i_particle) == ParticleType::GHOST) {
      modified_pnd(i_particle) = 0.0;
      continue;
    }
    double n_hat = initial_particle_number_density;
    Grid::Neighbors neighbors;
    grid.getNeighbors(i_particle, neighbors);
    if (neighbors.empty()) continue;
    for (int j_particle : neighbors) {
      Eigen::Vector3d r_ij = temporary_position.col(j_particle) - temporary_position.col(i_particle);
      n_hat += weightForParticleNumberDensity(r_ij) - weightForParticleNumberDensity(l0_vec);
    }
    modified_pnd(i_particle) = std::max(particle_number_density(i_particle), n_hat);
  }
}

void BubbleParticles::solvePressurePoisson(const tiny_mps::Timer& timer) {
  using namespace tiny_mps;
  Grid grid(condition_.laplacian_pressure_weight_radius, temporary_position, boundary_types.array() != BoundaryType::OTHERS, condition_.dimension);
  using T = Eigen::Triplet<double>;
  double lap_r = grid.getGridWidth();
  int n_size = (int)(std::pow(lap_r * 2, dimension));
  double delta_time = timer.getCurrentDeltaTime();
  Eigen::SparseMatrix<double> p_mat(size, size);
  source_term.setZero();
  std::vector<T> coeffs(size * n_size);
  std::vector<int> neighbors(n_size * 2);
  for (int i_particle = 0; i_particle < size; ++i_particle) {
    if (boundary_types(i_particle) == BoundaryType::OTHERS) {
      coeffs.push_back(T(i_particle, i_particle, 1.0));
      continue;
    } else if (boundary_types(i_particle) == BoundaryType::SURFACE) {
      coeffs.push_back(T(i_particle, i_particle, 1.0));
      continue;
    }
    grid.getNeighbors(i_particle, neighbors);
    double sum = 0.0;
    double div_vel = 0.0;
    for (int j_particle : neighbors) {
      if (boundary_types(j_particle) == BoundaryType::OTHERS) continue;
      Eigen::Vector3d r_ij = temporary_position.col(j_particle) - temporary_position.col(i_particle);
      double mat_ij = weightForLaplacianPressure(r_ij) * 2 * dimension
              / (laplacian_lambda_pressure * initial_particle_number_density);
      sum -= mat_ij;
      div_vel += (temporary_velocity.col(j_particle) - temporary_velocity.col(i_particle)).dot(r_ij)
              * weightForLaplacianPressure(r_ij) * condition_.dimension / (r_ij.squaredNorm() * initial_particle_number_density);
      if (boundary_types(j_particle) == BoundaryType::INNER) {
        coeffs.push_back(T(i_particle, j_particle, mat_ij));
      }
    }
    sum -= condition_.weak_compressibility * condition_.mass_density / (delta_time * delta_time);
    coeffs.push_back(T(i_particle, i_particle, sum));
    double initial_pnd_i = initial_particle_number_density * (1 - void_fraction(i_particle));
    source_term(i_particle) = div_vel * condition_.mass_density * condition_.relaxation_coefficient_vel_div / delta_time
                - (particle_number_density(i_particle) - initial_pnd_i)
                * condition_.relaxation_coefficient_pnd * condition_.mass_density / (delta_time * delta_time * initial_particle_number_density);
  }
  p_mat.setFromTriplets(coeffs.begin(), coeffs.end()); // Finished setup matrix
  solveConjugateGradient(p_mat);
}

void BubbleParticles::solvePressurePoissonDuan(const tiny_mps::Timer& timer) {
  using namespace tiny_mps;
  Grid grid(condition_.laplacian_pressure_weight_radius, temporary_position, boundary_types.array() != BoundaryType::OTHERS, condition_.dimension);
  using T = Eigen::Triplet<double>;
  double lap_r = grid.getGridWidth();
  int n_size = (int)(std::pow(lap_r * 2, dimension));
  double delta_time = timer.getCurrentDeltaTime();
  Eigen::SparseMatrix<double> p_mat(size, size);
  source_term.setZero();
  std::vector<T> coeffs(size * n_size);
  std::vector<int> neighbors(n_size * 2);
  for (int i_particle = 0; i_particle < size; ++i_particle) {
    if (boundary_types(i_particle) == BoundaryType::OTHERS) {
      coeffs.push_back(T(i_particle, i_particle, 1.0));
      continue;
    } else if (boundary_types(i_particle) == BoundaryType::SURFACE) {
      coeffs.push_back(T(i_particle, i_particle, 1.0));
      continue;
    }
    grid.getNeighbors(i_particle, neighbors);
    double sum = 0.0;
    double div_vel = 0.0;
    for (int j_particle : neighbors) {
      if (boundary_types(j_particle) == BoundaryType::OTHERS) continue;
      Eigen::Vector3d r_ij = temporary_position.col(j_particle) - temporary_position.col(i_particle);
      double mat_ij = weightForLaplacianPressure(r_ij) * 2 * dimension
              / (laplacian_lambda_pressure * initial_particle_number_density);
      sum -= mat_ij;
      div_vel += (temporary_velocity.col(j_particle) - temporary_velocity.col(i_particle)).dot(r_ij)
              * weightForLaplacianPressure(r_ij) * condition_.dimension / (r_ij.squaredNorm() * initial_particle_number_density);
      if (boundary_types(j_particle) == BoundaryType::INNER) {
        coeffs.push_back(T(i_particle, j_particle, mat_ij));
      }
    }
    sum -= condition_.weak_compressibility * condition_.mass_density / (delta_time * delta_time);
    if (free_surface_type(i_particle) == SurfaceLayer::INNER_SURFACE) {
      sum -= (modified_pnd(i_particle) - particle_number_density(i_particle)) * 2 * dimension / (laplacian_lambda_pressure * initial_particle_number_density);
      coeffs.push_back(T(i_particle, i_particle, sum));
      double initial_pnd_i = initial_particle_number_density * (1 - void_fraction(i_particle));
      source_term(i_particle) = div_vel * condition_.mass_density * condition_.relaxation_coefficient_vel_div / delta_time
                - (modified_pnd(i_particle) - initial_pnd_i)
                * condition_.relaxation_coefficient_pnd * condition_.mass_density / (delta_time * delta_time * initial_particle_number_density);
      // source_term(i_particle) = div_vel * condition_.mass_density * condition_.relaxation_coefficient_vel_div / delta_time
      //           - (modified_pnd(i_particle) - initial_particle_number_density)
      //           * condition_.relaxation_coefficient_pnd * condition_.mass_density / (delta_time * delta_time * initial_particle_number_density);
    } else {
      coeffs.push_back(T(i_particle, i_particle, sum));
      // double initial_pnd_i = initial_particle_number_density * (1 - void_fraction(i_particle));
      source_term(i_particle) = div_vel * condition_.mass_density * condition_.relaxation_coefficient_vel_div / delta_time
                - (particle_number_density(i_particle) - initial_particle_number_density)
                * condition_.relaxation_coefficient_pnd * condition_.mass_density / (delta_time * delta_time * initial_particle_number_density);
    }
  }
  p_mat.setFromTriplets(coeffs.begin(), coeffs.end()); // Finished setup matrix
  solveConjugateGradient(p_mat);
}

void BubbleParticles::correctVelocityDuan(const tiny_mps::Timer& timer) {
  using namespace tiny_mps;
  Grid grid(condition_.gradient_radius, temporary_position, boundary_types.array() != BoundaryType::OTHERS, condition_.dimension);
  correction_velocity.setZero();
  int tensor_count = 0;
  int not_tensor_count = 0;
  for (int i_particle = 0; i_particle < size; ++i_particle) {
    if (particle_types(i_particle) != ParticleType::NORMAL) continue;
    if (boundary_types(i_particle) == BoundaryType::OTHERS) continue;
    Grid::Neighbors neighbors;
    grid.getNeighbors(i_particle, neighbors);
    Eigen::Matrix3d tensor = Eigen::Matrix3d::Zero();
    Eigen::Vector3d tmp_vel(0.0, 0.0, 0.0);
    if (free_surface_type(i_particle) == SurfaceLayer::INNER_SURFACE) {
      for (int j_particle : neighbors) {
        if (boundary_types(j_particle) == BoundaryType::OTHERS) continue;
        Eigen::Vector3d r_ij = temporary_position.col(j_particle) - temporary_position.col(i_particle);
        tmp_vel += r_ij * (pressure(j_particle) + pressure(i_particle)) * weightForGradientPressure(r_ij) / r_ij.squaredNorm();
      }
      if (dimension == 2) tmp_vel(2) = 0;
      correction_velocity.col(i_particle) -= tmp_vel * dimension * timer.getCurrentDeltaTime() / (initial_particle_number_density * condition_.mass_density);
    } else {
      double p_min = pressure(i_particle);
      double p_max = pressure(i_particle);
      for (int j_particle : neighbors) {
        if (boundary_types(j_particle) == BoundaryType::OTHERS) continue;
        p_min = std::min(pressure(j_particle), p_min);
        p_max = std::max(pressure(j_particle), p_max);
      }
      for (int j_particle : neighbors) {
        if (boundary_types(j_particle) == BoundaryType::OTHERS) continue;
        Eigen::Vector3d r_ij = temporary_position.col(j_particle) - temporary_position.col(i_particle);
        Eigen::Vector3d n_ij = r_ij.normalized();
        Eigen::Matrix3d tmp_tensor = Eigen::Matrix3d::Zero();
        tmp_tensor << n_ij(0) * n_ij(0), n_ij(0) * n_ij(1), n_ij(0) * n_ij(2),
                      n_ij(1) * n_ij(0), n_ij(1) * n_ij(1), n_ij(1) * n_ij(2),
                      n_ij(2) * n_ij(0), n_ij(2) * n_ij(1), n_ij(2) * n_ij(2);
        tensor += tmp_tensor * weightForGradientPressure(r_ij) / initial_particle_number_density;
        double xi = 0.2 + 2 * normal_vector.col(j_particle).norm();
        tmp_vel += r_ij * (pressure(j_particle) - pressure(i_particle) + xi * (p_max - p_min)) * weightForGradientPressure(r_ij) / r_ij.squaredNorm();
      }
      if (dimension == 2) {
        tmp_vel(2) = 0;
        tensor(2, 2) = 1.0;
      }
      if (tensor.determinant() > 1.0e-10) {
        correction_velocity.col(i_particle) -= tensor.inverse() * tmp_vel * timer.getCurrentDeltaTime() / (initial_particle_number_density * condition_.mass_density);
        ++tensor_count;
      } else {
        correction_velocity.col(i_particle) -= tmp_vel * dimension * timer.getCurrentDeltaTime() / (initial_particle_number_density * condition_.mass_density);
        ++not_tensor_count;
      }
    }
  }
  std::cout << "Tensor: " << tensor_count << ", Not Tensor: " << not_tensor_count << std::endl;
  temporary_velocity += correction_velocity;
}

void BubbleParticles::initAverageGrid(const Eigen::Vector3d& min_pos, const Eigen::Vector3d& max_pos) {
  Eigen::Vector3d r_ij = max_pos - min_pos;
  grid_min_pos = min_pos;
  grid_max_pos = max_pos;
  grid_w = r_ij(0) / condition_.average_distance + 1;
  grid_h = r_ij(1) / condition_.average_distance + 1;
  average_grid.reserve(grid_w * grid_h);
  for (int i = 0; i < grid_w * grid_h; i++) {
    average_grid[i] = 0;
  }
}

void BubbleParticles::updateAverageGrid(double start_time, const tiny_mps::Timer& timer) {
  using namespace tiny_mps;
  std::cout << "average: " << average_count  << ", " << timer.getLoopCount() << std::endl;
  if (start_time > timer.getCurrentTime()) {
    average_count = timer.getLoopCount();
    return;
  }
  std::vector<int> number(grid_w * grid_h);
  std::vector<double> tmp_average(grid_w * grid_h);
  for (int i_grid = 0; i_grid < grid_w * grid_h; ++i_grid) {
    tmp_average[i_grid] = 0.0;
    number[i_grid] = 0.0;
  }
  for (int i_particle = 0; i_particle < getSize(); ++i_particle) {
    if (boundary_types(i_particle) == BoundaryType::OTHERS) continue;
    int ix = std::floor((temporary_position(0, i_particle) - grid_min_pos(0) + condition_.average_distance / 2.0) / condition_.average_distance);
    int iy = std::floor((temporary_position(1, i_particle) - grid_min_pos(1) + condition_.average_distance / 2.0) / condition_.average_distance);
    if (ix < 0 || ix >= grid_w) continue;
    if (iy < 0 || iy >= grid_h) continue;
    tmp_average[ix + iy * grid_w] += pressure(i_particle);
    number[ix + iy * grid_w]++;
  }
  int n = timer.getLoopCount() - average_count;
  for (int i_grid = 0; i_grid < grid_w * grid_h; ++i_grid) {
    if (number[i_grid] > 0) tmp_average[i_grid] /= number[i_grid];
    average_grid[i_grid] = (average_grid[i_grid] * n + tmp_average[i_grid])/ (n + 1);
  }

}

}