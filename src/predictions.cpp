#include "predictions.h"
#include "params.h"
#include <cassert>

using namespace std;

int get_lane(double d)
{
  return (int)(d / param_lane_width);
}

double get_sdistance(double s1, double s2)
{
  // account for s wraparound at max_s
  double sdistance = min( fabs(s1 - s2), min(fabs((s1+max_s) - s2), fabs(s1 - (s2+max_s))) );
  return sdistance;
}

// we generate predictions for closet car per lane in front of us
// we generate predictions for closet car per lane behind us
// => at most 6 predictions (for now on) as we have 3 lanes

// sort of simple scene detection
vector<int> find_closest_objects(vector<vector<double>> sensor_fusion, double car_s)
{
  vector<int> front = {-1, -1, -1}; // idx of closest object per lane
  vector<int> back = {-1, -1, -1}; // idx of closest object per lane

  vector<double> front_dmin = {1e10, 1e10, 1e10}; // per lane
  vector<double> back_dmin = {1e10, 1e10, 1e10}; // per lane

  // Handle FOV and s wraparound
  double sfov_min = car_s - param_fov;
  double sfov_max = car_s + param_fov;
  double sfov_shit = 0;
  if (sfov_min < 0)
  {
    sfov_shit = -sfov_min;
  }
  else if (sfov_max > max_s)
  {
    sfov_shit = max_s - sfov_max;
  }
  sfov_min += sfov_shit;
  sfov_max += sfov_shit;
  assert(sfov_min >= 0 && sfov_min <= max_s);
  assert(sfov_max >= 0 && sfov_max <= max_s);

  car_s += sfov_shit;

  for (int i = 0; i < sensor_fusion.size(); i++)
  {
    double s = sensor_fusion[i][5] + sfov_shit;
    if (s >= sfov_min && s <= sfov_max) // object in FOV
    {
      double d = sensor_fusion[i][6];
      int lane = get_lane(d);
      double dist = get_sdistance(s, car_s);

      if (s > car_s) /* front */
      {
        if (dist < front_dmin[lane])
        {
          front[lane] = i;
          front_dmin[lane] = dist;
        }
      }
      else /* back */
      {
        if (dist < back_dmin[lane])
        {
          back[lane] = i;
          back_dmin[lane] = dist;
        }
      }
    }
  }
  return { front[0], back[0], front[1], back[1], front[2], back[2] };
}


// 6 predictions at most x 50 points x 2 coord (x,y): 6 objects predicted over 1 second horizon
vector<vector<vector<double>>> generate_predictions(vector<vector<double>> sensor_fusion, double car_s, double car_d, int horizon)
{
  vector<vector<vector<double>>> predictions; // vector of at most 6 predicitons of "n_horizon" (x,y) coordinates

  vector<int> closest_objects = find_closest_objects(sensor_fusion, car_s); 

  for (int i = 0; i < closest_objects.size(); i++)
  {
    int obj = closest_objects[i];
    if (obj >= 0)
    {
      double x = sensor_fusion[obj][1];
      double y = sensor_fusion[obj][2];
      double vx = sensor_fusion[obj][3];
      double vy = sensor_fusion[obj][4];
      vector<vector<double>> prediction; // vector of at most 6 predicitons of "n_horizon" (x,y) coordinates
      for (int j = 0; j < horizon; j++)
      {
        prediction.push_back({x + vx * j*param_dt, y + vy * j*param_dt});
      }
      predictions.push_back(prediction);
    }
  }

  return predictions;
}
