#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>

#include <climits>

#include "MqttClient.h"

#include <json.hpp>

#include "PolitoceanConstants.h"
#include "PolitoceanExceptions.hpp"
#include <mqttLogger.h>

#include "Reflectables/Vector.hpp"

#include "ComponentsManager.hpp"
#include "Button.hpp"

#include "XboxOneController.hpp"

using namespace Politocean;
using namespace Politocean::Constants;

/**************************************************************
 * Listener class for Joystick device
 *************************************************************/

class Listener
{
    std::queue<Button> buttons_;

    bool isButtonUpdated_ = false;
    bool isAxesUpdated_ = false;

    Types::Vector<int> axes_;

    std::mutex mutexBtn_;

public:
    void listenForButtons(Button button);
    void listenForAxes(Types::Vector<int> axes);

    Button button();

    Types::Vector<int> axes();

    bool isButtonUpdated();
    bool isAxesUpdated();
};

void Listener::listenForButtons(Button button)
{
    buttons_.push(button);

    isButtonUpdated_ = true;
}

Button Listener::button()
{
    std::lock_guard<std::mutex> lock(mutexBtn_);

    if (buttons_.empty())
        return Button(-1, 0);

    Button button = buttons_.front();
    buttons_.pop();

    if (buttons_.empty())
        isButtonUpdated_ = false;

    return button;
}

bool Listener::isButtonUpdated()
{
    return isButtonUpdated_;
}

bool Listener::isAxesUpdated()
{
    return isAxesUpdated_;
}

void Listener::listenForAxes(Types::Vector<int> axes)
{
    if (axes_ == axes)
        return;

    axes_ = axes;

    isAxesUpdated_ = true;
}

Types::Vector<int> Listener::axes()
{
    isAxesUpdated_ = false;
    return axes_;
}

/**************************************************************
 * Talker class for Joystick publisher
 *************************************************************/

class Talker
{
    /**
     * @buttonTalker	: talker thread for button value
     */
    std::thread *buttonTalker_, *axesTalker_;

    /**
     * @isTalking_ : it is true if the talker is isTalking
     */
    bool isTalking_ = false;

public:
    void startTalking(MqttClient &publisher, Listener &listener);
    void stopTalking();

    bool isTalking();
};

