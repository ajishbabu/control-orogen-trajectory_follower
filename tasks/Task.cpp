/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"
#include <base/Logging.hpp>
#include <random>

using namespace trajectory_follower;

Task::Task(std::string const& name)
    : TaskBase(name)
{
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
{
}

Task::~Task()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    if (! TaskBase::configureHook())
        return false;
    return true;
}
bool Task::startHook()
{
    trajectoryFollower = TrajectoryFollower( _follower_config.value() );

    if (! TaskBase::startHook())
        return false;
    return true;
}
void Task::updateHook()
{
    TaskBase::updateHook();

    motionCommand.translation = 0;
    motionCommand.rotation    = 0;
    motionCommand.heading     = 0;

    if( _robot_pose.readNewest( rbpose ) == RTT::NoData)
    {
        if( !trajectories.empty() )
        {
            LOG_WARN_S << "Clearing old trajectories, since there is no "
                       "trajectory or pose data.";
        }

        trajectoryFollower.removeTrajectory();
        _motion_command.write(motionCommand);

        return;
    }

    base::Pose robotPose = base::Pose( rbpose.position, rbpose.orientation );

    if (_trajectory.readNewest(trajectories, false) == RTT::NewData && !trajectories.empty()) {
        trajectoryFollower.setNewTrajectory(SubTrajectory(trajectories.front()), robotPose);
        trajectories.erase(trajectories.begin());
    }
    
    SubTrajectory subTrajectory;
    if (_holonomic_trajectory.readNewest(subTrajectory, false) == RTT::NewData) {
        std::cout << "got subTrajectory.." << std::endl;
        trajectories.clear();
        trajectoryFollower.setNewTrajectory(subTrajectory, robotPose);
    }

    FollowerStatus status = trajectoryFollower.traverseTrajectory(motionCommand, robotPose);

    switch(status)
    {
    case TRAJECTORY_FINISHED:
        if(!trajectories.empty())
        {
            trajectoryFollower.setNewTrajectory(trajectories.front(), robotPose);
            trajectories.erase(trajectories.begin());
        }
        else if(state() != FINISHED_TRAJECTORIES)
        {
            LOG_INFO_S << "update TrajectoryFollowerTask state to FINISHED_TRAJECTORIES.";
            state(FINISHED_TRAJECTORIES);
        }
        break;
    case TRAJECTORY_FOLLOWING:
        if(state() != FOLLOWING_TRAJECTORY)
        {
            LOG_INFO_S << "update TrajectoryFollowerTask state to FOLLOWING_TRAJECTORY.";
            state(FOLLOWING_TRAJECTORY);
        }
        break;
    case SLAM_POSE_CHECK_FAILED:
        if(state() != SLAM_POSE_INVALID)
        {
            LOG_INFO_S << "update TrajectoryFollowerTask state to SLAM_POSE_INVALID.";
            state(SLAM_POSE_INVALID);
        }
        break;
    case INITIAL_STABILITY_FAILED:
        if(state() != STABILITY_FAILED)
        {
            LOG_ERROR_S << "update TrajectoryFollowerTask state to STABILITY_FAILED.";
            state(STABILITY_FAILED);
        }
        break;
    default:
        std::runtime_error("Unknown TrajectoryFollower state");
    }
    
    _follower_data.write(trajectoryFollower.getData());
    _motion_command.write(motionCommand);
}

void Task::errorHook()
{
    TaskBase::errorHook();
}

void Task::stopHook()
{
    motionCommand.translation = 0;
    motionCommand.rotation    = 0;
    motionCommand.heading     = 0;
    _motion_command.write(motionCommand);

    TaskBase::stopHook();
}

void Task::cleanupHook()
{
    TaskBase::cleanupHook();
}

void Task::gen_lateral(const base::samples::RigidBodyState& current_pose, const ::base::samples::RigidBodyState& target_pose, double speed)
{
    base::Pose2D curPos;
    curPos.position.x() = current_pose.position.x();
    curPos.position.y() = current_pose.position.y();
    curPos.orientation = current_pose.getYaw();
    
    base::Position2D tarPos;
    tarPos.x() = target_pose.position.x();
    tarPos.y() = target_pose.position.y();
    
    Lateral lateralTrajectory(curPos, tarPos, speed);
    base::Pose robotPose = base::Pose(current_pose.position, current_pose.orientation);
    trajectoryFollower.setNewTrajectory(lateralTrajectory, robotPose);
    updateHook();
}