#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>

#include "MqttClient.h"

#include <json.hpp>

#include "PolitoceanConstants.h"
#include "PolitoceanExceptions.hpp"
#include <mqttLogger.h>

#include "Reflectables/Vector.hpp"

#include "ComponentsManager.hpp"
#include "Button.hpp"

using namespace Politocean;
using namespace Politocean::Constants;
using namespace Politocean::Constants::Commands;

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

    return button;
}

bool Listener::isButtonUpdated()
{
    return !buttons_.empty();
}

bool Listener::isAxesUpdated()
{
    return isAxesUpdated_ && !axes_.empty();
}

void Listener::listenForAxes(Types::Vector<int> axes)
{
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
        std::map<int, int> prevAxes;
        prevAxes.insert(std::pair<int, int>(Axes::X, 0));
        prevAxes.insert(std::pair<int, int>(Axes::Y, 0));
        prevAxes.insert(std::pair<int, int>(Axes::RZ, 0));
        prevAxes.insert(std::pair<int, int>(Axes::SHOULDER, 0));
        prevAxes.insert(std::pair<int, int>(Axes::WRIST, 0));
        prevAxes.insert(std::pair<int, int>(Axes::HAND, 0));
        prevAxes.insert(std::pair<int, int>(Axes::PITCH, 0));

        while (publisher.is_connected())
        {
            if (!listener.isAxesUpdated())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(Timing::Milliseconds::JOYSTICK));
                continue;
            }

            Types::Vector<int> axes = listener.axes();

            if (axes[Axes::X] != prevAxes.at(Axes::X) || axes[Axes::Y] != prevAxes.at(Axes::Y) || axes[Axes::RZ] != prevAxes.at(Axes::RZ) || axes[Axes::PITCH] != prevAxes.at(Axes::PITCH))
            {

                std::vector<int> atmega_axes = {
                    axes[Axes::X],
                    axes[Axes::Y],
                    axes[Axes::RZ],
                    axes[Axes::PITCH]};

                Types::Vector<int> atmega = atmega_axes;
                publisher.publish(Topics::AXES, atmega);

                prevAxes[Axes::X] = axes[Axes::X];
                prevAxes[Axes::Y] = axes[Axes::Y];
                prevAxes[Axes::RZ] = axes[Axes::RZ];
                prevAxes[Axes::PITCH] = axes[Axes::PITCH];
            }

            if (axes[Axes::SHOULDER] != prevAxes.at(Axes::SHOULDER))
            {
                int shoulder_axes = axes[Axes::SHOULDER];
                nlohmann::json shoulder = shoulder_axes;
                publisher.publish(Topics::SHOULDER_VELOCITY, shoulder.dump());

                prevAxes[Axes::SHOULDER] = axes[Axes::SHOULDER];
            }

            if (axes[Axes::WRIST] != prevAxes.at(Axes::WRIST))
            {
                int shoulder_wrist = axes[Axes::WRIST];
                nlohmann::json wrist = shoulder_wrist;
                publisher.publish(Topics::WRIST_VELOCITY, wrist.dump());

                prevAxes[Axes::WRIST] = axes[Axes::WRIST];
            }

            if (axes[Axes::HAND] != prevAxes.at(Axes::HAND))
            {
                int shoulder_hand = axes[Axes::HAND];
                nlohmann::json hand = shoulder_hand;
                publisher.publish(Topics::HAND_VELOCITY, hand.dump());

                prevAxes[Axes::HAND] = axes[Axes::HAND];
            }
        }
    });

    buttonTalker_ = new std::thread([&]() {
        while (publisher.is_connected())
        {

            if (!listener.isButtonUpdated())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(Timing::Milliseconds::COMMANDS));
                continue;
            }

            Button button = listener.button();
            int id = button.getId();
            int value = button.getValue();

            string action = Actions::NONE;

            string topic = "";

            // Parsing button by identifier
            switch (id)
            {
            case Buttons::START_AND_STOP:
                topic = Topics::COMMANDS;
                if (value)
                    action = Actions::ATMega::START_AND_STOP;
                break;

            case Buttons::MOTORS:
                topic = Topics::COMMANDS;

                if (value && ComponentsManager::GetComponentState(component_t::POWER) == Component::Status::ENABLED)
                    action = Actions::OFF;
                else if (value && ComponentsManager::GetComponentState(component_t::POWER) == Component::Status::DISABLED)
                    action = Actions::ON;
                else
                    break;

                break;

            case Buttons::RESET:
                topic = Topics::COMMANDS;
                if (value)
                    action = Actions::RESET;
                break;

            case Buttons::VUP:
                topic = Topics::COMMANDS;
                value ? action = Actions::ATMega::VUP_ON : action = Actions::ATMega::VUP_OFF;
                break;

            case Buttons::VUP_FAST:
                topic = Topics::COMMANDS;
                value ? action = Actions::ATMega::VUP_FAST_ON : action = Actions::ATMega::VUP_FAST_OFF;
                break;

            case Buttons::VDOWN:
                topic = Topics::COMMANDS;
                value ? action = Actions::ATMega::VDOWN_ON : action = Actions::ATMega::VDOWN_OFF;
                break;

            case Buttons::SLOW:
                topic = Topics::COMMANDS;
                if (value)
                    action = Actions::ATMega::SLOW;
                break;

            case Buttons::MEDIUM_FAST:
                topic = Topics::COMMANDS;
                if (value)
                    action = Actions::ATMega::MEDIUM;
                else
                    action = Actions::ATMega::FAST;
                break;

            case Buttons::SHOULDER_ENABLE:
                topic = Topics::SHOULDER;
                if (value)
                    action = Actions::ON;
                break;

            case Buttons::SHOULDER_DISABLE:
                topic = Topics::SHOULDER;
                if (value)
                    action = Actions::OFF;
                break;

            case Buttons::WRIST_ENABLE:
                topic = Topics::WRIST;
                if (value)
                    action = Actions::ON;
                break;

            case Buttons::WRIST_DISABLE:
                topic = Topics::WRIST;
                if (value)
                    action = Actions::OFF;
                break;

            case Buttons::WRIST:
                topic = Topics::WRIST;
                if (value)
                    action = Actions::START;
                else
                    action = Actions::STOP;
                break;

            case Buttons::SHOULDER_UP:
                topic = Topics::SHOULDER;
                if (value)
                    action = Actions::Stepper::UP;
                else
                    action = Actions::STOP;
                break;

            case Buttons::SHOULDER_DOWN:
                topic = Topics::SHOULDER;
                if (value)
                    action = Actions::Stepper::DOWN;
                else
                    action = Actions::STOP;
                break;

            case Buttons::HAND:
                topic = Topics::HAND;
                if (value)
                    action = Actions::START;
                else
                    action = Actions::STOP;
                break;

            case Buttons::HEAD_ENABLE:
                topic = Topics::HEAD;
                if (value)
                    action = Actions::ON;
                break;

            case Buttons::HEAD_DISABLE:
                topic = Topics::HEAD;
                if (value)
                    action = Actions::OFF;
                break;

            case Buttons::HEAD_UP:
                topic = Topics::HEAD;
                if (value)
                    action = Actions::Stepper::UP;
                else
                    action = Actions::STOP;
                break;

            case Buttons::HEAD_DOWN:
                topic = Topics::HEAD;
                if (value)
                    action = Actions::Stepper::DOWN;
                else
                    action = Actions::STOP;
                break;

            case Buttons::PITCH_CONTROL:
                topic = Topics::COMMANDS;
                if (value)
                    action = Actions::ATMega::PITCH_CONTROL;
                break;

            default:
                break;
            }

            if (action != Actions::NONE)
                publisher.publish(topic, action);
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