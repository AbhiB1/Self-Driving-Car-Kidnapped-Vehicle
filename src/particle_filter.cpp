/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using std::string;
using std::vector;


void ParticleFilter::init(double x, double y, double theta, double std_pos[]) {
    /**
     * TODO: Set the number of particles. Initialize all particles to 
     *   first position (based on estimates of x, y, theta and their uncertainties
     *   from GPS) and all weights to 1. 
     * TODO: Add random Gaussian noise to each particle.
     * NOTE: Consult particle_filter.h for more information about this method 
     *   (and others in this file).
     */
    num_particles = 100;  // TODO: Set the number of particles
	std::default_random_engine ranPos;
	
	std::normal_distribution<double> ini_x(x, std_pos[0]);
	std::normal_distribution<double> ini_y(y, std_pos[1]);
	std::normal_distribution<double> ini_theta(theta, std_pos[2]);
    
    for (int i = 0; i < num_particles; ++i) 
	{
		Particle particle;
		particle.id = int(i);
		particle.weight = 1.0;
		particle.x = ini_x(ranPos);
		particle.y = ini_y(ranPos);
		particle.theta = ini_theta(ranPos);
		
		particles.push_back(particle);
    }
    is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], 
                                double velocity, double yaw_rate) {
    /**
     * TODO: Add measurements to each particle and add random Gaussian noise.
     * NOTE: When adding noise you may find std::normal_distribution 
     *   and std::default_random_engine useful.
     *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
     *  http://www.cplusplus.com/reference/random/default_random_engine/
     */
	std::default_random_engine ranPos;
	for (int i = 0; i < num_particles; ++i)
	{
		double x = particles[i].x;
		double y = particles[i].y;
		double theta = particles[i].theta;

		double theta_p, x_p, y_p;

		if (fabs(yaw_rate) > 1e-5) 
		{
			theta_p = theta + yaw_rate * delta_t;
			x_p = x + velocity / yaw_rate * (sin(theta_p) - sin(theta));
			y_p = y + velocity / yaw_rate * (cos(theta) - cos(theta_p));
		}
		else 
		{
			theta_p = theta;
			x_p = x + velocity * delta_t * cos(theta);
			y_p = y + velocity * delta_t * sin(theta);
		}

		std::normal_distribution<double> dist_x(x_p, std_pos[0]);
		std::normal_distribution<double> dist_y(y_p, std_pos[1]);
		std::normal_distribution<double> dist_theta(theta_p, std_pos[2]);

		particles[i].x = dist_x(ranPos);
		particles[i].y = dist_y(ranPos);
		particles[i].theta = dist_theta(ranPos);
	}
}

void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted, 
                                     vector<LandmarkObs>& observations) {
  /**
   * TODO: Find the predicted measurement that is closest to each 
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */
	for (size_t i = 0 ; i < observations.size(); ++i) 
	{
		double min_dist = std::numeric_limits<double>::max();

		for (size_t j = 0; j < predicted.size(); j++) 
		{
			double d = dist(observations[i].x, observations[i].y, predicted[j].x, predicted[j].y);
			if (d < min_dist) 
			{
				observations[i].id = predicted[j].id;
				min_dist = d;
			}
		}
	}
}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
                                   const vector<LandmarkObs> &observations, 
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian 
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */
   // Gather std values for readability
	double std_x = std_landmark[0];
	double std_y = std_landmark[1];

	// Iterate over all particles
	for (int i = 0; i < num_particles; ++i) 
	{
		double x = particles[i].x;
		double y = particles[i].y;
		double theta = particles[i].theta;

		vector<LandmarkObs> predicted_landmarks;
		for (size_t j = 0; j < map_landmarks.landmark_list.size(); ++j) 
		{
			int id_l = map_landmarks.landmark_list[j].id_i;
			double x_l = (double)map_landmarks.landmark_list[j].x_f;
			double y_l = (double)map_landmarks.landmark_list[j].y_f;

			double d = dist(x, y, x_l, y_l);
			if (d < sensor_range) 
			{
				LandmarkObs prediction;
				prediction.id = id_l;
				prediction.x = x_l;
				prediction.y = y_l;
				predicted_landmarks.push_back(prediction);
			}
		}

		vector<LandmarkObs> observed_landmarks;
		for (size_t j = 0; j < observations.size(); ++j) 
		{
			LandmarkObs translated_observations;
			translated_observations.x = cos(theta) * observations[j].x - sin(theta) * observations[j].y + x;
			translated_observations.y = sin(theta) * observations[j].x + cos(theta) * observations[j].y + y;

			observed_landmarks.push_back(translated_observations);
		}

		dataAssociation(predicted_landmarks, observed_landmarks);

		double true_particle = 1.0;

		double mu_x, mu_y;
		for (size_t j = 0; j < observed_landmarks.size(); ++j) 
		{
			for (size_t k = 0; k < predicted_landmarks.size(); ++k)
			{
				if (observed_landmarks[j].id == predicted_landmarks[k].id)
				{
					mu_x = predicted_landmarks[k].x;
					mu_y = predicted_landmarks[k].y;
					break;
				}
			}

			double norm_factor = 2 * M_PI * std_x * std_y;
			double prob = exp(-(pow(observed_landmarks[j].x - mu_x, 2) / (2 * std_x * std_x) + pow(observed_landmarks[j].y - mu_y, 2) / (2 * std_y * std_y)));

			true_particle *= prob / norm_factor;
		}

		particles[i].weight = true_particle;

	} 

	double norm = 0.0;
	for (size_t i = 0; i < particles.size(); ++i)
		norm += particles[i].weight;

	for (size_t i = 0; i < particles.size(); ++i)
		particles[i].weight /= (norm + std::numeric_limits<double>::epsilon());
}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */
	vector<double> weights;
	std::default_random_engine ranPos;

	for (size_t i = 0; i < particles.size(); ++i)
		weights.push_back(particles[i].weight);

	std::discrete_distribution<int> weighted_distribution(weights.begin(), weights.end());

	vector<Particle> resampled_particles;
	for (int i = 0; i < num_particles; ++i)
	{
		int k = weighted_distribution(ranPos);
		resampled_particles.push_back(particles[k]);
	}

	particles = resampled_particles;

	for (size_t i = 0; i < particles.size(); ++i)
		particles[i].weight = 1.0;
}

void ParticleFilter::SetAssociations(Particle& particle, 
                                     const vector<int>& associations, 
                                     const vector<double>& sense_x, 
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates

    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}