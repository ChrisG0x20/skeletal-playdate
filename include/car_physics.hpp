//
// Copyright (c) 2022 Christopher Gassib
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CLGCARPHYSICS_HPP
#define CLGCARPHYSICS_HPP

#include "box2d/box2d.h"
#include "clg-math/clg_vector.hpp"

// TODO:
//  Add standard braking.
//  Add handbraking for the rear wheels.
//  Maybe add line locking for braking the front wheels.
//  Fix physics for peel-out.
//  Add traction control.
//  Add anti-lock brakes.

namespace clg
{
namespace formula
{
constexpr float gravitationalAcceleration = 9.80665f; // m/s2
constexpr float absoluteTemperature = 20.0f; // C
constexpr float absoluteAirPressure = 101.325f; // kPa
constexpr float specificGasConstant = 287.058f; // for dry air (J/kg*K))
constexpr float C2K = 273.15f;

inline constexpr float RecalculateMassDensityOfAir(float temperatureC, float pressurekPa)
{
    // (absolute pressure (Pa)) / (specific gas constant * absolute temperture K)
    const auto result = (pressurekPa / 1000.0f) / (specificGasConstant * (temperatureC + C2K));
    return result;
}

inline constexpr float RecalculateMassDensityOfAir(float temperatureC)
{
    return RecalculateMassDensityOfAir(temperatureC, absoluteAirPressure);
}

// Verified this is about 1.2041f with the defaults.
//pd::logToConsole("Mass Density of Air: %d", massDensityOfAir);
constexpr float massDensityOfAir = RecalculateMassDensityOfAir(absoluteTemperature, absoluteAirPressure); // kg/m^3

// Drag Area:
//      0.790 m^2    for average full-size passenger cars
//      0.47         1999 Honda Insight
//      2.46         2003 Hummer H2
//      0.60 to 0.70 for an average bicycle
//      0.576 m^2    Tesla Model S
//
// Drag Coefficient:
//      0.29 to 0.4 for sports cars
//      0.43 to 0.5 for pickup trucks
//      0.6  to 0.9 for tractor-trailers
//      0.4  to 0.5 for average economy cars
//      0.24        Tesla Model S
inline constexpr float AerodynamicDrag(float speed, float frontalAreaNormalToVelocity, float dragCoefficient)
{
    return 0.5f * massDensityOfAir * (speed * speed) * frontalAreaNormalToVelocity * dragCoefficient;
}

inline constexpr float AerodynamicDrag(float speed, float frontalAreaNormalToVelocity)
{
    return AerodynamicDrag(speed, frontalAreaNormalToVelocity, 0.24f);
}

inline constexpr float AerodynamicDrag(float speed)
{
    return AerodynamicDrag(speed, 0.576f, 0.24f);
}

// Coefficient of Rolling Resistance:
//      0.015           typical car tires
//      0.0062 to 0.015 car tire range
//      0.010 to 0.015  ordinary car tires on concrete
//      0.3             ordinary car tires on sand
//      0.006 to 0.01   truck tires
//      0.0045 to 0.008 semi truck tires
//
// NOTE: Weight can be the whole vehicle weight, or this can be computed
//       for each tire independantly and totalled.
inline constexpr float RollingResistance(float weightSupportedByTire, float coefficientOfRollingResistance)
{
    return coefficientOfRollingResistance * weightSupportedByTire;
}

inline constexpr float RollingResistance(float weightSupportedByTire)
{
    return RollingResistance(weightSupportedByTire, 0.015f);
}

inline constexpr float TotalDrag(float speed, float weightSupportedByTires)
{
    return AerodynamicDrag(speed) + RollingResistance(weightSupportedByTires);
}

// tireRadius in meters
inline constexpr float TireForceOnRoad(float torqueOnTire, float tireRadius)
{
    return torqueOnTire / tireRadius;
}

inline constexpr float TireForceOnRoad(float torqueOnTire)
{
    return torqueOnTire / 0.2794f; // meter radius or 558.8 mm (22 inch) diameter
}

inline constexpr float TireFrictionForce(float weightSupportedByTire, float coefficientOfFriction)
{
    return coefficientOfFriction * weightSupportedByTire;
}

inline constexpr float TireDynamicFrictionForce(float weightSupportedByTire)
{
    return 2.0f * weightSupportedByTire;
}

inline constexpr float TireStaticFrictionForce(float weightSupportedByTire)
{
    return 3.5f * weightSupportedByTire;
}

} // namespace Formula

class Tire
{
public:
    enum class ControlState : int
    {
        Neutral = 0x0,
        Left    = 0x1,
        Right   = 0x2,
        Up      = 0x4,
        Down    = 0x8
    };

    Tire()
        : m_body(nullptr)
    {
    }

    void Initialize(b2Body* body)
    {
        m_body = body;
        //skidSound = body.GetComponent<AudioSource>();
    }

