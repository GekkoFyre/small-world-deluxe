#ifndef WATERFALLCOLORMAPS_H
#define WATERFALLCOLORMAPS_H

#include <tuple>
#include <vector>

namespace ColorMaps
{

/* x (control point), red, green, blue
 * all values must be defined between 0.0 and 1.0
 * ControlPoints must be sorted in ascending order of 'x' points.
 */
typedef std::tuple<double, double, double, double> ControlPoint;
typedef std::vector<ControlPoint> ControlPoints;

/**
 * @author Copyright © 2019 Amine Mzoughi <https://github.com/embeddedmz/QwtWaterfallplot>.
 * @return
 */
ControlPoints BlackBodyRadiation();
ControlPoints CoolToWarm();
ControlPoints Jet();

}

#endif // WATERFALLCOLORMAPS_H
