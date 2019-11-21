#ifndef XBOXONECONTROLLER_HPP
#define XBOXONECONTROLLER_HPP

namespace XboxOneController
{
namespace Axes
{
static const int X = 0;
static const int Y = 1;
static const int YAW = 3;
static const int PITCH = 4;
static const int V_DOWN = 2;
static const int V_UP = 5;
static const int WRIST = 6;
static const int CAMERA = 7;
} // namespace Axes
namespace Buttons
{
static const int POWER = 8;
static const int MOTORS = 0;
static const int HAND_OPEN = 2;
static const int HAND_CLOSE = 1;
static const int SHOULDER_DOWN = 4;
static const int SHOULDER_UP = 5;
static const int STEPPERS = 3;
static const int MODE = 7;
static const int RESET = 6;
} // namespace Buttons
} // namespace XboxOneController

#endif // XBOXONECONTROLLER_HPP