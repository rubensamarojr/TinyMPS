// Copyright (c) 2017 Shota SUGIHARA
// Distributed under the MIT License.
#include <iostream>
#include <boost/format.hpp>
#include <Eigen/Core>
#include "condition.h"
#include "grid.h"
#include "particles.h"

#include "voro++.hh"
#include "vorogmsh.h"

#include <delaunator-header-only.hpp>


// Set up constants for the container geometry
const double x_min=0.2,x_max=0.35;
const double y_min=0.0,y_max=0.1;
const double z_min=0,z_max=0.1;

 // Set up the number of blocks that the container is divided into.
const int n_x=3,n_y=3,n_z=3;

// Sample code using TinyMPS library.
int main(int argc, char* argv[])
{
	/* x0, y0, x1, y1, ... */
    std::vector<double> coords = {-1, 1, 1, 1, 1, -1, -1, -1};

    //triangulation happens here
    delaunator::Delaunator d(coords);

    for(std::size_t i = 0; i < d.triangles.size(); i+=3) {
        printf(
            "Triangle points: [[%f, %f], [%f, %f], [%f, %f]]\n",
            d.coords[2 * d.triangles[i]],        //tx0
            d.coords[2 * d.triangles[i] + 1],    //ty0
            d.coords[2 * d.triangles[i + 1]],    //tx1
            d.coords[2 * d.triangles[i + 1] + 1],//ty1
            d.coords[2 * d.triangles[i + 2]],    //tx2
            d.coords[2 * d.triangles[i + 2] + 1] //ty2
        );
    }
	
	try
	{
		std::string output_path = "./output/";
		std::string input_data = "./input/input.data";
		std::string input_grid = "./input/dam.grid";

		char output_geo[128];
		int igeo = 1;

		if (argc >= 2) output_path = argv[1];
		if (argc >= 3) input_data = argv[2];
		if (argc >= 4) input_grid = argv[3];
		output_path += "output_%1%.vtk";
		tiny_mps::Condition condition(input_data);
		tiny_mps::Particles particles(input_grid, condition);
		tiny_mps::Timer timer(condition);
		Eigen::Vector3d minpos(-0.1, -0.1, 0);
		Eigen::Vector3d maxpos(1.1, 2.1, 0);
		while(particles.nextLoop(output_path, timer))
		{
			particles.calculateTemporaryVelocity(condition.gravity, timer);
			particles.updateTemporaryPosition(timer);
			particles.giveCollisionRepulsionForce();
			particles.updateTemporaryPosition(timer);
			particles.calculateTemporaryParticleNumberDensity();
			particles.checkSurfaceParticles();
			particles.solvePressurePoisson(timer);
			particles.setZeroOnNegativePressure();
			particles.correctVelocity(timer);
			particles.updateTemporaryPosition(timer);
			particles.updateVelocityAndPosition();
			particles.removeOutsideParticles(minpos, maxpos);

			voro::container_poly conp(x_min,x_max,y_min,y_max,z_min,z_max,n_x,n_y,n_z,false,false,false,8);
			//conp.import("pack_six_cube_poly");

			//put(int n,double x,double y,double z,double r);
			double posX, posY, posZ;
			int nParticles = particles.getSize();
			for (int i_particle = 0; i_particle < nParticles; ++i_particle)
			{
				posX = particles.position(0,i_particle);
				posY = particles.position(1,i_particle);
				posZ = particles.position(2,i_particle);
				conp.put(nParticles, posX, posY, posZ, condition.average_distance);
			}

			// Compute voronoi cells from the box conp and
			// draw the figure gnu
			conp.draw_cells_gnuplot("example.gnu");

			// Mesma coisa que acima, porem fora da lib
			voro::c_loop_all cloop(conp);
			voro::voronoicell c;
			double *pp;
			if(cloop.start()) do if(conp.compute_cell(c,cloop))
			{
				pp=conp.p[cloop.ijk]+conp.ps*cloop.q;
				c.draw_gnuplot(*pp,pp[1],pp[2],"example2.gnu");
			} while(cloop.inc());



			// Write GMSH GEO file
			vorogmsh gmsh(conp);
			if(timer.isOutputTime())
			{
				sprintf(output_geo,"./output_geo/output_%d.geo",igeo);
				igeo++;
				gmsh.saveasgeo(output_geo);
			}
			
			//gmsh.saveasgeo("pack_six_cube_poly_01.geo",0.1);
		}
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		exit(EXIT_FAILURE);
	}
	return 1;
}