void Talker::startTalking(MqttClient &publisher, Listener &listener)
{

    if (isTalking_)
        return;

    isTalking_ = true;

    axesTalker_ = new std::thread([&]() {
        while (publisher.is_connected())
        {
            if (!listener.isAxesUpdated())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(Timing::Milliseconds::JOYSTICK));
                continue;
            }

            Types::Vector<int> axes = listener.axes();

            Types::Vector<int> atmega = {
                axes[XboxOneController::Axes::X],                                            // Commands::ATMega::Axes::X_AXIS
                axes[XboxOneController::Axes::Y],                                            // Commands::ATMega::Axes::Y_AXIS
                axes[XboxOneController::Axes::V_UP] - axes[XboxOneController::Axes::V_DOWN], // Commands::ATMega::Axes::Z_AXIS
                axes[XboxOneController::Axes::YAW],                                          // Commands::ATMega::Axes::RZ_AXIS
                axes[XboxOneController::Axes::PITCH]};                                       // Commands::ATMega::Axes::PITCH_AXIS

            publisher.publish(Topics::AXES, atmega);

            if (axes[XboxOneController::Axes::WRIST] == 0)
                publisher.publish(Topics::WRIST, Commands::Actions::STOP);
            else
            {
                publisher.publish(Topics::WRIST_VELOCITY, std::to_string(axes[XboxOneController::Axes::WRIST]));
                publisher.publish(Topics::WRIST, Commands::Actions::START);
            }

            if (axes[XboxOneController::Axes::CAMERA] > 0)
            {
                publisher.publish(Topics::HEAD, Commands::Actions::Stepper::DOWN);
            }
            else if (axes[XboxOneController::Axes::CAMERA] < 0)
            {
                publisher.publish(Topics::HEAD, Commands::Actions::Stepper::UP);
            }
            else
            {
                publisher.publish(Topics::HEAD, Commands::Actions::STOP);
            }
        }

        isTalking_ = false;
    });

    buttonTalker_ = new std::thread([&]() {
        std::string mode = Commands::Actions::ATMega::SLOW;

        while (publisher.is_connected())
        {

            if (!listener.isButtonUpdated())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(Timing::Milliseconds::COMMANDS));
                continue;
            }

            Button button = listener.button();

            // Parsing button by identifier
            switch (button.getId())
            {
            case XboxOneController::Buttons::POWER:
                if (button.getValue())
                {
                    Component::Status powerStatus = ComponentsManager::GetComponentState(component_t::POWER);
                    if (powerStatus == Component::Status::ENABLED)
                    {
                        publisher.publish(Topics::COMMANDS, Commands::Actions::OFF);
                        publisher.publish(Topics::COMMANDS, Commands::Actions::RESET);
                    }
                    else if (powerStatus == Component::Status::DISABLED)
                        publisher.publish(Topics::COMMANDS, Commands::Actions::ON);
                    else
                        break;
                }
                break;

            case XboxOneController::Buttons::MOTORS:
                if (button.getValue())
                    publisher.publish(Topics::COMMANDS, Commands::Actions::ATMega::START_AND_STOP);
                break;

            case XboxOneController::Buttons::RESET:
                if (button.getValue())
                    publisher.publish(Topics::COMMANDS, Commands::Actions::RESET);
                break;

            case XboxOneController::Buttons::STEPPERS:
                if (button.getValue())
                {
                    Component::Status shoulderStatus = ComponentsManager::GetComponentState(component_t::SHOULDER);
                    Component::Status wristStatus = ComponentsManager::GetComponentState(component_t::WRIST);
                    Component::Status headStatus = ComponentsManager::GetComponentState(component_t::HEAD);

                    if (!(shoulderStatus == wristStatus && shoulderStatus == headStatus))
                    {
                        publisher.publish(Topics::SHOULDER, Commands::Actions::OFF);
                        publisher.publish(Topics::WRIST, Commands::Actions::OFF);
                        publisher.publish(Topics::HEAD, Commands::Actions::OFF);

                        break;
                    }

                    // I'm checking just if one component is enabled as considered above
                    if (shoulderStatus == Component::Status::ENABLED)
                    {
                        publisher.publish(Topics::SHOULDER, Commands::Actions::OFF);
                        publisher.publish(Topics::WRIST, Commands::Actions::OFF);
                        publisher.publish(Topics::HEAD, Commands::Actions::OFF);
                    }
                    else if (shoulderStatus == Component::Status::DISABLED)
                    {
                        publisher.publish(Topics::SHOULDER, Commands::Actions::ON);
                        publisher.publish(Topics::WRIST, Commands::Actions::ON);
                        publisher.publish(Topics::HEAD, Commands::Actions::ON);
                    }
                }
                break;

            case XboxOneController::Buttons::SHOULDER_UP:
                if (button.getValue())
                    publisher.publish(Topics::SHOULDER, Commands::Actions::Stepper::UP);
                else
                    publisher.publish(Topics::SHOULDER, Commands::Actions::STOP);
                break;

            case XboxOneController::Buttons::SHOULDER_DOWN:
                if (button.getValue())
                    publisher.publish(Topics::SHOULDER, Commands::Actions::Stepper::DOWN);
                else
                    publisher.publish(Topics::SHOULDER, Commands::Actions::STOP);
                break;

            case XboxOneController::Buttons::HAND_CLOSE:
                if (button.getValue())
                {
                    publisher.publish(Topics::HAND_VELOCITY, std::to_string(INT_MAX));
                    publisher.publish(Topics::HAND, Commands::Actions::START);
                }
                else
                    publisher.publish(Topics::HAND, Commands::Actions::STOP);
                break;

            case XboxOneController::Buttons::HAND_OPEN:
                if (button.getValue())
                {
                    publisher.publish(Topics::HAND_VELOCITY, std::to_string(INT_MIN));
                    publisher.publish(Topics::HAND, Commands::Actions::START);
                }
                else
                    publisher.publish(Topics::HAND, Commands::Actions::STOP);
                break;

            case XboxOneController::Buttons::MODE:
                if (button.getValue())
                {
                    if (mode == Commands::Actions::ATMega::SLOW)
                        mode = Commands::Actions::ATMega::MEDIUM;
                    else if (mode == Commands::Actions::ATMega::MEDIUM)
                        mode = Commands::Actions::ATMega::FAST;
                    else
                        mode = Commands::Actions::ATMega::SLOW;

                    publisher.publish(Topics::COMMANDS, mode);
                }
                break;

            default:
                break;
            }
        }

        isTalking_ = false;
    });
}

void Talker::stopTalking()
{
    if (!isTalking_)
        return;

    isTalking_ = false;
    buttonTalker_->join();
}

bool Talker::isTalking()
{
    return isTalking_;
}

int main(int argc, const char *argv[])
{
    mqttLogger::setRootTag(argv[0]);

    MqttClient &hmiClient = MqttClient::getInstance(Constants::Hmi::CMD_ID, Constants::Hmi::IP_ADDRESS);
    Listener listener;
    Talker talker;

    ComponentsManager::Init(Hmi::CMD_ID);

    hmiClient.subscribeTo(Topics::JOYSTICK_BUTTONS, &Listener::listenForButtons, &listener);
    hmiClient.subscribeTo(Topics::JOYSTICK_AXES, &Listener::listenForAxes, &listener);

    talker.startTalking(MqttClient::getInstance(Constants::Hmi::CMD_ID, Constants::Rov::IP_ADDRESS), listener);

    hmiClient.wait();

    talker.stopTalking();

    return 0;
}