    void setCharacteristics(float maxBackwardSpeed, float maxDriveForce)
    {
        m_maxBackwardSpeed = maxBackwardSpeed;
        m_maxDriveForce = maxDriveForce;
    }

    b2Vec2 getLateralVelocity()
    {
        const auto currentRightNormal = m_body->GetWorldVector(b2Vec2(1.0f, 0.0f));
        const auto result = b2Dot(currentRightNormal, m_body->GetLinearVelocity()) * currentRightNormal;
        return result;
    }

    b2Vec2 getForwardVelocity()
    {
        const auto currentForwardNormal = m_body->GetWorldVector(b2Vec2(0.0f, 1.0f));
        const auto result = b2Dot(currentForwardNormal, m_body->GetLinearVelocity()) * currentForwardNormal;
        return result;
    }

    // NOTE: deltaTime is in seconds
    void updateFriction(float weightSupportedByTire, float deltaTime)
    {
        const auto lateralVelocity = getLateralVelocity();
        const auto kgPerSecond = (weightSupportedByTire / formula::gravitationalAcceleration) / deltaTime;
        const b2Vec2 lateralForce(lateralVelocity.x * kgPerSecond, lateralVelocity.y * kgPerSecond);

        // Start with static coefficient of friction
        const auto frictionalForceMagnitude = formula::TireStaticFrictionForce(weightSupportedByTire);

        const auto lateralForceMagnitude = lateralForce.Length();
        auto lateralVelocityNormal = lateralVelocity;
        lateralVelocityNormal.Normalize();

        // if (the tire shouldn't be skidding)
        if (frictionalForceMagnitude > lateralForceMagnitude)
        {
            // friction counter acts lateral forces
            lateralVelocityNormal *= -lateralForceMagnitude;
            m_body->ApplyForceToCenter(lateralVelocityNormal, true);

            if (IsSkidding)
            {
                IsSkidding = false;
                // skidSound.Stop();
            }
        }
        else // else (the tire is skidding)
        {
            // skid using dynamic coefficient of friction
            const auto dynamicFrictionalForceMagnitude = formula::TireDynamicFrictionForce(weightSupportedByTire);
            lateralVelocityNormal *= -dynamicFrictionalForceMagnitude;
            m_body->ApplyForceToCenter(lateralVelocityNormal, true);

            // TODO: where does the 60.0f come from??? -- threashold for making noise
            if (!IsSkidding && lateralForceMagnitude > dynamicFrictionalForceMagnitude * 60.0f)
            {
                IsSkidding = true;
                // skidSound.Play();
            }
        }
    }

    // NOTE: The top speed is simply an emergent property of motor torque and
    //  drag. However, top speed in reverse is artificially limited.
    void updateDrive(ControlState controlState, float tireRollingResistance)
    {
        // Figure out what direction the car is facing.
        auto currentForwardNormal = m_body->GetWorldVector(b2Vec2(0.0f, 1.0f));
        const auto currentSpeed = b2Dot(getForwardVelocity(), currentForwardNormal);
        Speed = currentSpeed;

        // Check the accelerator.
        float driveForce = 0.0f;
        switch (static_cast<ControlState>(
            static_cast<int>(controlState) & (static_cast<int>(ControlState::Up) | static_cast<int>(ControlState::Down))
            ))
        {
            case ControlState::Up:
                {
                    driveForce = formula::TireForceOnRoad(m_maxDriveForce);
                }
                break;

            case ControlState::Down:
                {
                    // Calculate current speed moving in the forward direction.
                    if (currentSpeed > m_maxBackwardSpeed)
                    {
                        driveForce = -formula::TireForceOnRoad(m_maxDriveForce);
                        break;
                    }
                }
                return;

            default:
                return; // do nothing
        }

        currentForwardNormal *= (driveForce - tireRollingResistance);
        m_body->ApplyForceToCenter(currentForwardNormal, true);
    }

    float Speed;
    b2Body* m_body;
    //AudioSource skidSound;
    bool IsSkidding = false;

private:
    float m_maxBackwardSpeed;
    float m_maxDriveForce;
}; // class Tire

class Car
{
public:
    enum class TireIndex : int
    {
        FrontLeft   = 0,
        FrontRight  = 1,
        BackLeft    = 2,
        BackRight   = 3
    };

    // NOTE: Tesla Model S P85D is limited to 15 mph in reverse.
    static constexpr float maxBackwardSpeed = -22.352f;   // m/s (about 50 mph)

    // TODO: Change these fixed values to some sort of torque curve.
    static constexpr float backTireMaxDriveForce = 601.0f; // Nm
    static constexpr float frontTireMaxDriveForce = 331.0f;

    static constexpr float totalWeight = 2240.0f; // kg
    static constexpr float wheelWeight = 15.0f; // kg per tire

    //JointMotor2D powerSteering = new JointMotor2D();

    b2Body* m_body;
    Tire m_tires[4];
    b2RevoluteJoint* flJoint;
    b2RevoluteJoint* frJoint;

