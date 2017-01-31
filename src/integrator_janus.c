/**
 * @file    integrator_janus.c
 * @brief   Janus integration scheme.
 * @author  Hanno Rein <hanno@hanno-rein.de>
 * @details This file implements the WHFast integration scheme.  
 * Described in Rein & Tamayo 2015.
 * 
 * @section LICENSE
 * Copyright (c) 2015 Hanno Rein, Daniel Tamayo
 *
 * This file is part of rebound.
 *
 * rebound is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * rebound is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rebound.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "rebound.h"
#include "particle.h"
#include "tools.h"
#include "gravity.h"
#include "boundary.h"
#include "integrator.h"
#include "integrator_whfast.h"

static void to_int(struct reb_particle_int* psi, struct reb_particle* ps, unsigned int N, double int_scale){
    for(unsigned int i=0; i<N; i++){ 
        psi[i].x = ps[i].x*int_scale; 
        psi[i].y = ps[i].y*int_scale; 
        psi[i].z = ps[i].z*int_scale; 
        psi[i].vx = ps[i].vx*int_scale; 
        psi[i].vy = ps[i].vy*int_scale; 
        psi[i].vz = ps[i].vz*int_scale; 
    }
}
static void to_double(struct reb_particle* ps, struct reb_particle_int* psi, unsigned int N, double int_scale){
    for(unsigned int i=0; i<N; i++){ 
        ps[i].x = ((double)psi[i].x)/int_scale; 
        ps[i].y = ((double)psi[i].y)/int_scale; 
        ps[i].z = ((double)psi[i].z)/int_scale; 
        ps[i].vx = ((double)psi[i].vx)/int_scale; 
        ps[i].vy = ((double)psi[i].vy)/int_scale; 
        ps[i].vz = ((double)psi[i].vz)/int_scale; 
    }
}

static void leapfrog(struct reb_simulation* r, double dt){
    struct reb_simulation_integrator_janus* ri_janus = &(r->ri_janus);
    const unsigned int N = r->N;
    const double int_scale  = ri_janus->scale;

    for(int i=0; i<N; i++){
        ri_janus->p_curr[i].x += (__int128)(dt/2.*(double)ri_janus->p_curr[i].vx) ;
        ri_janus->p_curr[i].y += (__int128)(dt/2.*(double)ri_janus->p_curr[i].vy) ;
        ri_janus->p_curr[i].z += (__int128)(dt/2.*(double)ri_janus->p_curr[i].vz) ;
    }
    
    r->gravity_ignore_terms = 0;
    to_double(r->particles, ri_janus->p_curr, N, int_scale); 
    reb_update_acceleration(r);

    for(int i=0; i<N; i++){
        ri_janus->p_curr[i].vx += (__int128)(int_scale*dt*r->particles[i].ax) ;
        ri_janus->p_curr[i].vy += (__int128)(int_scale*dt*r->particles[i].ay) ;
        ri_janus->p_curr[i].vz += (__int128)(int_scale*dt*r->particles[i].az) ;
    }

    for(int i=0; i<N; i++){
        ri_janus->p_curr[i].x += (__int128)(dt/2.*(double)ri_janus->p_curr[i].vx) ;
        ri_janus->p_curr[i].y += (__int128)(dt/2.*(double)ri_janus->p_curr[i].vy) ;
        ri_janus->p_curr[i].z += (__int128)(dt/2.*(double)ri_janus->p_curr[i].vz) ;
    }

}

const double gamma1 = 0.39216144400731413928;
const double gamma2 = 0.33259913678935943860;
const double gamma3 = -0.70624617255763935981;
const double gamma4 = 0.082213596293550800230;
const double gamma5 = 0.79854399093482996340;
void reb_integrator_janus_part1(struct reb_simulation* r){
    r->gravity_ignore_terms = 0;
    struct reb_simulation_integrator_janus* ri_janus = &(r->ri_janus);
    const unsigned int N = r->N;
    if (ri_janus->allocated_N != N){
        const double int_scale  = ri_janus->scale;
        ri_janus->allocated_N = N;
        ri_janus->p_curr = realloc(ri_janus->p_curr, sizeof(struct reb_particle_int)*N);
        to_int(ri_janus->p_curr, r->particles, N, int_scale); 
    }

    leapfrog(r, gamma1*r->dt);
    leapfrog(r, gamma2*r->dt);
    leapfrog(r, gamma3*r->dt);
    leapfrog(r, gamma4*r->dt);
    leapfrog(r, gamma5*r->dt);
    leapfrog(r, gamma4*r->dt);
    leapfrog(r, gamma3*r->dt);
    leapfrog(r, gamma2*r->dt);
    leapfrog(r, gamma1*r->dt);

}

void reb_integrator_janus_part2(struct reb_simulation* r){
    struct reb_simulation_integrator_janus* ri_janus = &(r->ri_janus);
    const unsigned int N = r->N;
    const double int_scale  = ri_janus->scale;
    to_double(r->particles, ri_janus->p_curr, N, int_scale); 
    r->t += r->dt;
}

void reb_integrator_janus_synchronize(struct reb_simulation* r){
}
void reb_integrator_janus_reset(struct reb_simulation* r){
    struct reb_simulation_integrator_janus* const ri_janus = &(r->ri_janus);
    ri_janus->allocated_N = 0;
    if (ri_janus->p_curr){
        free(ri_janus->p_curr);
        ri_janus->p_curr = NULL;
    }
}