    float Speed = 0.0f;

    Car()
        : m_body(nullptr)
        , flJoint(nullptr)
        , frJoint(nullptr)
        , Speed(0.0f)
    {
    }

    void Initialize(b2Body* body, b2Body* tireBodies[4], b2RevoluteJoint* wheelJoint[2])
    {
        m_body = body;
        flJoint = wheelJoint[0];
        frJoint = wheelJoint[1];
        for (unsigned int i = 0; i < array_count(m_tires); i++)
        {
            m_tires[i].Initialize(tireBodies[i]);
        }

        flJoint->SetMaxMotorTorque(500.0f);
        frJoint->SetMaxMotorTorque(500.0f);
        m_tires[0].setCharacteristics(maxBackwardSpeed, frontTireMaxDriveForce / 2.0f);
        m_tires[1].setCharacteristics(maxBackwardSpeed, frontTireMaxDriveForce / 2.0f);
        m_tires[2].setCharacteristics(maxBackwardSpeed, backTireMaxDriveForce / 2.0f);
        m_tires[3].setCharacteristics(maxBackwardSpeed, backTireMaxDriveForce / 2.0f);

        // Debug.Log("Total Drag: " + formula::TotalDrag(0.0f, totalWeight));
    }

    // TODO: The steering input should maybe be handled before the physics are
    //  computed...
    void update(Tire::ControlState controlState, float deltaTime)
    {
        // m_body->drag = formula::AerodynamicDrag(m_body->velocity.magnitude);
        {
            auto velocityNormal = m_body->GetLinearVelocity();
            const auto currentSpeed = velocityNormal.Normalize();
            velocityNormal *= -formula::AerodynamicDrag(currentSpeed);
            m_body->ApplyForceToCenter(velocityNormal, true);
        }

        auto totalSpeed = 0.0f;
        for (auto& tire : m_tires)
        {
            // NOTE: Drive force (the tire pushing against the road) should be
            //  calculated alongside friction so that burnout may be simulated.
            tire.updateFriction(totalWeight / clg::array_count(m_tires), deltaTime);
            tire.updateDrive(controlState, formula::RollingResistance(totalWeight / 4.0f));

            totalSpeed += tire.Speed;
        }
        Speed = totalSpeed / clg::array_count(m_tires);

        // Update steering controls with user input
        ///////////////////////////////////////////

        // NOTE: The front wheels can pivot a hard-coded 40 degs in either direction.
        // TODO: This value should either be read out of the Unity editor's
        //  hinge joint property, or the value should be pushed from code
        //  overwriting the hinge joint instance's limits.

        // NOTE: This is a mostly arbitrary turning rate.
        //  The original idea was to take about half a second to transition from
        //  steering wheel full-left to full-right.
        // TODO: Test with 360 controller and keyboard to get the feel "right".

        switch (static_cast<Tire::ControlState>(
            static_cast<int>(controlState) & (static_cast<int>(Tire::ControlState::Left) | static_cast<int>(Tire::ControlState::Right))
            ))
        {
            case Tire::ControlState::Left:
                {
                    if (flJoint->GetJointAngle() > -40.0f)
                    {
                        const auto radiansPerSecond = clg::to_radians(-160.0f) * 4;
                        flJoint->SetMotorSpeed(radiansPerSecond);
                        frJoint->SetMotorSpeed(radiansPerSecond);
                        flJoint->EnableMotor(true);
                        frJoint->EnableMotor(true);
                    }
                }
                return;

            case Tire::ControlState::Right:
                {
                    if (flJoint->GetJointAngle() < 40.0f)
                    {
                        const auto radiansPerSecond = clg::to_radians(160.0f) * 4;
                        flJoint->SetMotorSpeed(radiansPerSecond);
                        frJoint->SetMotorSpeed(radiansPerSecond);
                        flJoint->EnableMotor(true);
                        frJoint->EnableMotor(true);
                    }
                }
                return;

            default:
                break;
        }

        // When there is no steering input, drift the wheels back to center
        ///////////////////////////////////////////////////////////////////
        const auto radiansPerSecond = flJoint->GetJointAngle() * 10.0f;

        if (flJoint->GetJointAngle() < -0.1f)
        {
            flJoint->SetMotorSpeed(radiansPerSecond);
            frJoint->SetMotorSpeed(radiansPerSecond);
            flJoint->EnableMotor(true);
            frJoint->EnableMotor(true);
            return;
        }

        if (flJoint->GetJointAngle() > 0.1f)
        {
            flJoint->SetMotorSpeed(radiansPerSecond);
            frJoint->SetMotorSpeed(radiansPerSecond);
            flJoint->EnableMotor(true);
            frJoint->EnableMotor(true);
            return;
        }

        flJoint->EnableMotor(false);
        frJoint->EnableMotor(false);
    }
}; // class Car

} // namespace clg

#endif // CLGCARPHYSICS_HPP
